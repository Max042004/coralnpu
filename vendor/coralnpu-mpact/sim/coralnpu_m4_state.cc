// Copyright 2026 Google LLC
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

#include "sim/coralnpu_m4_state.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "sim/coralnpu_v2_state.h"
#include "sim/coralnpu_xstatus.h"
#include "riscv/riscv_csr.h"
#include "riscv/riscv_state.h"
#include "riscv/riscv_vector_state.h"
#include "mpact/sim/util/memory/memory_interface.h"

namespace coralnpu::sim {

CoralNPUM4State::CoralNPUM4State(
    std::string id, ::mpact::sim::riscv::RiscVXlen xlen,
    ::mpact::sim::util::MemoryInterface* memory,
    ::mpact::sim::util::AtomicMemoryOpInterface* atomic_memory)
    : CoralNPUV2State(std::move(id), xlen, memory, atomic_memory) {
  // Override the default mstatus with the CoralNPU extended one for Zvt.
  const uint64_t old_mstatus_val = mstatus_->GetUint64();
  csr_set()
      ->RemoveCsr(
          static_cast<uint64_t>(::mpact::sim::riscv::RiscVCsrEnum::kMStatus))
      .IgnoreError();

  constexpr uint64_t kMsInitial =
      static_cast<uint64_t>(::mpact::sim::riscv::RiscVMStatus::kStatusInitial)
      << 29;
  auto* new_mstatus =
      new CoralNPUMStatus(old_mstatus_val | kMsInitial, this, misa());
  csr_set()->AddCsr(new_mstatus).IgnoreError();
  mstatus_ = new_mstatus;
}

CoralNPUM4State::~CoralNPUM4State() {
  // Our overridden mstatus is not in csr_vec_, so we clean it up.
  delete mstatus_;
}

void CoralNPUM4State::MarkMatrixDirty() {
  auto* ms = static_cast<CoralNPUMStatus*>(mstatus());
  if (ms != nullptr && ms->ms() != ::mpact::sim::riscv::RiscVMStatus::kStatusOff &&
      ms->ms() != ::mpact::sim::riscv::RiscVMStatus::kStatusDirty) {
    ms->set_ms(::mpact::sim::riscv::RiscVMStatus::kStatusDirty);
    ms->Submit();
  }
  if (rv_vector() != nullptr) {
    rv_vector()->MarkDirty();
  }
}

void CoralNPUM4State::MarkMatrixInitial() {
  auto* ms = static_cast<CoralNPUMStatus*>(mstatus());
  if (ms != nullptr && ms->ms() != ::mpact::sim::riscv::RiscVMStatus::kStatusOff &&
      ms->ms() != ::mpact::sim::riscv::RiscVMStatus::kStatusInitial) {
    ms->set_ms(::mpact::sim::riscv::RiscVMStatus::kStatusInitial);
    ms->Submit();
  }
}

bool CoralNPUM4State::IsMatrixStatusOff() const {
  auto* ms = static_cast<CoralNPUMStatus*>(
      const_cast<CoralNPUM4State*>(this)->mstatus());
  return ms != nullptr &&
         ms->ms() == ::mpact::sim::riscv::RiscVMStatus::kStatusOff;
}

namespace {
inline uint64_t StretchMisa32(uint32_t value) {
  uint64_t value64 = static_cast<uint64_t>(value);
  value64 = ((value64 & 0xc000'0000) << 32) | (value64 & 0x03ff'ffff);
  return value64;
}

}  // namespace

std::unique_ptr<CoralNPUM4State> CreateCoralNPUM4State(
    std::string id, ::mpact::sim::riscv::RiscVXlen xlen,
    ::mpact::sim::util::MemoryInterface* memory,
    ::mpact::sim::util::AtomicMemoryOpInterface* atomic_memory,
    const CoralNPUV2StateConfig* config) {
  auto state = std::make_unique<CoralNPUM4State>(std::move(id), xlen, memory,
                                                 atomic_memory);
  if (config != nullptr) {
    state->misa()->Set(StretchMisa32(config->initial_misa_value));
    for (const auto& region : config->memory_regions) {
      state->AddMemoryRegion(region.start_address, region.length,
                             region.permissions);
    }
  }
  return state;
}

}  // namespace coralnpu::sim
