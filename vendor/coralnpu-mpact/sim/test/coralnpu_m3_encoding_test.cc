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

#include "sim/coralnpu_m3_encoding.h"

#include <cstdint>
#include <memory>

#include "sim/coralnpu_m3_enums.h"
#include "sim/coralnpu_v2_state.h"
#include "googletest/include/gtest/gtest.h"
#include "riscv/riscv_state.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"

namespace {

using ::coralnpu::sim::CoralNPUM3Encoding;
using ::coralnpu::sim::CreateCoralNPUV2State;
using ::mpact::sim::riscv::RiscVXlen;
using ::mpact::sim::util::FlatDemandMemory;
using SlotEnum = ::coralnpu::sim::isa32_m3::SlotEnum;
using OpcodeEnum = ::coralnpu::sim::isa32_m3::OpcodeEnum;
using SourceOpEnum = ::coralnpu::sim::isa32_m3::SourceOpEnum;

TEST(CoralNPUM3EncodingTest, TestInvalidCsrIndex) {
  auto memory = std::make_unique<FlatDemandMemory>();
  auto state = CreateCoralNPUV2State("test", RiscVXlen::RV32, memory.get());
  CoralNPUM3Encoding encoding(state.get());

  // CSRRS with an invalid CSR index (e.g., 0xFFF).
  // CSRRS: inst[31:20] = csr, inst[19:15] = rs1, inst[14:12] = funct3,
  // inst[11:7] = rd, inst[6:0] = opcode.
  // Opcode for SYSTEM is 0x73. Funct3 for CSRRS is 0x2.
  uint32_t inst_word =
      (0xFFFU << 20) | (0x1U << 15) | (0x2U << 12) | (0x2U << 7) | 0x73U;
  encoding.ParseInstruction(inst_word);

  auto* source_op = encoding.GetSource(
      SlotEnum::kCoralnpuM3, 0, OpcodeEnum::kCsrrs, SourceOpEnum::kCsr, 0);

  ASSERT_NE(source_op, nullptr);
  // It should return an ImmediateOperand with the CSR index.
  EXPECT_EQ(source_op->AsUint32(0), 0xFFF);
  delete source_op;
}

}  // namespace
