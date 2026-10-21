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

#ifndef SIM_CORALNPU_M3_USER_DECODER_H_
#define SIM_CORALNPU_M3_USER_DECODER_H_

#include <cstdint>
#include <memory>

#include "sim/coralnpu_m3_decoder.h"
#include "sim/coralnpu_m3_encoding.h"
#include "sim/coralnpu_m3_enums.h"
#include "sim/coralnpu_v2_state.h"
#include "absl/base/nullability.h"
#include "riscv/riscv_generic_decoder.h"
#include "mpact/sim/generic/arch_state.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/decoder_interface.h"
#include "mpact/sim/generic/instruction.h"
#include "mpact/sim/generic/program_error.h"
#include "mpact/sim/util/memory/memory_interface.h"

namespace coralnpu::sim {

// This is the factory class needed by the generated decoder. It is responsible
// for creating the decoder for each slot instance. Since the riscv architecture
// only has a single slot, it's a pretty simple class.
class CoralNPUM3IsaFactory
    : public ::coralnpu::sim::isa32_m3::CoralNPUM3InstructionSetFactory {
  using ArchState = ::mpact::sim::generic::ArchState;
  using CoralnpuM3Slot = ::coralnpu::sim::isa32_m3::CoralnpuM3Slot;

 public:
  std::unique_ptr<CoralnpuM3Slot> CreateCoralnpuM3Slot(
      ArchState* state) override {
    return std::make_unique<CoralnpuM3Slot>(state);
  }
};

class CoralNPUM3UserDecoder : public ::mpact::sim::generic::DecoderInterface {
 public:
  using CoralNPUM3Encoding = ::coralnpu::sim::CoralNPUM3Encoding;
  using CoralNPUM3InstructionSet =
      ::coralnpu::sim::isa32_m3::CoralNPUM3InstructionSet;
  using CoralNPUV2State = ::coralnpu::sim::CoralNPUV2State;
  using DataBuffer = ::mpact::sim::generic::DataBuffer;
  using Instruction = ::mpact::sim::generic::Instruction;
  using MemoryInterface = ::mpact::sim::util::MemoryInterface;
  using OpcodeEnum = ::coralnpu::sim::isa32_m3::OpcodeEnum;
  using ProgramError = ::mpact::sim::generic::ProgramError;
  using RiscVGenericDecoder =
      ::mpact::sim::riscv::RiscVGenericDecoder<CoralNPUV2State, OpcodeEnum,
                                               CoralNPUM3Encoding,
                                               CoralNPUM3InstructionSet>;

  CoralNPUM3UserDecoder(CoralNPUV2State* /*absl_nonnull*/ state,
                        MemoryInterface* /*absl_nonnull*/ memory);
  ~CoralNPUM3UserDecoder() override;

  // Decodes an instruction at the given address.
  Instruction* DecodeInstruction(uint64_t address) override;

  // Returns the number of opcodes supported by this decoder.
  int GetNumOpcodes() const override;

  // Returns the name of the opcode at the given index.
  const char* GetOpcodeName(int index) const override;

 private:
  CoralNPUV2State* state_;
  std::unique_ptr<ProgramError> decode_error_;
  std::unique_ptr<CoralNPUM3Encoding> coralnpu_m3_encoding_;
  std::unique_ptr<CoralNPUM3IsaFactory> coralnpu_m3_isa_factory_;
  std::unique_ptr<CoralNPUM3InstructionSet> coralnpu_m3_isa_;
  std::unique_ptr<RiscVGenericDecoder> decoder_;
};

}  // namespace coralnpu::sim

#endif  // SIM_CORALNPU_M3_USER_DECODER_H_
