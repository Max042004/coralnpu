// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "riscv/riscv_xstatus.h"

#include <cstdint>

#include "riscv/riscv_csr.h"
#include "riscv/riscv_misa.h"
#include "riscv/riscv_state.h"
#include "mpact/sim/generic/type_helpers.h"

namespace mpact {
namespace sim {
namespace riscv {

using ::mpact::sim::generic::operator*;  // NOLINT: is used below (clang error).

constexpr uint64_t kSetMaskFrom32 = 0xffff'fff0'ffff'ffffULL;

// Helper functions to convert between 32 and 64 bit views of mstatus.
constexpr uint64_t kRv32SdBit = uint64_t{1} << 31;
constexpr uint64_t kRv32NonSdBits = uint64_t{0x7fff'ffff};

static inline uint64_t StretchMStatus32(uint32_t value) {
  uint64_t value64 = static_cast<uint64_t>(value);
  value64 = ((value64 & kRv32SdBit) << 32) | (value64 & kRv32NonSdBits);
  return value64;
}

static inline uint32_t CompressMStatus64(uint64_t value) {
  uint32_t value32 = static_cast<uint32_t>(((value >> 32) & kRv32SdBit) |
                                           (value & kRv32NonSdBits));
  return value32;
}

// Constructors.

RiscVMStatus::RiscVMStatus(uint32_t initial_value, ArchState* state,
                           RiscVMIsa* misa, uint64_t read_mask,
                           uint64_t write_mask)
    : RiscVMStatus(StretchMStatus32(initial_value), state, RiscVXlen::RV32,
                   misa, read_mask, write_mask) {}

RiscVMStatus::RiscVMStatus(uint64_t initial_value, ArchState* state,
                           RiscVMIsa* misa, uint64_t read_mask,
                           uint64_t write_mask)
    : RiscVMStatus(initial_value, state,
                   (state != nullptr) ? static_cast<RiscVState*>(state)->xlen()
                                      : RiscVXlen::RV64,
                   misa, read_mask, write_mask) {}

RiscVMStatus::RiscVMStatus(uint64_t initial_value, ArchState* state,
                           RiscVXlen xlen, RiscVMIsa* misa, uint64_t read_mask,
                           uint64_t write_mask)
    : RiscVSimpleCsr<uint64_t>("mstatus", RiscVCsrEnum::kMStatus, initial_value,
                               read_mask, write_mask, state),
      misa_(misa),
      read_mask_32_(xlen == RiscVXlen::RV32
                        ? CompressMStatus64(read_mask)
                        : static_cast<uint32_t>(read_mask & kLower32Mask)),
      write_mask_32_(xlen == RiscVXlen::RV32
                         ? CompressMStatus64(write_mask)
                         : static_cast<uint32_t>(write_mask & kLower32Mask)),
      set_mask_from_32_(~(kMstatusSxlMask | kMstatusUxlMask)) {
  uint64_t initial_mstatus = this->RiscVSimpleCsr<uint64_t>::GetUint64();
  if (xlen == RiscVXlen::RV64) {
    if (misa_ != nullptr && misa_->HasSupervisorMode()) {
      initial_mstatus |= (kXlen64Encoding << kMstatusSxlShift);
    }
    if (misa_ != nullptr && misa_->HasUserMode()) {
      initial_mstatus |= (kXlen64Encoding << kMstatusUxlShift);
    }
  }
  this->RiscVSimpleCsr<uint64_t>::Set(initial_mstatus);
}

// Override implementations.
uint32_t RiscVMStatus::AsUint32() {
  // The value is stored as if xlen is 64, so adjust the layout.
  return GetUint32() & read_mask_32_;
}

uint64_t RiscVMStatus::AsUint64() {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    return static_cast<uint64_t>(AsUint32());
  }
  return GetUint64() & read_mask();
}

void RiscVMStatus::Write(uint32_t value) {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    Set(value & write_mask_32_);
  } else {
    uint64_t current = GetUint64();
    uint64_t new_value = (current & ~static_cast<uint64_t>(write_mask_32_)) |
                         static_cast<uint64_t>(value & write_mask_32_);
    Set(new_value);
  }
}

void RiscVMStatus::SetBits(uint32_t bits) {
  uint32_t new_value = GetUint32() | (bits & write_mask_32_);
  Set(new_value);
}

void RiscVMStatus::ClearBits(uint32_t bits) {
  uint32_t new_value = GetUint32() & ~(bits & write_mask_32_);
  Set(new_value);
}

uint32_t RiscVMStatus::GetUint32() {
  uint64_t val = RiscVSimpleCsr<uint64_t>::GetUint64();
  int fs_val = static_cast<int>((val & kMstatusFsMask) >> kMstatusFsShift);
  int vs_val = static_cast<int>((val & kMstatusVsMask) >> kMstatusVsShift);
  int xs_val = static_cast<int>((val & kMstatusXsMask) >> kMstatusXsShift);
  uint32_t val32 = static_cast<uint32_t>(val & kLower32Mask);
  if (fs_val == kStatusDirty || vs_val == kStatusDirty ||
      xs_val == kStatusDirty) {
    val32 |= static_cast<uint32_t>(kMstatusSdMaskRv32);
  } else {
    val32 &= ~static_cast<uint32_t>(kMstatusSdMaskRv32);
  }
  return val32;
}

uint64_t RiscVMStatus::GetUint64() {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    return static_cast<uint64_t>(GetUint32());
  }
  uint64_t val = RiscVSimpleCsr<uint64_t>::GetUint64();
  int fs_val = static_cast<int>((val & kMstatusFsMask) >> kMstatusFsShift);
  int vs_val = static_cast<int>((val & kMstatusVsMask) >> kMstatusVsShift);
  int xs_val = static_cast<int>((val & kMstatusXsMask) >> kMstatusXsShift);
  if (fs_val == kStatusDirty || vs_val == kStatusDirty ||
      xs_val == kStatusDirty) {
    val |= kMstatusSdMaskRv64;
  } else {
    val &= ~kMstatusSdMaskRv64;
  }
  return val;
}

bool RiscVMStatus::sd() {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    return (GetUint32() & static_cast<uint32_t>(kMstatusSdMaskRv32)) != 0;
  }
  return (GetUint64() & kMstatusSdMaskRv64) != 0;
}

void RiscVMStatus::Set(uint32_t value) {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    uint64_t new_value = (StretchMStatus32(value) & set_mask_from_32_) |
                         (GetUint64() & ~set_mask_from_32_);
    Set(new_value);
  } else {
    uint64_t current = GetUint64();
    uint64_t new_value =
        (current & kUpper32Mask) | static_cast<uint64_t>(value);
    Set(new_value);
  }
}

void RiscVMStatus::Set(uint64_t value) {
  uint64_t old_value = GetUint64();
  value &= ~(kMstatusSdMaskRv32 | kMstatusSdMaskRv64);
  this->RiscVSimpleCsr<uint64_t>::Set(value);
  if (state() != nullptr &&
      (old_value & kInterruptMask) != (value & kInterruptMask)) {
    state()->CheckForInterrupt();
  }
}

void RiscVMStatus::set_mpp(uint32_t value) {
  // Don't allow values for unsupported privilege modes.
  if ((value == *PrivilegeMode::kUser) &&
      (misa_ != nullptr && !misa_->HasUserMode())) {
    return;
  }
  if ((value == *PrivilegeMode::kSupervisor) &&
      (misa_ != nullptr && !misa_->HasSupervisorMode())) {
    return;
  }
  SetterHelper<11, 0b11>(value);
}

// RiscVSStatus methods.

RiscVSStatus::RiscVSStatus(RiscVMStatus* mstatus, RiscVState* state)
    : RiscVSimpleCsr<uint64_t>("sstatus", RiscVCsrEnum::kSStatus, kReadMask,
                               kWriteMask, state),
      mstatus_(mstatus) {
  set_mask_from_32_ =
      ~(RiscVMStatus::kMstatusSxlMask | RiscVMStatus::kMstatusUxlMask);
  read_mask_32_ = CompressMStatus64(read_mask());
  write_mask_32_ = CompressMStatus64(write_mask());
}

uint32_t RiscVSStatus::AsUint32() {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    return mstatus_->AsUint32() & read_mask_32_;
  }
  return CompressMStatus64(AsUint64()) & read_mask_32_;
}

uint64_t RiscVSStatus::AsUint64() { return mstatus_->AsUint64() & read_mask(); }

void RiscVSStatus::Set(uint32_t value) {
  uint64_t new_value = StretchMStatus32(value);
  Set(new_value);
}

// Only update those bytes of mstatus that can be written into sstatus.
void RiscVSStatus::Set(uint64_t value) {
  mstatus_->Set((mstatus_->GetUint64() & ~write_mask()) |
                (value & write_mask()));
}

void RiscVSStatus::Write(uint32_t value) { Set(value & write_mask_32_); }

void RiscVSStatus::SetBits(uint32_t bits) {
  uint32_t new_value = GetUint32() | (bits & write_mask_32_);
  Set(new_value);
}

void RiscVSStatus::ClearBits(uint32_t bits) {
  uint32_t new_value = GetUint32() & ~(bits & write_mask_32_);
  Set(new_value);
}

uint32_t RiscVSStatus::GetUint32() {
  if (state() != nullptr &&
      static_cast<RiscVState*>(state())->xlen() == RiscVXlen::RV32) {
    return mstatus_->GetUint32() & read_mask_32_;
  }
  return CompressMStatus64(GetUint64()) & read_mask_32_;
}

uint64_t RiscVSStatus::GetUint64() {
  return mstatus_->GetUint64() & read_mask();
}

// RiscVUStatus methods.

RiscVUStatus::RiscVUStatus(RiscVMStatus* mstatus, RiscVState* state)
    : RiscVSimpleCsr<uint64_t>("ustatus", RiscVCsrEnum::kUStatus, kReadMask,
                               kWriteMask, state),
      mstatus_(mstatus) {
  set_mask_from_32_ =
      ~(RiscVMStatus::kMstatusSxlMask | RiscVMStatus::kMstatusUxlMask);
  read_mask_32_ = CompressMStatus64(read_mask());
  write_mask_32_ = CompressMStatus64(write_mask());
}

uint32_t RiscVUStatus::AsUint32() { return CompressMStatus64(AsUint64()); }

uint64_t RiscVUStatus::AsUint64() { return mstatus_->AsUint64(); }

void RiscVUStatus::Write(uint32_t value) { Set(value & write_mask_32_); }

void RiscVUStatus::SetBits(uint32_t bits) {
  uint32_t new_value = GetUint32() | (bits & write_mask_32_);
  Set(new_value);
}

void RiscVUStatus::ClearBits(uint32_t bits) {
  uint32_t new_value = GetUint32() & ~(bits & write_mask_32_);
  Set(new_value);
}

void RiscVUStatus::Set(uint32_t value) {
  uint64_t new_value = StretchMStatus32(value);
  Set(new_value);
}

// Only update those bytes of mstatus that can be written into ustatus.
void RiscVUStatus::Set(uint64_t value) {
  mstatus_->Set((mstatus_->GetUint64() & ~write_mask()) |
                (value & write_mask()));
}

uint32_t RiscVUStatus::GetUint32() { return CompressMStatus64(GetUint64()); }

uint64_t RiscVUStatus::GetUint64() { return mstatus_->GetUint64(); }

}  // namespace riscv
}  // namespace sim
}  // namespace mpact
