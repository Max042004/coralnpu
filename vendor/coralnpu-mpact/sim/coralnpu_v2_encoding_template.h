// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SIM_CORALNPU_V2_ENCODING_TEMPLATE_H_
#define SIM_CORALNPU_V2_ENCODING_TEMPLATE_H_

#include <cstdint>

#include "sim/coralnpu_v2_getters.h"
#include "sim/coralnpu_v2_state.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "riscv/riscv_encoding_common.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/generic/simple_resource.h"

namespace coralnpu::sim {

template <typename Base, typename OpcodeEnum, typename SlotEnum,
          typename SourceOpEnum, typename DestOpEnum, typename Extractors>
class CoralNPUV2EncodingTemplate
    : public Base,
      public ::mpact::sim::riscv::RiscVEncodingCommon {
 public:
  using DestinationOperandInterface =
      ::mpact::sim::generic::DestinationOperandInterface;
  using SimpleResourcePool = ::mpact::sim::generic::SimpleResourcePool;
  using SourceOperandInterface = ::mpact::sim::generic::SourceOperandInterface;

  using SourceOpGetterMap =
      absl::flat_hash_map<int, absl::AnyInvocable<SourceOperandInterface*()>>;
  using DestOpGetterMap = absl::flat_hash_map<
      int, absl::AnyInvocable<DestinationOperandInterface*(int)>>;

  explicit CoralNPUV2EncodingTemplate(CoralNPUV2State* /*absl_nonnull*/ state)
      : RiscVEncodingCommon(), state_(state) {
    source_op_getters_.emplace(
        *SourceOpEnum::kNone,
        []() -> SourceOperandInterface* { return nullptr; });
    dest_op_getters_.emplace(
        *DestOpEnum::kNone,
        [](int latency) -> DestinationOperandInterface* { return nullptr; });
    // Add CoralNPU ISA source operand getters.
    AddCoralNPUV2SourceGetters<SourceOpEnum, Extractors>(source_op_getters_,
                                                         this);
    // Add CoralNPU ISA destination operand getters.
    AddCoralNPUV2DestGetters<DestOpEnum, Extractors>(dest_op_getters_, this);
  }

  // Based on EncodingBase
  OpcodeEnum GetOpcode(SlotEnum, int) override { return opcode_; }

  // The following method returns a source operand that corresponds to the
  // particular operand field.
  SourceOperandInterface* GetSource(SlotEnum, int, OpcodeEnum, SourceOpEnum op,
                                    int source_no) override {
    auto iter = source_op_getters_.find(*op);
    if (iter == source_op_getters_.end()) return nullptr;
    return iter->second();
  }

  // The following method returns a destination operand that corresponds to the
  // particular operand field.
  DestinationOperandInterface* GetDestination(SlotEnum, int, OpcodeEnum,
                                              DestOpEnum op, int dest_no,
                                              int latency) override {
    auto iter = dest_op_getters_.find(*op);
    if (iter == dest_op_getters_.end()) return nullptr;
    return iter->second(latency);
  }

  // This method returns latency for any destination operand for which the
  // latency specifier in the .isa file is '*'. Since there are none, just
  // return 0.
  int GetLatency(SlotEnum, int, OpcodeEnum, DestOpEnum, int) override {
    return 0;
  }

  // Based on RiscVEncodingCommon
  CoralNPUV2State* state() const override { return state_; }

  SimpleResourcePool* resource_pool() override { return nullptr; }

  uint32_t inst_word() const override { return inst_word_; }

  // Parses an instruction and determines the opcode.
  void ParseInstruction(uint32_t inst_word, OpcodeEnum (*Decode)(uint32_t)) {
    inst_word_ = inst_word;
    opcode_ = Decode(inst_word_);
  }

  const SourceOpGetterMap& source_op_getters() { return source_op_getters_; }
  const DestOpGetterMap& dest_op_getters() { return dest_op_getters_; }

 protected:
  uint32_t inst_word_;
  OpcodeEnum opcode_;
  CoralNPUV2State* state_;
  SourceOpGetterMap source_op_getters_;
  DestOpGetterMap dest_op_getters_;
};

}  // namespace coralnpu::sim

#endif  // SIM_CORALNPU_V2_ENCODING_TEMPLATE_H_
