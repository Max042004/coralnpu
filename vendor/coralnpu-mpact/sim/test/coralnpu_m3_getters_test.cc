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

#include <cstdint>
#include <memory>

#include "sim/coralnpu_m3_encoding.h"
#include "sim/coralnpu_m3_enums.h"
#include "sim/coralnpu_v2_state.h"
#include "googletest/include/gtest/gtest.h"
#include "riscv/riscv_register.h"
#include "riscv/riscv_state.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"

namespace {

using ::coralnpu::sim::CoralNPUM3Encoding;
using ::coralnpu::sim::CreateCoralNPUV2State;
using ::mpact::sim::riscv::RiscVXlen;
using ::mpact::sim::riscv::RVFpRegister;
using ::mpact::sim::util::FlatDemandMemory;
using SlotEnum = ::coralnpu::sim::isa32_m3::SlotEnum;
using OpcodeEnum = ::coralnpu::sim::isa32_m3::OpcodeEnum;
using SourceOpEnum = ::coralnpu::sim::isa32_m3::SourceOpEnum;

TEST(CoralNPUM3GettersTest, TestFrs1) {
  auto memory = std::make_unique<FlatDemandMemory>();
  auto state = CreateCoralNPUV2State("test", RiscVXlen::RV32, memory.get());
  CoralNPUM3Encoding encoding(state.get());

  uint32_t inst_word = (0xBU << 15);  // rs1 = 11
  encoding.ParseInstruction(inst_word);

  auto* source_op = encoding.GetSource(
      SlotEnum::kCoralnpuM3, 0, OpcodeEnum::kFaddS, SourceOpEnum::kFrs1, 0);

  ASSERT_NE(source_op, nullptr);
  state->GetRegister<RVFpRegister>("f11").first->data_buffer()->Set<uint64_t>(
      0, 0x1111222233334444ULL);
  EXPECT_EQ(source_op->AsUint64(0), 0x1111222233334444ULL);
  delete source_op;
}

TEST(CoralNPUM3GettersTest, TestFrs2) {
  auto memory = std::make_unique<FlatDemandMemory>();
  auto state = CreateCoralNPUV2State("test", RiscVXlen::RV32, memory.get());
  CoralNPUM3Encoding encoding(state.get());

  uint32_t inst_word = (0xCU << 20);  // rs2 = 12
  encoding.ParseInstruction(inst_word);

  auto* source_op = encoding.GetSource(
      SlotEnum::kCoralnpuM3, 0, OpcodeEnum::kFaddS, SourceOpEnum::kFrs2, 0);

  ASSERT_NE(source_op, nullptr);
  state->GetRegister<RVFpRegister>("f12").first->data_buffer()->Set<uint64_t>(
      0, 0x5555666677778888ULL);
  EXPECT_EQ(source_op->AsUint64(0), 0x5555666677778888ULL);
  delete source_op;
}

TEST(CoralNPUM3GettersTest, TestFrs3) {
  auto memory = std::make_unique<FlatDemandMemory>();
  auto state = CreateCoralNPUV2State("test", RiscVXlen::RV32, memory.get());
  CoralNPUM3Encoding encoding(state.get());

  uint32_t inst_word = (0xDU << 27);  // rs3 = 13
  encoding.ParseInstruction(inst_word);

  auto* source_op = encoding.GetSource(
      SlotEnum::kCoralnpuM3, 0, OpcodeEnum::kFmaddS, SourceOpEnum::kFrs3, 0);

  ASSERT_NE(source_op, nullptr);
  state->GetRegister<RVFpRegister>("f13").first->data_buffer()->Set<uint64_t>(
      0, 0x9999AAAABBBBCCCCULL);
  EXPECT_EQ(source_op->AsUint64(0), 0x9999AAAABBBBCCCCULL);
  delete source_op;
}

}  // namespace
