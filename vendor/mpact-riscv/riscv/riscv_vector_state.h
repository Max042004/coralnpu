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

#ifndef MPACT_RISCV_RISCV_RISCV_VECTOR_STATE_H_
#define MPACT_RISCV_RISCV_RISCV_VECTOR_STATE_H_

#include <cstdint>  // NOLINT(misc-include-cleaner)

#include "riscv/riscv_csr.h"

// This file contains the definition of the vector state class. This class
// is used by the vector instructions to obtain information about the state
// and configuration of the vector unit. This class is also used to provide
// values that are read from CSRs, and updated by by values written to CSRs.

namespace mpact {
namespace sim {
namespace riscv {

inline constexpr uint32_t kVillBitMask = 0x80000000U;

class RiscVState;
class RiscVVectorState;

// Implementation of the 'vl' CSR.
class RiscVVl : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVl(RiscVVectorState* vector_state);

  // Overrides. Note that this CSR is read-only.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }

 private:
  const RiscVVectorState* const vector_state_;
};

// Implementation of the 'vtype' CSR.
class RiscVVtype : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVtype(RiscVVectorState* vector_state);

  // Overrides. Note that this CSR is read-only.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }

 private:
  const RiscVVectorState* const vector_state_;
};

// Implementation of the 'vstart' CSR.
class RiscVVstart : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVstart(RiscVVectorState* vector_state);

  // Overrides.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  void Write(uint32_t value) override;
  void Write(uint64_t value) override { Write(static_cast<uint32_t>(value)); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }
  void Set(uint32_t value) override { Write(value); }
  void Set(uint64_t value) override { Write(static_cast<uint32_t>(value)); }

 private:
  RiscVVectorState* const vector_state_;
};

// Implementation of the 'vxsat' CSR.
class RiscVVxsat : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVxsat(RiscVVectorState* vector_state);

  // Overrides.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  void Write(uint32_t value) override;
  void Write(uint64_t value) override { Write(static_cast<uint32_t>(value)); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }
  void Set(uint32_t value) override { Write(value); }
  void Set(uint64_t value) override { Write(static_cast<uint32_t>(value)); }

 private:
  RiscVVectorState* const vector_state_;
};

// Implementation of the 'vxrm' CSR.
class RiscVVxrm : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVxrm(RiscVVectorState* vector_state);

  // Overrides.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  void Write(uint32_t value) override;
  void Write(uint64_t value) override { Write(static_cast<uint32_t>(value)); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }
  void Set(uint32_t value) override { Write(value); }
  void Set(uint64_t value) override { Write(static_cast<uint32_t>(value)); }

 private:
  RiscVVectorState* const vector_state_;
};

// Implementation of the 'vcsr' CSR. This CSR mirrors the bits in 'vxsat' and
// 'vxrm' as follows:
//
//   bits 2:1 - vxrm
//   bits 0:0 - vxsat
class RiscVVcsr : public RiscVSimpleCsr<uint32_t> {
 public:
  explicit RiscVVcsr(RiscVVectorState* vector_state);

  // Overrides.
  uint32_t AsUint32() override;
  uint64_t AsUint64() override { return static_cast<uint64_t>(AsUint32()); }
  void Write(uint32_t value) override;
  void Write(uint64_t value) override { Write(static_cast<uint32_t>(value)); }
  uint32_t GetUint32() override { return AsUint32(); }
  uint64_t GetUint64() override { return AsUint64(); }
  void Set(uint32_t value) override { Write(value); }
  void Set(uint64_t value) override { Write(static_cast<uint32_t>(value)); }

 private:
  RiscVVectorState* const vector_state_;
};

class RiscVVectorState {
 public:
  RiscVVectorState(RiscVState* state, int byte_length, int max_elen = 64);

  void SetVectorType(uint32_t vtype);
  void MarkDirty();

  // Public getters and setters.
  int max_elen() const { return max_elen_; }
  void set_max_elen(int elen) { max_elen_ = elen; }
  int vstart() const { return vstart_; }
  void clear_vstart() {
    if (vstart_ != 0) {
      vstart_ = 0;
      MarkDirty();
    }
  }
  void set_vstart(int value) {
    vstart_ = value;
    MarkDirty();
  }
  int vector_length() const { return vector_length_; }
  void set_vector_length(int value) {
    vector_length_ = value;
    MarkDirty();
  }
  bool vector_tail_agnostic() const { return vector_tail_agnostic_; }
  bool vector_mask_agnostic() const { return vector_mask_agnostic_; }
  int vector_length_multiplier() const { return vector_length_multiplier_; }
  int selected_element_width() const { return selected_element_width_; }
  bool vector_exception() const { return vector_exception_; }
  void clear_vector_exception() { vector_exception_ = false; }
  void set_vector_exception() { vector_exception_ = true; }
  uint32_t vtype() const { return vtype_; }
  void set_vtype(uint32_t value) { vtype_ = value; }
  bool is_vill() const { return (vtype_ & kVillBitMask) != 0; }
  int vector_register_byte_length() const {
    return vector_register_byte_length_;
  }
  int max_vector_length() const { return max_vector_length_; }
  bool vxsat() const { return vxsat_; }
  void set_vxsat(bool value) {
    vxsat_ = value;
    MarkDirty();
  }
  int vxrm() const { return vxrm_; }
  void set_vxrm(int value) {
    vxrm_ = value & 0x3;
    MarkDirty();
  }

  // Controls the behavior of tail elements when an instruction operates under
  // a tail-agnostic policy (either via vtype.vta=1 or implicitly for mask
  // destination instructions per RISC-V Vector Specification Section 15). The
  // specification allows an implementation to either leave the tail undisturbed
  // or overwrite it with 1s. When set to true, tail elements are overwritten
  // with 1s to match the RTL implementation of CoralNPU / Kelvin. When false,
  // they remain undisturbed.
  bool mask_tail_agnostic_writes_ones() const {
    return mask_tail_agnostic_writes_ones_;
  }
  void set_mask_tail_agnostic_writes_ones(bool val) {
    mask_tail_agnostic_writes_ones_ = val;
  }

  const RiscVState* riscv_state() const { return state_; }
  RiscVState* riscv_state() { return state_; }

 private:
  // Vector length multiplier is scaled by 8, to provide integer representation
  // of values from 1/8, 1/4, 1/2, 1, 2, 4, 8, as 1, 2, 4, 8, 16, 32, 64.
  void set_vector_length_multiplier(int value) {
    vector_length_multiplier_ = value;
  }
  void set_selected_element_width(int value) {
    selected_element_width_ = value;
  }
  void set_vector_tail_agnostic(bool value) { vector_tail_agnostic_ = value; }
  void set_vector_mask_agnostic(bool value) { vector_mask_agnostic_ = value; }

  RiscVState* state_ = nullptr;
  uint32_t vtype_ = 0x80000000;
  bool vector_exception_ = false;
  int vector_register_byte_length_ = 0;
  int max_elen_ = 64;
  int vstart_ = 0;
  int max_vector_length_ = 0;
  int vector_length_ = 0;
  int vector_length_multiplier_ = 8;
  // Selected element width (SEW) in bytes.
  int selected_element_width_ = 1;
  bool vector_tail_agnostic_ = false;
  bool vector_mask_agnostic_ = false;
  bool mask_tail_agnostic_writes_ones_ = false;
  bool vxsat_ = false;
  int vxrm_ = 0;

  RiscVVl vl_csr_;
  RiscVVtype vtype_csr_;
  RiscVSimpleCsr<uint32_t> vlenb_csr_;
  RiscVVstart vstart_csr_;
  RiscVVxsat vxsat_csr_;
  RiscVVxrm vxrm_csr_;
  RiscVVcsr vcsr_csr_;
};

}  // namespace riscv
}  // namespace sim
}  // namespace mpact
#endif  // MPACT_RISCV_RISCV_RISCV_VECTOR_STATE_H_
