// Copyright 2025 Google LLC
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

#include "sim/coralnpu_v2_instructions.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "sim/coralnpu_v2_state.h"
#include "googlemock/include/gmock/gmock.h"
#include "googletest/include/gtest/gtest.h"
#include "absl/functional/any_invocable.h"
#include "absl/functional/bind_front.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "riscv/riscv_f_instructions.h"
#include "riscv/riscv_i_instructions.h"
#include "riscv/riscv_register.h"
#include "riscv/riscv_register_aliases.h"
#include "riscv/riscv_state.h"
#include "riscv/riscv_vector_memory_instructions.h"
#include "riscv/riscv_vector_state.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/immediate_operand.h"
#include "mpact/sim/generic/instruction.h"
#include "mpact/sim/generic/operand_interface.h"
#include "mpact/sim/generic/register.h"
#include "mpact/sim/generic/type_helpers.h"
#include "mpact/sim/util/memory/flat_demand_memory.h"
#include "mpact/sim/util/memory/memory_interface.h"

/* start - new test macros */
#define ABSL_EXPECT_OK(expression) \
  EXPECT_THAT(expression, ::absl_testing::IsOk())
#define ABSL_ASSERT_OK(expression) \
  ASSERT_THAT(expression, ::absl_testing::IsOk())
/* end - new test macros */

/* start - helper test macro */
#define ASSERT_OK_AND_ASSIGN(lhs, rexpr) \
  auto status_or = (rexpr); \
  ABSL_ASSERT_OK(status_or); \
  lhs = std::move(status_or).value();
/* end - helper test macro */

namespace {
using ::coralnpu::sim::CoralNPUV2Fsw;
using ::coralnpu::sim::CoralNPUV2Jal;
using ::coralnpu::sim::CoralNPUV2Jalr;
using ::coralnpu::sim::CoralNPUV2Lb;
using ::coralnpu::sim::CoralNPUV2Lbu;
using ::coralnpu::sim::CoralNPUV2Lh;
using ::coralnpu::sim::CoralNPUV2Lhu;
using ::coralnpu::sim::CoralNPUV2Lw;
using ::coralnpu::sim::CoralNPUV2Mpause;
using ::coralnpu::sim::CoralNPUV2Sb;
using ::coralnpu::sim::CoralNPUV2Sh;
using ::coralnpu::sim::CoralNPUV2State;
using ::coralnpu::sim::CoralNPUV2Sw;
using ::coralnpu::sim::CoralNPUV2VlIndexed;
using ::coralnpu::sim::CoralNPUV2Vlm;
using ::coralnpu::sim::CoralNPUV2VlRegister;
using ::coralnpu::sim::CoralNPUV2VlSegment;
using ::coralnpu::sim::CoralNPUV2VlSegmentIndexed;
using ::coralnpu::sim::CoralNPUV2VlSegmentStrided;
using ::coralnpu::sim::CoralNPUV2VlStrided;
using ::coralnpu::sim::MemoryPermission;
using ::mpact::sim::generic::DataBuffer;
using ::mpact::sim::generic::ImmediateOperand;
using ::mpact::sim::generic::Instruction;
using ::mpact::sim::generic::RegisterBase;
using ::mpact::sim::generic::SourceOperandInterface;
using ::mpact::sim::riscv::ExceptionCode;
using ::mpact::sim::riscv::kFRegisterAliases;
using ::mpact::sim::riscv::kXRegisterAliases;
using ::mpact::sim::riscv::RiscVIFlwChild;
using ::mpact::sim::riscv::RiscVVectorState;
using ::mpact::sim::riscv::RiscVXlen;
using ::mpact::sim::riscv::RV32Register;
using ::mpact::sim::riscv::RV32VectorDestinationOperand;
using ::mpact::sim::riscv::RV32VectorSourceOperand;
using ::mpact::sim::riscv::RV32VectorTrueOperand;
using ::mpact::sim::riscv::RVFpRegister;
using ::mpact::sim::riscv::RVVectorRegister;
using ::mpact::sim::riscv::VlChild;
using ::mpact::sim::riscv::VlSegmentChild;
using ::mpact::sim::riscv::RV32::RiscVILbChild;
using ::mpact::sim::riscv::RV32::RiscVILbuChild;
using ::mpact::sim::riscv::RV32::RiscVILhChild;
using ::mpact::sim::riscv::RV32::RiscVILhuChild;
using ::mpact::sim::riscv::RV32::RiscVILwChild;
using ::mpact::sim::util::FlatDemandMemory;
using ::mpact::sim::util::MemoryInterface;
using ::testing::Each;
using ::testing::ElementsAreArray;

using ::mpact::sim::generic::operator*;  // NOLINT: clang-tidy false positive.

constexpr uint32_t kGoodLsuAddress = 0x00014000;
constexpr std::array<uint32_t, 5> kTestData = {
    0x12345678, 0x90abcdef, 0x87654321, 0xfedcba09, 0x11223344};
constexpr std::array<uint8_t, 16> kMaskTestData = {
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0x87, 0x65, 0x43, 0x21, 0xfe, 0xdc, 0xba, 0x09};

constexpr uint32_t kBadLsuAddress = 0x100000;
constexpr uint32_t kTestWord = 0xf0f0a5a5;
constexpr uint16_t kTestHalfWord = 0xa5a5;
constexpr uint8_t kTestByte = 0xa5;
constexpr uint64_t kNanBoxedTestWord =
    0xffffffff'00000000 | static_cast<uint64_t>(kTestWord);
constexpr uint32_t kLsuAccessStartAddress = 0x00010000;
constexpr uint32_t kLsuAccessLength = 0x8000;
constexpr uint32_t kItcmStartAddress = 0x00000000;
constexpr uint32_t kItcmLength = 0x2000;

class CoralNPUV2InstructionTest : public ::testing::Test {
 public:
  using SemanticFunction = std::function<void(Instruction*)>;

 protected:
  void SetUp() override {
    memory_ = std::make_unique<FlatDemandMemory>();
    state_ = std::make_unique<CoralNPUV2State>("CoralNPUV2", RiscVXlen::RV32,
                                               memory_.get());
    rv_vector_ = std::make_unique<RiscVVectorState>(
        state_.get(),
        /*byte_length=*/coralnpu::sim::kCoralNPUV2VectorByteLength);
    state_->set_rv_vector(rv_vector_.get());
    state_->AddMemoryRegion(kLsuAccessStartAddress, kLsuAccessLength,
                            MemoryPermission::kReadWrite);
    state_->AddMemoryRegion(kItcmStartAddress, kItcmLength,
                            MemoryPermission::kReadExecute);

    state_->AddMpauseHandler([this](const Instruction*) -> bool {
      was_mpause_handler_called_ = true;
      return true;
    });

    state_->set_on_trap([this](bool, uint64_t trap_value,
                               uint64_t exception_code, uint64_t epc,
                               const Instruction*) -> bool {
      was_trap_handler_called_ = true;
      exception_code_ = static_cast<ExceptionCode>(exception_code);
      trap_value_ = trap_value;
      epc_ = epc;
      return false;
    });

    // Add the scalar, floating point and vector registers to the state.
    std::string reg_name;
    for (int i = 0; i < 32; i++) {
      reg_name = absl::StrCat("x", i);
      x_regs_.push_back(new RV32Register(state_.get(), reg_name));
      state_->AddRegister(reg_name, x_regs_.back());
      ABSL_ASSERT_OK(state_->AddRegisterAlias<RV32Register>(reg_name,
                                                       kXRegisterAliases[i]));

      reg_name = absl::StrCat("f", i);
      f_regs_.push_back(new RVFpRegister(state_.get(), reg_name));
      state_->AddRegister(reg_name, f_regs_.back());
      ABSL_ASSERT_OK(state_->AddRegisterAlias<RVFpRegister>(reg_name,
                                                       kFRegisterAliases[i]));

      reg_name = absl::StrCat("v", i);
      v_regs_.push_back(new RVVectorRegister(
          state_.get(), reg_name,
          /*width=*/coralnpu::sim::kCoralNPUV2VectorByteLength));
      state_->AddRegister(reg_name, v_regs_.back());
    }
    pc_reg_ = std::make_unique<RV32Register>(state_.get(), "pc");
  }

  void AttachLoadChildInstruction(Instruction*, SemanticFunction);

  void SetMemoryContents(uint32_t address,
                         absl::Span<const uint32_t> test_data);
  void SetMemoryContents(uint32_t address, absl::Span<const uint8_t> test_data);
  void SetMemoryContents(uint32_t address, uint32_t test_data);
  void SetMemoryContents(uint32_t address, uint16_t test_data);
  void SetMemoryContents(uint32_t address, uint8_t test_data);

  void AttachSegmentLoadChildInstruction(Instruction* parent, int nf_value,
                                         int v_dest_start, int element_width);

  std::unique_ptr<Instruction> CreateSegmentLoadInstruction(
      int nf_value, int element_width, int v_dest_start,
      SourceOperandInterface* mask_operand = nullptr);

  std::unique_ptr<Instruction> CreateSegmentStridedLoadInstruction(
      int nf_value, int element_width, int v_dest_start, int64_t stride);

  std::unique_ptr<Instruction> CreateSegmentIndexedLoadInstruction(
      int nf_value, int element_width, int index_width, int v_dest_start,
      int v_index_start, int num_regs = 1,
      SourceOperandInterface* mask_operand = nullptr);

  std::unique_ptr<Instruction> CreateStridedVectorLoadInstruction(
      int element_width, int v_dest, SourceOperandInterface* stride_operand,
      SourceOperandInterface* mask_operand = nullptr, int num_regs = 1);

  std::unique_ptr<Instruction> CreateVlmInstruction(int v_dest,
                                                    int element_width);

  std::unique_ptr<Instruction> CreateVlRegisterInstruction(int num_regs,
                                                           int element_width,
                                                           int v_dest_start);

  std::unique_ptr<Instruction> CreateVlIndexedInstruction(
      int index_width, int v_index, int v_dest,
      SourceOperandInterface* mask_operand = nullptr, int num_regs = 1);

  std::unique_ptr<Instruction> CreateLoadInstruction(
      SemanticFunction parent_semantic_function,
      SemanticFunction child_semantic_function);
  std::unique_ptr<Instruction> CreateStoreInstruction(
      SemanticFunction parent_semantic_function);
  std::unique_ptr<Instruction> CreateFloatLoadInstruction();
  std::unique_ptr<Instruction> CreateFloatStoreInstruction();
  std::unique_ptr<Instruction> CreateJalInstruction(int32_t offset);
  std::unique_ptr<Instruction> CreateJalrInstruction(int32_t offset);

  std::unique_ptr<Instruction> CreateVsIndexedInstruction(
      int index_width, int v_index, int v_src,
      SourceOperandInterface* mask_operand = nullptr, int num_regs = 1);

  std::unique_ptr<Instruction> CreateStridedVectorStoreInstruction(
      int element_width, int v_src, SourceOperandInterface* stride_operand,
      SourceOperandInterface* mask_operand);

  std::unique_ptr<Instruction> CreateVsSegmentInstruction(
      int nf_value, int element_width, int v_src_start,
      SourceOperandInterface* mask_operand = nullptr);

  std::unique_ptr<Instruction> CreateVsSegmentStridedInstruction(
      int nf_value, int element_width, int v_src_start,
      SourceOperandInterface* stride_operand,
      SourceOperandInterface* mask_operand = nullptr);

  std::unique_ptr<Instruction> CreateVsSegmentIndexedInstruction(
      int nf_value, int element_width, int index_width, int v_src_start,
      int v_index_start, SourceOperandInterface* mask_operand = nullptr,
      int num_index_regs = 1);

  std::unique_ptr<Instruction> CreateVsRegisterInstruction(int num_regs,
                                                           int v_src_start);

  std::unique_ptr<Instruction> CreateVsmInstruction(int v_src);

  uint32_t GetXRegValue(RV32Register* reg) {
    return reg->data_buffer()->Get<uint32_t>(/*index=*/0);
  }

  void SetXRegValue(int reg_idx, uint32_t value) {
    x_regs_[reg_idx]->data_buffer()->Set<uint32_t>(/*index=*/0, value);
  }

  template <typename T>
  void SetVRegData(int reg_idx, absl::Span<const T> data) {
    DataBuffer* db = v_regs_[reg_idx]->data_buffer();
    int limit = std::min<int>(data.size(), db->size<T>());
    for (int i = 0; i < limit; i++) {
      db->Set<T>(i, data[i]);
    }
  }

  template <typename T>
  void SetVRegData(int reg_idx, const std::vector<T>& data) {
    DataBuffer* db = v_regs_[reg_idx]->data_buffer();
    int limit = std::min<int>(data.size(), db->size<T>());
    for (int i = 0; i < limit; i++) {
      db->Set<T>(i, data[i]);
    }
  }

  void SetupVectorState(uint32_t vtype, int vl) {
    rv_vector_->SetVectorType(vtype);
    rv_vector_->set_vector_length(vl);
  }

  template <typename T>
  T GetMemoryContents(uint32_t address) {
    DataBuffer* mem_db = state_->db_factory()->Allocate<T>(/*size=*/1);
    memory_->Load(address, mem_db, /*inst=*/nullptr, /*context=*/nullptr);
    T mem_value = mem_db->Get<T>(/*index=*/0);
    mem_db->DecRef();
    return mem_value;
  }

  std::unique_ptr<MemoryInterface> memory_;
  std::unique_ptr<CoralNPUV2State> state_;
  std::unique_ptr<RiscVVectorState> rv_vector_;
  // Registers are owned by the state_.
  std::vector<RV32Register*> x_regs_;
  std::vector<RVFpRegister*> f_regs_;
  std::vector<RVVectorRegister*> v_regs_;
  std::unique_ptr<RV32Register> pc_reg_;
  std::unique_ptr<Instruction> child_inst_;
  bool was_mpause_handler_called_ = false;
  bool was_trap_handler_called_ = false;
  uint64_t trap_value_ = 0;
  uint64_t epc_ = 0;
  ExceptionCode exception_code_ = ExceptionCode::kBreakpoint;
};

void CoralNPUV2InstructionTest::AttachLoadChildInstruction(
    Instruction* parent, SemanticFunction semantic_function) {
  child_inst_ = std::make_unique<Instruction>(parent->address(), state_.get());
  child_inst_->set_size(parent->size());
  child_inst_->set_semantic_function(semantic_function);
  parent->AppendChild(child_inst_.get());
}

void CoralNPUV2InstructionTest::SetMemoryContents(
    uint32_t address, absl::Span<const uint32_t> test_data) {
  DataBuffer* db = state_->db_factory()->Allocate<uint32_t>(test_data.size());
  for (int i = 0; i < test_data.size(); i++) {
    db->Set<uint32_t>(/*index=*/i, test_data[i]);
  }
  memory_->Store(address, db);
  db->DecRef();
}

void CoralNPUV2InstructionTest::SetMemoryContents(
    uint32_t address, absl::Span<const uint8_t> test_data) {
  DataBuffer* db = state_->db_factory()->Allocate<uint8_t>(test_data.size());
  for (int i = 0; i < test_data.size(); i++) {
    db->Set<uint8_t>(/*index=*/i, test_data[i]);
  }
  memory_->Store(address, db);
  db->DecRef();
}

void CoralNPUV2InstructionTest::SetMemoryContents(uint32_t address,
                                                  uint32_t test_data) {
  DataBuffer* db = state_->db_factory()->Allocate<uint32_t>(1);
  db->Set<uint32_t>(/*index=*/0, test_data);
  memory_->Store(address, db);
  db->DecRef();
}

void CoralNPUV2InstructionTest::SetMemoryContents(uint32_t address,
                                                  uint16_t test_data) {
  DataBuffer* db = state_->db_factory()->Allocate<uint16_t>(1);
  db->Set<uint16_t>(/*index=*/0, test_data);
  memory_->Store(address, db);
  db->DecRef();
}

void CoralNPUV2InstructionTest::SetMemoryContents(uint32_t address,
                                                  uint8_t test_data) {
  DataBuffer* db = state_->db_factory()->Allocate<uint8_t>(1);
  db->Set<uint8_t>(/*index=*/0, test_data);
  memory_->Store(address, db);
  db->DecRef();
}

void CoralNPUV2InstructionTest::AttachSegmentLoadChildInstruction(
    Instruction* parent, int nf_value, int v_dest_start, int element_width) {
  AttachLoadChildInstruction(parent,
                             absl::bind_front(&VlSegmentChild, element_width));
  child_inst_->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  int num_regs = nf_value + 1;
  std::vector<RegisterBase*> dest_regs;
  std::string reg_name = absl::StrCat("v", v_dest_start);
  if (num_regs > 1) {
    absl::StrAppend(&reg_name, "-v", v_dest_start + num_regs - 1);
  }
  for (int i = 0; i < num_regs; i++) {
    dest_regs.push_back(v_regs_[v_dest_start + i]);
  }
  child_inst_->AppendDestination(new RV32VectorDestinationOperand(
      absl::MakeSpan(dest_regs), /*latency=*/0, reg_name));
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateSegmentLoadInstruction(
    int nf_value, int element_width, int v_dest_start,
    SourceOperandInterface* mask_operand) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlSegment, element_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  AttachSegmentLoadChildInstruction(inst.get(), nf_value, v_dest_start,
                                    element_width);
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateSegmentStridedLoadInstruction(
    int nf_value, int element_width, int v_dest_start, int64_t stride) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlSegmentStrided, element_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int64_t>(stride));
  inst->AppendSource(new RV32VectorTrueOperand(state_.get()));
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  AttachSegmentLoadChildInstruction(inst.get(), nf_value, v_dest_start,
                                    element_width);
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateSegmentIndexedLoadInstruction(
    int nf_value, int element_width, int index_width, int v_dest_start,
    int v_index_start, int num_regs, SourceOperandInterface* mask_operand) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlSegmentIndexed, index_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  std::vector<RegisterBase*> index_regs;
  for (int i = 0; i < num_regs; i++) {
    index_regs.push_back(v_regs_[v_index_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(index_regs), absl::StrCat("v", v_index_start)));
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  AttachSegmentLoadChildInstruction(inst.get(), nf_value, v_dest_start,
                                    element_width);
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateStridedVectorLoadInstruction(
    int element_width, int v_dest, SourceOperandInterface* stride_operand,
    SourceOperandInterface* mask_operand, int num_regs) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlStrided, element_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(stride_operand);
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  AttachLoadChildInstruction(inst.get(), VlChild);
  std::vector<RegisterBase*> dest_regs;
  for (int i = 0; i < num_regs; ++i) {
    dest_regs.push_back(v_regs_[v_dest + i]);
  }
  child_inst_->AppendDestination(new RV32VectorDestinationOperand(
      absl::MakeSpan(dest_regs), 0, absl::StrCat("v", v_dest)));
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateVlmInstruction(
    int v_dest, int element_width) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(&CoralNPUV2Vlm);
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(element_width));
  inst->AppendSource(new RV32VectorTrueOperand(state_.get()));
  AttachLoadChildInstruction(inst.get(), VlChild);
  std::vector<RegisterBase*> dest_regs = {v_regs_[v_dest]};
  child_inst_->AppendDestination(new RV32VectorDestinationOperand(
      absl::MakeSpan(dest_regs), 0, absl::StrCat("v", v_dest)));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVlRegisterInstruction(int num_regs,
                                                       int element_width,
                                                       int v_dest_start) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlRegister, num_regs, element_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  AttachLoadChildInstruction(inst.get(), VlChild);
  std::vector<RegisterBase*> dest_regs;
  for (int i = 0; i < num_regs; i++) {
    dest_regs.push_back(v_regs_[v_dest_start + i]);
  }
  child_inst_->AppendDestination(new RV32VectorDestinationOperand(
      absl::MakeSpan(dest_regs), 0, absl::StrCat("v", v_dest_start)));

  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVlIndexedInstruction(
    int index_width, int v_index, int v_dest,
    SourceOperandInterface* mask_operand, int num_regs) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&CoralNPUV2VlIndexed, index_width));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  std::vector<RegisterBase*> index_regs;
  for (int i = 0; i < num_regs; i++) {
    index_regs.push_back(v_regs_[v_index + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(absl::MakeSpan(index_regs),
                                                 absl::StrCat("v", v_index)));
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  AttachLoadChildInstruction(inst.get(), VlChild);
  std::vector<RegisterBase*> dest_regs;
  for (int i = 0; i < num_regs; i++) {
    dest_regs.push_back(v_regs_[v_dest + i]);
  }
  child_inst_->AppendDestination(new RV32VectorDestinationOperand(
      absl::MakeSpan(dest_regs), 0, absl::StrCat("v", v_dest)));
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateLoadInstruction(
    SemanticFunction parent_semantic_function,
    SemanticFunction child_semantic_function) {
  // Source operand 1: x register containing the base address to load.
  // Source operand 2: immediate operand containing the offset to load.
  // Destination operand 1: x register to store the loaded data.
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(parent_semantic_function);
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(0));
  AttachLoadChildInstruction(inst.get(), child_semantic_function);
  child_inst_->AppendDestination(
      x_regs_[2]->CreateDestinationOperand(/*latency=*/0));
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateStoreInstruction(
    SemanticFunction parent_semantic_function) {
  // Source operand 1: x register containing the base address to store.
  // Source operand 2: immediate operand containing the offset to store.
  // Source operand 3: x register containing the data to store.
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(parent_semantic_function);
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(0));
  inst->AppendSource(x_regs_[2]->CreateSourceOperand());
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateFloatLoadInstruction() {
  // Source operand 1: x register containing the base address to load.
  // Source operand 2: immediate operand containing the offset to load.
  // Destination operand 1: fp register to store the loaded data.
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(CoralNPUV2Lw);
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(0));
  AttachLoadChildInstruction(inst.get(), RiscVIFlwChild);
  child_inst_->AppendDestination(
      f_regs_[0]->CreateDestinationOperand(/*latency=*/0));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateFloatStoreInstruction() {
  // Source operand 1: x register containing the base address to store.
  // Source operand 2: immediate operand containing the offset to store.
  // Source operand 3: fp register containing the data to store.
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(CoralNPUV2Fsw);
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(0));
  inst->AppendSource(f_regs_[0]->CreateSourceOperand());
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateJalInstruction(
    int32_t offset) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(CoralNPUV2Jal);
  inst->AppendSource(new ImmediateOperand<int32_t>(offset));
  inst->AppendDestination(pc_reg_->CreateDestinationOperand(/*latency=*/0));
  inst->AppendDestination(x_regs_[1]->CreateDestinationOperand(/*latency=*/0));
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateJalrInstruction(
    int32_t offset) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(CoralNPUV2Jalr);
  inst->AppendSource(x_regs_[2]->CreateSourceOperand());
  inst->AppendSource(new ImmediateOperand<int32_t>(offset));
  inst->AppendDestination(pc_reg_->CreateDestinationOperand(/*latency=*/0));
  inst->AppendDestination(x_regs_[1]->CreateDestinationOperand(/*latency=*/0));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVsIndexedInstruction(
    int index_width, int v_index, int v_src,
    SourceOperandInterface* mask_operand, int num_regs) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&::coralnpu::sim::CoralNPUV2VsIndexed, index_width));
  std::vector<RegisterBase*> v_src_regs;
  for (int i = 0; i < num_regs; i++) {
    v_src_regs.push_back(v_regs_[v_src + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(absl::MakeSpan(v_src_regs),
                                                 absl::StrCat("v", v_src)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  std::vector<RegisterBase*> index_regs;
  for (int i = 0; i < num_regs; i++) {
    index_regs.push_back(v_regs_[v_index + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(absl::MakeSpan(index_regs),
                                                 absl::StrCat("v", v_index)));
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateStridedVectorStoreInstruction(
    int element_width, int v_src, SourceOperandInterface* stride_operand,
    SourceOperandInterface* mask_operand) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&::coralnpu::sim::CoralNPUV2VsStrided, element_width));
  std::vector<RegisterBase*> v_src_regs = {v_regs_[v_src]};
  inst->AppendSource(new RV32VectorSourceOperand(absl::MakeSpan(v_src_regs),
                                                 absl::StrCat("v", v_src)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(stride_operand);
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVsSegmentInstruction(
    int nf_value, int element_width, int v_src_start,
    SourceOperandInterface* mask_operand) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&::coralnpu::sim::CoralNPUV2VsSegment, element_width));
  int num_regs = nf_value + 1;
  std::vector<RegisterBase*> v_src_regs;
  for (int i = 0; i < num_regs; i++) {
    v_src_regs.push_back(v_regs_[v_src_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(v_src_regs), absl::StrCat("v", v_src_start)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVsSegmentStridedInstruction(
    int nf_value, int element_width, int v_src_start,
    SourceOperandInterface* stride_operand,
    SourceOperandInterface* mask_operand) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(absl::bind_front(
      &::coralnpu::sim::CoralNPUV2VsSegmentStrided, element_width));
  int num_regs = nf_value + 1;
  std::vector<RegisterBase*> v_src_regs;
  for (int i = 0; i < num_regs; i++) {
    v_src_regs.push_back(v_regs_[v_src_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(v_src_regs), absl::StrCat("v", v_src_start)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  inst->AppendSource(stride_operand);
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVsSegmentIndexedInstruction(
    int nf_value, int element_width, int index_width, int v_src_start,
    int v_index_start, SourceOperandInterface* mask_operand,
    int num_index_regs) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(absl::bind_front(
      &::coralnpu::sim::CoralNPUV2VsSegmentIndexed, element_width));
  int num_regs = nf_value + 1;
  std::vector<RegisterBase*> v_src_regs;
  for (int i = 0; i < num_regs; i++) {
    v_src_regs.push_back(v_regs_[v_src_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(v_src_regs), absl::StrCat("v", v_src_start)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  std::vector<RegisterBase*> index_regs;
  for (int i = 0; i < num_index_regs; i++) {
    index_regs.push_back(v_regs_[v_index_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(index_regs), absl::StrCat("v", v_index_start)));
  if (mask_operand == nullptr) {
    mask_operand = new RV32VectorTrueOperand(state_.get());
  }
  inst->AppendSource(mask_operand);
  inst->AppendSource(new ImmediateOperand<int32_t>(nf_value));
  return inst;
}

std::unique_ptr<Instruction>
CoralNPUV2InstructionTest::CreateVsRegisterInstruction(int num_regs,
                                                       int v_src_start) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(
      absl::bind_front(&::coralnpu::sim::CoralNPUV2VsRegister, num_regs));
  std::vector<RegisterBase*> v_src_regs;
  for (int i = 0; i < num_regs; i++) {
    v_src_regs.push_back(v_regs_[v_src_start + i]);
  }
  inst->AppendSource(new RV32VectorSourceOperand(
      absl::MakeSpan(v_src_regs), absl::StrCat("v", v_src_start)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  return inst;
}

std::unique_ptr<Instruction> CoralNPUV2InstructionTest::CreateVsmInstruction(
    int v_src) {
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(&::coralnpu::sim::CoralNPUV2Vsm);
  std::vector<RegisterBase*> v_src_regs = {v_regs_[v_src]};
  inst->AppendSource(new RV32VectorSourceOperand(absl::MakeSpan(v_src_regs),
                                                 absl::StrCat("v", v_src)));
  inst->AppendSource(x_regs_[1]->CreateSourceOperand());
  return inst;
}

TEST_F(CoralNPUV2InstructionTest, MPause_ExecutesHandler) {
  // Create a test instruction and execute it.
  auto inst = std::make_unique<Instruction>(/*address=*/0, state_.get());
  inst->set_size(4);
  inst->set_semantic_function(CoralNPUV2Mpause);
  inst->Execute(/*context=*/nullptr);

  // Verify that the mpause test handler was called.
  EXPECT_TRUE(was_mpause_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Lw_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestWord);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test load word (lw) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lw, RiscVILwChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  EXPECT_EQ(GetXRegValue(x_regs_[2]), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Lw_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestWord);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test load word (lw) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lw, RiscVILwChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data.
  EXPECT_NE(GetXRegValue(x_regs_[2]), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Sw_ValidAddress_Succeeds) {
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, kTestWord);

  // Create a test store word (sw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sw);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the memory contents were updated with the test data.
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Sw_InvalidAddress_Traps) {
  SetXRegValue(1, kBadLsuAddress);
  SetXRegValue(2, kTestWord);

  // Create a test store word (sw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sw);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the memory contents were not updated since the access was
  // invalid.
  EXPECT_NE(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Lh_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestHalfWord);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test load half word (lh) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lh, RiscVILhChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  uint32_t expected_value =
      (kTestHalfWord & 0x8000) ? 0xffff'0000 | kTestHalfWord : kTestHalfWord;
  EXPECT_EQ(GetXRegValue(x_regs_[2]), expected_value);
}

TEST_F(CoralNPUV2InstructionTest, Lh_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestHalfWord);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test load half word (lh) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lh, RiscVILhChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data.
  uint32_t unwanted_register_value =
      (kTestHalfWord & 0x8000) ? 0xffff'0000 | kTestHalfWord : kTestHalfWord;
  EXPECT_NE(GetXRegValue(x_regs_[2]), unwanted_register_value);
}

TEST_F(CoralNPUV2InstructionTest, Sh_ValidAddress_Succeeds) {
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, kTestHalfWord);

  // Create a test store half word (sh) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sh);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the memory contents were updated with the register contents.
  EXPECT_EQ(GetMemoryContents<uint16_t>(kGoodLsuAddress), kTestHalfWord);
}

TEST_F(CoralNPUV2InstructionTest, Sh_InvalidAddress_Traps) {
  SetXRegValue(1, kBadLsuAddress);
  SetXRegValue(2, kTestHalfWord);

  // Create a test store half word (sh) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sh);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the memory contents were not updated with the register contents
  // since the access is invalid.
  EXPECT_NE(GetMemoryContents<uint16_t>(kBadLsuAddress), kTestHalfWord);
}

TEST_F(CoralNPUV2InstructionTest, Lhu_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestHalfWord);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test load unsigned half word (lhu) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lhu, RiscVILhuChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  uint32_t expected_value = static_cast<uint32_t>(kTestHalfWord);
  EXPECT_EQ(GetXRegValue(x_regs_[2]), expected_value);
}

TEST_F(CoralNPUV2InstructionTest, Lhu_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestHalfWord);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test load unsigned half word (lhu) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lhu, RiscVILhuChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data since
  // the access is invalid.
  uint32_t unwanted_register_value = static_cast<uint32_t>(kTestHalfWord);
  EXPECT_NE(GetXRegValue(x_regs_[2]), unwanted_register_value);
}

TEST_F(CoralNPUV2InstructionTest, Lb_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestByte);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test load byte (lb) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lb, RiscVILbChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  uint32_t expected_value =
      (kTestByte & 0x80) ? 0xffff'ff00 | kTestByte : kTestByte;
  EXPECT_EQ(GetXRegValue(x_regs_[2]), expected_value);
}

TEST_F(CoralNPUV2InstructionTest, Lb_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestByte);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test load byte (lb) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lb, RiscVILbChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data.
  uint32_t unwanted_register_value =
      (kTestByte & 0x80) ? 0xffff'ff00 | kTestByte : kTestByte;
  EXPECT_NE(GetXRegValue(x_regs_[2]), unwanted_register_value);
}

TEST_F(CoralNPUV2InstructionTest, Sb_ValidAddress_Succeeds) {
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, kTestByte);

  // Create a test store byte (sb) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sb);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the memory contents were updated with the register contents.
  EXPECT_EQ(GetMemoryContents<uint8_t>(kGoodLsuAddress), kTestByte);
}

TEST_F(CoralNPUV2InstructionTest, Sb_InvalidAddress_Traps) {
  SetXRegValue(1, kBadLsuAddress);
  SetXRegValue(2, kTestByte);

  // Create a test store byte (sb) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateStoreInstruction(CoralNPUV2Sb);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the memory contents were not updated with the register contents
  // since the access was invalid.
  EXPECT_NE(GetMemoryContents<uint8_t>(kBadLsuAddress), kTestByte);
}

TEST_F(CoralNPUV2InstructionTest, Lbu_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestByte);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test load unsigned byte (lbu) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lbu, RiscVILbuChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  uint32_t expected_value = static_cast<uint32_t>(kTestByte);
  EXPECT_EQ(GetXRegValue(x_regs_[2]), expected_value);
}

TEST_F(CoralNPUV2InstructionTest, Lbu_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestByte);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test load unsigned byte (lbu) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lbu, RiscVILbuChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data.
  uint32_t unwanted_register_value = static_cast<uint32_t>(kTestByte);
  EXPECT_NE(GetXRegValue(x_regs_[2]), unwanted_register_value);
}

// Test with an address where the access spans across the valid LSU range
// boundary.
TEST_F(CoralNPUV2InstructionTest, Lw_AddressBeforeValidRange_Traps) {
  const uint32_t kTestAddress = kLsuAccessStartAddress - 1;
  SetMemoryContents(kTestAddress, kTestWord);
  SetXRegValue(1, kTestAddress);

  // Create a test load word (lw) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lw, RiscVILwChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);

  // Verify that the destination register does not contain the test data.
  EXPECT_NE(GetXRegValue(x_regs_[2]), kTestWord);
}

// Test with an address where the access spans across the valid LSU range
// boundary.
TEST_F(CoralNPUV2InstructionTest, Lw_AddressSpanningEndBoundary_Traps) {
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 3;
  SetMemoryContents(kTestAddress, kTestWord);
  SetXRegValue(1, kTestAddress);

  // Create a test load word (lw) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lw, RiscVILwChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);

  // Verify that the destination register does not contain the test data.
  EXPECT_NE(GetXRegValue(x_regs_[2]), kTestWord);
}

// Test load word at the end of valid LSU range.
TEST_F(CoralNPUV2InstructionTest, Lw_AddressAtEndOfValidRange_Succeeds) {
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetMemoryContents(kTestAddress, kTestWord);
  SetXRegValue(1, kTestAddress);

  // Create a test load word (lw) instruction and execute it.
  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(CoralNPUV2Lw, RiscVILwChild);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register does contain the test data.
  EXPECT_EQ(GetXRegValue(x_regs_[2]), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Flw_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestWord);
  SetXRegValue(1, kGoodLsuAddress);

  // Create a test float load word (flw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateFloatLoadInstruction();
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination register contains the test data from memory.
  uint64_t fp_word = f_regs_[0]->data_buffer()->Get<uint64_t>(0);
  EXPECT_EQ(static_cast<uint32_t>(fp_word), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Flw_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestWord);
  SetXRegValue(1, kBadLsuAddress);

  // Create a test float load word (flw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateFloatLoadInstruction();
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination register does not contain the test data.
  uint64_t fp_word = f_regs_[0]->data_buffer()->Get<uint64_t>(0);
  EXPECT_NE(static_cast<uint32_t>(fp_word), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Fsw_ValidAddress_Succeeds) {
  SetXRegValue(1, kGoodLsuAddress);
  f_regs_[0]->data_buffer()->Set<uint64_t>(/*index=*/0, kNanBoxedTestWord);

  // Create a test float store word (fsw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateFloatStoreInstruction();
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the memory contents were updated with the test data.
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Fsw_InvalidAddress_Traps) {
  SetXRegValue(1, kBadLsuAddress);
  f_regs_[0]->data_buffer()->Set<uint64_t>(/*index=*/0, kNanBoxedTestWord);

  // Create a test float store word (fsw) instruction and execute it.
  std::unique_ptr<Instruction> inst = CreateFloatStoreInstruction();
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the memory contents were not updated since the access was
  // invalid.
  EXPECT_NE(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestWord);
}

TEST_F(CoralNPUV2InstructionTest, Jal_AlignedTarget_Succeeds) {
  int32_t offset = 4;
  std::unique_ptr<Instruction> inst = CreateJalInstruction(offset);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Jal_InvalidTarget_Traps) {
  int32_t offset = kItcmLength;
  std::unique_ptr<Instruction> inst = CreateJalInstruction(offset);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kItcmLength);

  EXPECT_EQ(exception_code_, ExceptionCode::kInstructionAccessFault);
  // The destination register should NOT be updated if trap is taken for Jal.
  EXPECT_EQ(GetXRegValue(x_regs_[1]), 0);
}

TEST_F(CoralNPUV2InstructionTest, Jalr_AlignedTarget_Succeeds) {
  int32_t offset = 0;
  SetXRegValue(2, 4);
  std::unique_ptr<Instruction> inst = CreateJalrInstruction(offset);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Jalr_InvalidTarget_Traps) {
  int32_t offset = 0;
  SetXRegValue(2, kItcmLength);
  std::unique_ptr<Instruction> inst = CreateJalrInstruction(offset);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kItcmLength);

  EXPECT_EQ(exception_code_, ExceptionCode::kInstructionAccessFault);
  // The destination register should be updated even if trap is taken for Jalr.
  EXPECT_EQ(GetXRegValue(x_regs_[1]), inst->address() + inst->size());
}

// Verifies that Jal traps to the correct EPC for an invalid target.
TEST_F(CoralNPUV2InstructionTest, Jal_InvalidTarget_CheckEpc) {
  int32_t offset = kItcmLength;
  uint32_t inst_address = 0x100;
  std::unique_ptr<Instruction> inst = CreateJalInstruction(offset);
  inst->set_address(inst_address);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(epc_, inst_address + offset);
}

// Verifies that Jalr traps to the correct EPC for an invalid target.
TEST_F(CoralNPUV2InstructionTest, Jalr_InvalidTarget_CheckEpc) {
  int32_t offset = 0;
  uint32_t inst_address = 0x100;
  SetXRegValue(2, kItcmLength);
  std::unique_ptr<Instruction> inst = CreateJalrInstruction(offset);
  inst->set_address(inst_address);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(epc_, kItcmLength + offset);
}

TEST_F(CoralNPUV2InstructionTest, VlUnitStride_UnmaskedValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, VlUnitStride_UnmaskedInvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint8_t>(), Each(0));
}

TEST_F(CoralNPUV2InstructionTest, VlUnitStride_MaskedValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestData);
  std::vector<uint32_t> mask_data = {0x0000'0006, 0, 0, 0};
  SetVRegData(0, mask_data);
  std::vector<uint32_t> expected_data = {0x0, 0x90abcdef, 0x87654321, 0x0};
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/1,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/v_regs_[0]->CreateSourceOperand());
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(expected_data));
}

TEST_F(CoralNPUV2InstructionTest, VlUnitStride_MaskedInvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestData);
  std::vector<uint32_t> mask_data = {0x0000'0006, 0, 0, 0};
  SetVRegData(0, mask_data);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/1,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/v_regs_[0]->CreateSourceOperand());
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress + 4);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>(), Each(0));
}

// Verifies that masked unit-stride vector load succeeds when start address is
// before range.
TEST_F(CoralNPUV2InstructionTest,
       VlUnitStride_MaskedStartAddressBeforeRange_Succeeds) {
  const uint32_t kTestAddress = kLsuAccessStartAddress - 4;
  SetMemoryContents(kTestAddress, kTestData);
  std::vector<uint32_t> mask_data = {0x0000'000e, 0, 0, 0};
  SetVRegData(0, mask_data);
  std::vector<uint32_t> expected_data = {0x0, 0x90abcdef, 0x87654321,
                                         0xfedcba09};
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/1,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/v_regs_[0]->CreateSourceOperand());
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(expected_data));
}

// Verifies that masked unit-stride vector load succeeds when end address is
// after range.
TEST_F(CoralNPUV2InstructionTest,
       VlUnitStride_MaskedEndAddressAfterRange_Succeeds) {
  const uint32_t kTestAddress =
      kLsuAccessStartAddress + kLsuAccessLength - 3 * sizeof(uint32_t);
  SetMemoryContents(kTestAddress, kTestData);
  std::vector<uint32_t> mask_data = {0x0000'0007, 0, 0, 0};
  SetVRegData(0, mask_data);
  std::vector<uint32_t> expected_data = {0x12345678, 0x90abcdef, 0x87654321,
                                         0x0};
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/1,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/v_regs_[0]->CreateSourceOperand());
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(expected_data));
}

TEST_F(CoralNPUV2InstructionTest, Vlm_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kMaskTestData);
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // vl=16 -> ceil(16/8) = 2 bytes loaded.
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>().subspan(0, 2),
              ElementsAreArray(absl::MakeSpan(kMaskTestData).subspan(0, 2)));
}

TEST_F(CoralNPUV2InstructionTest, Vlm_AddressBeforeRange_Traps) {
  const uint32_t kTestAddress = kLsuAccessStartAddress - 1;
  SetMemoryContents(kTestAddress, kMaskTestData);
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>(), Each(0));
}

// Verifies that vector mask load succeeds at the exact end of LSU range.
TEST_F(CoralNPUV2InstructionTest, Vlm_AddressExactlyAtEndOfLsu_Succeeds) {
  // LSU range is [0x10000, 0x18000).
  // End address is 0x18000. Last valid byte is 0x17fff.
  // ceil(vl/8) = 2 bytes.
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 2;
  std::vector<uint8_t> test_data(2, 0x5a);
  SetMemoryContents(kTestAddress, test_data);
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kTestAddress);

  // Initialize destination register with zeros.
  std::vector<uint8_t> initial_v1(coralnpu::sim::kCoralNPUV2VectorByteLength,
                                  0);
  SetVRegData(1, initial_v1);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Only first 2 bytes loaded.
  std::vector<uint8_t> expected_data(2, 0x5a);
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>().subspan(0, 2),
              ElementsAreArray(expected_data));
}

// Verifies that vector mask load traps for an address before ITCM non-zero
// start.
TEST_F(CoralNPUV2InstructionTest, Vlm_AddressBeforeItcmNonZeroStart_Traps) {
  const uint32_t kStartAddress = 0x40000;
  state_->AddMemoryRegion(kStartAddress, kItcmLength,
                          MemoryPermission::kReadExecute);

  const uint32_t kTestAddress = kStartAddress - 1;
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);
}

// Verifies that vector mask load traps at the ITCM end address.
TEST_F(CoralNPUV2InstructionTest, Vlm_AddressAtItcmEnd_Traps) {
  const uint32_t kEndAddress = kItcmStartAddress + kItcmLength;
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kEndAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kEndAddress);
}

TEST_F(CoralNPUV2InstructionTest, Vlm_AddressAfterRange_Traps) {
  // LSU range is [0x10000, 0x18000).
  // vl=16 -> ceil(16/8) = 2 bytes loaded.
  // Set address to 0x17fff so that byte 1 (at 0x18000) is out of range.
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 1;
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress + 1);

  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>(), Each(0));
}

TEST_F(CoralNPUV2InstructionTest, Vl1r_ValidAddress_Succeeds) {
  // vl1re32
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, Vl1r_AddressBeforeRange_Traps) {
  // vl1re32
  const uint32_t kTestAddress = kLsuAccessStartAddress - 4;
  SetMemoryContents(kTestAddress, kTestData);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(), Each(0));
}

TEST_F(CoralNPUV2InstructionTest, Vl1r_AddressAfterRange_Traps) {
  // vl1re32
  const uint32_t kTestAddress =
      kLsuAccessStartAddress + kLsuAccessLength - 3 * sizeof(uint32_t);
  SetMemoryContents(kTestAddress, kTestData);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress + 12);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(), Each(0));
}

TEST_F(CoralNPUV2InstructionTest, Vl8r_ValidAddress_Succeeds) {
  // vl8re32
  std::vector<uint32_t> test_data = {
      0x00000000, 0x11111111, 0x22222222, 0x33333333,  // reg 0
      0x44444444, 0x55555555, 0x66666666, 0x77777777,  // reg 1
      0x88888888, 0x99999999, 0xaaaaaaaa, 0xbbbbbbbb,  // reg 2
      0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff,  // reg 3
      0x00000000, 0x11111111, 0x22222222, 0x33333333,  // reg 4
      0x44444444, 0x55555555, 0x66666666, 0x77777777,  // reg 5
      0x88888888, 0x99999999, 0xaaaaaaaa, 0xbbbbbbbb,  // reg 6
      0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff   // reg 7
  };
  SetMemoryContents(kGoodLsuAddress, test_data);
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/8, /*element_width=*/4,
                                  /*v_dest_start=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Verify that the destination registers contain the test data.
  for (int i = 0; i < 8; i++) {
    EXPECT_THAT(v_regs_[8 + i]->data_buffer()->Get<uint32_t>(),
                ElementsAreArray(absl::MakeSpan(test_data).subspan(i * 4, 4)))
        << "Register " << i + 8;
  }
}

TEST_F(CoralNPUV2InstructionTest, Vl8r_InvalidAddress_Traps) {
  // vl8re32
  std::vector<uint32_t> test_data(32, 0xdeadbeef);
  SetMemoryContents(kBadLsuAddress, test_data);
  SetXRegValue(1, kBadLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/8, /*element_width=*/4,
                                  /*v_dest_start=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  // Verify that the destination registers contain the test data.
  for (int i = 0; i < 8; i++) {
    EXPECT_THAT(v_regs_[8 + i]->data_buffer()->Get<uint32_t>(), Each(0))
        << "Register " << i + 8;
  }
}

TEST_F(CoralNPUV2InstructionTest, VlIndexed_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  std::vector<uint32_t> index_data = {0, 4, 8, 12};
  SetVRegData(1, index_data);

  std::unique_ptr<Instruction> inst =
      CreateVlIndexedInstruction(/*index_width=*/4, /*v_index=*/1,
                                 /*v_dest=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, VlIndexed_InvalidAddress_Traps) {
  SetMemoryContents(kBadLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  std::vector<uint32_t> index_data = {0, 4, 8, 12};
  SetVRegData(1, index_data);

  std::unique_ptr<Instruction> inst =
      CreateVlIndexedInstruction(/*index_width=*/4, /*v_index=*/1,
                                 /*v_dest=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(), Each(0));
}

TEST_F(CoralNPUV2InstructionTest, VlSegment_ValidAddress_Succeeds) {
  // 2 elements, 2 fields. Total 4 values in memory.
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateSegmentLoadInstruction(/*nf_value=*/1, /*element_width=*/4,
                                   /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  std::vector<uint32_t> expected_v1 = {kTestData[1], kTestData[3]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v1));
}

TEST_F(CoralNPUV2InstructionTest, VlSegment_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);

  std::unique_ptr<Instruction> inst =
      CreateSegmentLoadInstruction(/*nf_value=*/1, /*element_width=*/4,
                                   /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

TEST_F(CoralNPUV2InstructionTest,
       VlSegment_SecondElementInvalid_TrapsWithCorrectAddress) {
  // Setup: i=0 is valid, but i=1 is invalid.
  // SEW=32 (4 bytes), nf=1 (2 fields). segment_byte_size = 8.
  // base = boundary - 8.
  // i=0: [boundary - 8, boundary) - OK.
  // i=1: [boundary, boundary + 8) - FAIL.
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 8;
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst =
      CreateSegmentLoadInstruction(/*nf_value=*/1, /*element_width=*/4,
                                   /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, boundary);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_ValidAddress_Succeeds) {
  // 2 elements, 2 fields. Total 4 values in memory.
  // Stride 8 bytes.
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);

  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  std::vector<uint32_t> expected_v1 = {kTestData[1], kTestData[3]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v1));
}

TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);

  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_ValidAddress_Succeeds) {
  // 2 elements, 2 fields. Total 4 values.
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*index_width=*/4,
      /*v_dest_start=*/0, /*v_index_start=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  std::vector<uint32_t> expected_v1 = {kTestData[1], kTestData[3]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v1));
}

TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*index_width=*/4,
      /*v_dest_start=*/0, /*v_index_start=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that strided segment vector load traps when the last field is
// invalid.
TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_LastFieldInvalid_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1, vl=1
  uint32_t last_valid_addr = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetXRegValue(1, last_valid_addr);

  // nf=1 (2 fields), stride=8.
  // Field 0: last_valid_addr (Valid)
  // Field 1: last_valid_addr + 4 (Invalid)
  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, last_valid_addr + 4);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that indexed segment vector load traps when the last field is
// invalid.
TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_LastFieldInvalid_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  uint32_t last_valid_addr = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetXRegValue(1, last_valid_addr);
  std::vector<uint32_t> index_data = {0};
  SetVRegData(2, index_data);

  // nf=1 (2 fields).
  // Field 0: last_valid_addr + index[0] = last_valid_addr (Valid)
  // Field 1: last_valid_addr + index[0] + 4 = last_valid_addr + 4 (Invalid)
  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*index_width=*/4,
      /*v_dest_start=*/0, /*v_index_start=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, last_valid_addr + 4);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that indexed segment vector load succeeds with multiple index
// registers.
TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_MultipleRegisters_Succeeds) {
  // SEW=32, LMUL=1, vl=3.
  // vlen_bytes = 16.
  // index_width = 8 (64-bit).
  // indices_per_reg = 16 / 8 = 2.
  // vl=3 means we use 2 registers for indices.
  SetupVectorState(0b11'010'000, /*vl=*/3);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);

  // Indices: {0, 8, 16}.
  // Index i=0,1 are in V2. (64-bit: 0, 8)
  // Index i=2 is in V3. (64-bit: 16)
  std::vector<uint64_t> index_data_v2 = {0, 8};
  std::vector<uint64_t> index_data_v3 = {16, 0};
  SetVRegData(2, index_data_v2);
  SetVRegData(3, index_data_v3);

  // Memory data.
  SetMemoryContents(kGoodLsuAddress, kTestData);

  // nf=0 (1 field), element_width=4.
  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/8,
      /*v_dest_start=*/0, /*v_index_start=*/2, /*num_regs=*/2);
  inst->Execute(/*context=*/nullptr);

  EXPECT_FALSE(was_trap_handler_called_);
  // Expected v0 elements:
  // i=0: base + index[0] = base + 0 -> kTestData[0]
  // i=1: base + index[1] = base + 8 -> kTestData[2]
  // i=2: base + index[2] = base + 16 -> kTestData[4]
  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2], kTestData[4],
                                       0};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(expected_v0));
}

constexpr uint32_t kItcmAccessAddress = kItcmStartAddress + 0x100;

// Verifies that masked indexed vector load succeeds when invalid addresses are
// masked out.
TEST_F(CoralNPUV2InstructionTest, VlIndexed_MaskedInvalidAddress_Succeeds) {
  // The second element will be at an invalid address.
  SetMemoryContents(kGoodLsuAddress, kTestData);

  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  // Index 1 points to kBadLsuAddress.
  std::vector<uint32_t> index_data = {
      0, static_cast<uint32_t>(kBadLsuAddress - kGoodLsuAddress), 8, 12};
  SetVRegData(1, index_data);

  // Mask: element 1 is masked out (bit 1 is 0).
  std::vector<uint8_t> mask_data = {0b1101};
  SetVRegData(0, mask_data);

  std::vector<RegisterBase*> mask_regs = {v_regs_[0]};
  auto inst = CreateVlIndexedInstruction(
      /*index_width=*/4, /*v_index=*/1, /*v_dest=*/2,
      /*mask_operand=*/
      new RV32VectorSourceOperand(absl::MakeSpan(mask_regs), "v0"));

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(v_regs_[2]->data_buffer()->Get<uint32_t>()[0], kTestData[0]);
  EXPECT_EQ(v_regs_[2]->data_buffer()->Get<uint32_t>()[2], kTestData[2]);
  EXPECT_EQ(v_regs_[2]->data_buffer()->Get<uint32_t>()[3], kTestData[3]);
}

// Verifies that masked segment vector load succeeds when invalid addresses are
// masked out.
TEST_F(CoralNPUV2InstructionTest, VlSegment_MaskedInvalidAddress_Succeeds) {
  // 2 elements, 2 fields.
  // We'll make the second segment (index 1) start at an invalid address.
  uint32_t base_address = kLsuAccessStartAddress + kLsuAccessLength - 8;
  // Element 0 is at [base, base+7] - VALID
  // Element 1 is at [base+8, base+15] - INVALID

  SetMemoryContents(base_address, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, base_address);

  // Mask: element 1 is masked out (bit 1 is 0).
  std::vector<uint8_t> mask_data = {0b01};
  SetVRegData(0, mask_data);

  std::vector<RegisterBase*> mask_regs = {v_regs_[0]};
  auto inst = CreateSegmentLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_dest_start=*/2,
      /*mask_operand=*/
      new RV32VectorSourceOperand(absl::MakeSpan(mask_regs), "v0"));

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(v_regs_[2]->data_buffer()->Get<uint32_t>()[0], kTestData[0]);
  EXPECT_EQ(v_regs_[3]->data_buffer()->Get<uint32_t>()[0], kTestData[1]);
}

// Verifies that strided segment vector load traps when the last byte of the
// last field is invalid.
TEST_F(CoralNPUV2InstructionTest,
       VlSegmentStrided_LastByteOfLastFieldInvalid_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  // nf=0 (1 field), element_width=4.
  // Last valid byte is at kLsuAccessStartAddress + kLsuAccessLength - 1.
  // We want the access to be [addr, addr + 3].
  // If addr + 3 is the first invalid byte, then addr = boundary - 3.
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 3;
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/4);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, base);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that strided segment vector load traps when the range wraps around
// the 32-bit address space.
TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_WrappingRange_Traps) {
  // Use base = 1 which is in default ITCM (kReadExecute).
  uint32_t base = 1;

  // vl=3, stride=0x7FFFFFFF.
  // span = 2 * 0x7FFFFFFF = 0xFFFFFFFE.
  // start_addr = 1. end_addr = 1 + 0xFFFFFFFE = 0xFFFFFFFF.
  // No wrap at line 131 (end_addr >= start_addr).
  // max_addr = 0xFFFFFFFF. element_size = 2.
  // range_end = 0xFFFFFFFF + 2 = 0x100000001.
  // Wraps at line 141 (range_end > 0x100000000).
  SetupVectorState(0b11'001'000, /*vl=*/3);  // SEW=16 (2 bytes), LMUL=1
  SetXRegValue(1, base);

  // nf=0 (1 field), element_width=2. total size = 2.
  auto inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/0, /*element_width=*/2, /*v_dest_start=*/0,
      /*stride=*/0x7FFFFFFF);

  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  // It should trap on the second element (i=1) because base + 0x7FFFFFFF is
  // invalid.
  EXPECT_EQ(trap_value_, base + 0x7FFFFFFF);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that strided vector load traps when the span wraps around the 32-bit
// address space.
TEST_F(CoralNPUV2InstructionTest, VlStrided_WrappingSpan_Traps) {
  // Use base = 0x40000, stride = 0xFFFFFFFF, vl = 2.
  // First element: 0x40000 (OK).
  // Second element: 0x40000 + 0xFFFFFFFF = 0x3FFFF (TRAP).
  uint32_t base = 0x40000;
  state_->AddMemoryRegion(/*start_address=*/base, /*length=*/1,
                          MemoryPermission::kReadWrite);

  SetupVectorState(0b11'000'000, /*vl=*/2);  // SEW=8, LMUL=1
  SetXRegValue(1, base);

  auto inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/1, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(0xFFFFFFFF));
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  // Elements: 0x40000 (OK), 0x3FFFF (FAIL).
  EXPECT_EQ(trap_value_, 0x3FFFF);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// This test verifies that a valid strided vector load that does not wrap and is
// within a single memory region succeeds.
TEST_F(CoralNPUV2InstructionTest, VlStrided_NonWrappingValidRange_Succeeds) {
  uint32_t base = kGoodLsuAddress;

  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, base);

  SetMemoryContents(base, kTestData);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);

  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

// Verifies that indexed vector load traps with a valid base but invalid index.
TEST_F(CoralNPUV2InstructionTest, VlIndexed_ValidBaseInvalidIndex_Traps) {
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  std::vector<uint32_t> index_data = {0, 4, 8,
                                      kBadLsuAddress - kGoodLsuAddress};
  SetVRegData(1, index_data);

  std::unique_ptr<Instruction> inst =
      CreateVlIndexedInstruction(/*index_width=*/4, /*v_index=*/1,
                                 /*v_dest=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
}

// Verifies that unit-stride vector load with LMUL=1/8 succeeds.
TEST_F(CoralNPUV2InstructionTest, VlUnitStride_Lmul1_8_ValidAddress_Succeeds) {
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'111, /*vl=*/4);  // SEW=32, LMUL=1/2
  SetXRegValue(1, kGoodLsuAddress);

  // Pre-fill destination register to check if it gets written.
  std::vector<uint32_t> init_data = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
                                     0xFFFFFFFF};
  SetVRegData(0, init_data);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/1, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int32_t>(1),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  uint32_t loaded_data = v_regs_[0]->data_buffer()->Get<uint32_t>()[0];
  EXPECT_NE(loaded_data, 0xFFFFFFFF);
}

TEST_F(CoralNPUV2InstructionTest, VlUnitStride_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kItcmAccessAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, Vlm_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kMaskTestData);
  SetupVectorState(0, /*vl=*/0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kItcmAccessAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // vl=16 -> ceil(16/8) = 2 bytes loaded.
  EXPECT_THAT(v_regs_[1]->data_buffer()->Get<uint8_t>().subspan(0, 2),
              ElementsAreArray(absl::MakeSpan(kMaskTestData).subspan(0, 2)));
}

TEST_F(CoralNPUV2InstructionTest, Vl1r_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetXRegValue(1, kItcmAccessAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, VlIndexed_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kItcmAccessAddress);
  std::vector<uint32_t> index_data = {0, 4, 8, 12};
  SetVRegData(1, index_data);

  std::unique_ptr<Instruction> inst =
      CreateVlIndexedInstruction(/*index_width=*/4, /*v_index=*/1,
                                 /*v_dest=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

TEST_F(CoralNPUV2InstructionTest, VlSegment_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kItcmAccessAddress);

  std::unique_ptr<Instruction> inst =
      CreateSegmentLoadInstruction(/*nf_value=*/1, /*element_width=*/4,
                                   /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
}

TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kItcmAccessAddress);

  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
}

// Verifies that indexed segment vector load from ITCM succeeds.
TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_ItcmAddress_Succeeds) {
  SetMemoryContents(kItcmAccessAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kItcmAccessAddress);
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*index_width=*/4,
      /*v_dest_start=*/0, /*v_index_start=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected_v0 = {kTestData[0], kTestData[2]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>().subspan(0, 2),
              ElementsAreArray(expected_v0));
}

// Verifies that unit-stride vector load succeeds at the end of ITCM.
TEST_F(CoralNPUV2InstructionTest, VlUnitStride_ItcmEndAddress_Succeeds) {
  const uint32_t kEndAddress = kItcmStartAddress + kItcmLength - 16;
  SetMemoryContents(kEndAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kEndAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int32_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(absl::MakeSpan(kTestData).subspan(0, 4)));
}

// Verifies that unit-stride vector load traps when there is no read permission.
TEST_F(CoralNPUV2InstructionTest, VlUnitStride_NoReadPermission_Traps) {
  const uint32_t kBaseAddress = 0x40000;
  state_->AddMemoryRegion(/*start_address=*/kBaseAddress, /*length=*/0x1000,
                          MemoryPermission::kWrite);

  SetupVectorState(0, 0);  // Vector type not used for vlm
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, kBaseAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlmInstruction(/*v_dest=*/1, /*element_width=*/1);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that scalar load traps when accessing write-only memory.
TEST_F(CoralNPUV2InstructionTest, Lw_WriteOnlyMemory_Traps) {
  const uint32_t kBaseAddress = 0x40000;
  state_->AddMemoryRegion(/*start_address=*/kBaseAddress, /*length=I*/ 0x1000,
                          MemoryPermission::kWrite);

  SetXRegValue(1, kBaseAddress);
  SetXRegValue(2, 0);

  std::unique_ptr<Instruction> inst =
      CreateLoadInstruction(::coralnpu::sim::CoralNPUV2Lw, [](Instruction*) {});
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that scalar store traps when accessing read-only memory.
TEST_F(CoralNPUV2InstructionTest, Sw_ReadOnlyMemory_Traps) {
  const uint32_t kBaseAddress = 0x40000;
  state_->AddMemoryRegion(/*start_address=*/kBaseAddress, /*length=*/0x1000,
                          MemoryPermission::kRead);

  SetXRegValue(1, kBaseAddress);
  SetXRegValue(2, 0);
  SetXRegValue(3, kTestWord);

  std::unique_ptr<Instruction> inst =
      CreateStoreInstruction(::coralnpu::sim::CoralNPUV2Sw);
  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

// Verifies that segment vector load traps when crossing a memory boundary.
TEST_F(CoralNPUV2InstructionTest, VlSegment_BoundaryAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  uint32_t boundary_address = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetXRegValue(1, boundary_address);

  // nf=1 means 2 fields of 4 bytes each. Total 8 bytes.
  // Access to [boundary_address, boundary_address + 8)
  // [0x17FFC, 0x18004) -> Should trap because 0x18000+ is invalid.
  std::unique_ptr<Instruction> inst =
      CreateSegmentLoadInstruction(/*nf_value=*/1, /*element_width=*/4,
                                   /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, boundary_address);
  EXPECT_EQ(exception_code_, ExceptionCode::kLoadAccessFault);
}

// Verifies that indexed vector load succeeds with multiple registers.
TEST_F(CoralNPUV2InstructionTest, VlIndexed_MultiRegister_Succeeds) {
  // SEW=32, vl=5. vlen=16 bytes.
  // indices: [0, 4, 8, 12, 16]
  // Register v16 (index) will contain [0, 4, 8, 12]
  // Register v17 (index) will contain [16, 0, 0, 0]
  SetupVectorState(0b11010001, /*vl=*/5);  // SEW=32, LMUL=2

  std::vector<uint32_t> indices = {0, 4, 8, 12, 16};
  SetVRegData<uint32_t>(16, std::vector<uint32_t>{0, 4, 8, 12});
  SetVRegData<uint32_t>(17, std::vector<uint32_t>{16, 0, 0, 0});

  SetXRegValue(1, kGoodLsuAddress);
  // Memory contents at kGoodLsuAddress + indices[i] should be something
  // distinct.
  for (int i = 0; i < 5; ++i) {
    SetMemoryContents(kGoodLsuAddress + indices[i],
                      static_cast<uint32_t>(100 + i));
  }

  std::unique_ptr<Instruction> inst =
      CreateVlIndexedInstruction(/*index_width=*/4, /*v_index=*/16,
                                 /*v_dest=*/8, /*mask_operand=*/nullptr,
                                 /*num_regs=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  std::vector<uint32_t> expected = {100, 101, 102, 103, 104};
  EXPECT_THAT(v_regs_[8]->data_buffer()->Get<uint32_t>().subspan(0, 4),
              ElementsAreArray(expected.begin(), expected.begin() + 4));
  EXPECT_THAT(v_regs_[9]->data_buffer()->Get<uint32_t>().subspan(0, 1),
              ElementsAreArray(expected.begin() + 4, expected.end()));
}

// Verifies that strided vector load succeeds with EMUL=64.
TEST_F(CoralNPUV2InstructionTest, VlStrided_Emul64_Succeeds) {
  // element_width=4, LMUL=8, SEW=32 bits (4 bytes)
  // multiplier = 64.
  // emul = 4 * 64 / 4 = 64.
  SetupVectorState(0b11010011, /*vl=*/1);  // SEW=32 bits (010), LMUL=8 (011)
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, 4);  // stride
  SetMemoryContents(kGoodLsuAddress, static_cast<uint32_t>(0x12345678));

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0, x_regs_[2]->CreateSourceOperand(),
      /*mask_operand=*/nullptr,
      /*num_regs=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
  EXPECT_EQ(v_regs_[0]->data_buffer()->Get<uint32_t>(0), 0x12345678);
}

// Verifies that whole register load traps when spanning a memory boundary.
TEST_F(CoralNPUV2InstructionTest, VlRegister_SpanningBoundary_Traps) {
  // vl1re32
  // LSU range is [0x10000, 0x18000).
  // End address is 0x18000. Last valid byte is 0x17fff.
  // 4 bytes element. Set address to 0x17ffd so it spans boundary.
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 3;
  SetXRegValue(1, kTestAddress);

  // Initialize destination register with zeros.
  std::vector<uint32_t> initial_v0(
      coralnpu::sim::kCoralNPUV2VectorByteLength / sizeof(uint32_t), 0);
  SetVRegData(0, initial_v0);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kTestAddress);

  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(), Each(0));
}

// Verifies that strided vector load traps when EMUL is too large.
TEST_F(CoralNPUV2InstructionTest, VlStrided_EmulTooLarge_Traps) {
  // element_width=4, LMUL=8, SEW=8 bits (1 byte)
  // emul = 4 * 8 / 1 = 32. This is OK.
  // Let's make it fail: element_width=8, LMUL=8, SEW=1 byte.
  // emul = 8 * 8 / 1 = 64. Still OK.
  // element_width=16, LMUL=8, SEW=1 byte -> emul = 128 (Too large)
  SetupVectorState(0b11000011, /*vl=*/1);  // SEW=8 bits (000), LMUL=8 (011)
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, 16);  // stride

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/16, /*v_dest=*/0, x_regs_[2]->CreateSourceOperand(),
      /*mask_operand=*/nullptr,
      /*num_regs=*/8);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kIllegalInstruction);
  EXPECT_EQ(trap_value_, 0);
}

// Verifies that whole register load succeeds at the exact memory boundary.
TEST_F(CoralNPUV2InstructionTest, VlRegister_ExactBoundary_Succeeds) {
  // vl1re32.
  // num_elements = 16 * 1 / 4 = 4.
  // Access range: [base, base + 16).
  // Boundary is 0x18000.
  const uint32_t kBaseAddress = kLsuAccessStartAddress + kLsuAccessLength - 16;
  SetXRegValue(1, kBaseAddress);

  std::unique_ptr<Instruction> inst =
      CreateVlRegisterInstruction(/*num_regs=*/1, /*element_width=*/4,
                                  /*v_dest_start=*/0);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

// Verifies that strided segment vector load succeeds at the exact memory
// boundary.
TEST_F(CoralNPUV2InstructionTest, VlSegmentStrided_ExactBoundary_Succeeds) {
  // vl=1, nf=0 (1 field), element_width=4, stride=4.
  // Access range: [base, base + 4).
  // Boundary is 0x18000.
  const uint32_t kBaseAddress = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, kBaseAddress);

  std::unique_ptr<Instruction> inst = CreateSegmentStridedLoadInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_dest_start=*/0, /*stride=*/4);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

// Verifies that indexed segment vector load succeeds at the exact memory
// boundary.
TEST_F(CoralNPUV2InstructionTest, VlSegmentIndexed_ExactBoundary_Succeeds) {
  // vl=1, nf=0 (1 field), element_width=4, index_width=4, index[0]=0.
  // Access range: [base, base + 4).
  // Boundary is 0x18000.
  const uint32_t kBaseAddress = kLsuAccessStartAddress + kLsuAccessLength - 4;
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, kBaseAddress);
  SetVRegData(1, std::vector<uint32_t>{0});

  std::unique_ptr<Instruction> inst = CreateSegmentIndexedLoadInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_dest_start=*/0, /*v_index_start=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Vse8_ValidAddress_Succeeds) {
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 4));

  std::unique_ptr<Instruction> inst = CreateStridedVectorStoreInstruction(
      /*element_width=*/4, /*v_src=*/1,
      /*stride_operand=*/new ImmediateOperand<int64_t>(4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  for (int i = 0; i < 4; ++i) {
    uint32_t address = kGoodLsuAddress + i * 4;
    EXPECT_EQ(GetMemoryContents<uint32_t>(address), kTestData[i])
        << " memory mismatch at address: " << address
        << " expected: " << kTestData[i]
        << " actual: " << GetMemoryContents<uint32_t>(address);
  }
}

TEST_F(CoralNPUV2InstructionTest, Vse8_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 4));

  std::unique_ptr<Instruction> inst = CreateStridedVectorStoreInstruction(
      /*element_width=*/1, /*v_src=*/1,
      /*stride_operand=*/new ImmediateOperand<int64_t>(1),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsSegment_ValidAddress_Succeeds) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 4), kTestData[1]);
}

TEST_F(CoralNPUV2InstructionTest, VsSegment_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsSegmentStrided_ValidAddress_Succeeds) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst = CreateVsSegmentStridedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1,
      /*stride_operand=*/new ImmediateOperand<int64_t>(8));

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 8), kTestData[1]);
}

TEST_F(CoralNPUV2InstructionTest, VsSegmentStrided_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst = CreateVsSegmentStridedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1,
      /*stride_operand=*/new ImmediateOperand<int64_t>(8));

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsSegmentIndexed_ValidAddress_Succeeds) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateVsSegmentIndexedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_src_start=*/1, /*v_index_start=*/2);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 8), kTestData[1]);
}

TEST_F(CoralNPUV2InstructionTest, VsSegmentIndexed_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateVsSegmentIndexedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_src_start=*/1, /*v_index_start=*/2);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsRegister_ValidAddress_Succeeds) {
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst =
      CreateVsRegisterInstruction(/*num_regs=*/1, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // VsRegister stores 8-byte elements (uint64_t).
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 4), kTestData[1]);
}

TEST_F(CoralNPUV2InstructionTest, VsRegister_InvalidAddress_Traps) {
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));

  std::unique_ptr<Instruction> inst =
      CreateVsRegisterInstruction(/*num_regs=*/1, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsIndexed_InvalidAddress_Traps) {
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kBadLsuAddress);
  SetVRegData(1, absl::MakeSpan(kTestData).subspan(0, 2));
  std::vector<uint32_t> index_data = {0, 8};
  SetVRegData(2, index_data);

  std::unique_ptr<Instruction> inst = CreateVsIndexedInstruction(
      /*index_width=*/4, /*v_index=*/2, /*v_src=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kBadLsuAddress);
  EXPECT_EQ(exception_code_, ExceptionCode::kStoreAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, Vsm_ValidAddress_Succeeds) {
  std::vector<uint8_t> test_data = {0x55};
  rv_vector_->set_vector_length(8);
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData(1, test_data);

  std::unique_ptr<Instruction> inst = CreateVsmInstruction(/*v_src=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
  EXPECT_EQ(GetMemoryContents<uint8_t>(kGoodLsuAddress), 0x55);
}

TEST_F(CoralNPUV2InstructionTest, VsStrided_Emul1_Succeeds) {
  // SEW=32 bits (4 bytes), LMUL=1/8.
  // emul should be 1.
  SetupVectorState(0b11010101, /*vl=*/1);  // SEW=32 bits (010), LMUL=1/8 (101)
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, 4);  // stride

  std::unique_ptr<Instruction> inst = CreateStridedVectorStoreInstruction(
      /*element_width=*/4, /*v_src=*/0, x_regs_[2]->CreateSourceOperand(),
      /*mask_operand=*/nullptr);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, VsStrided_EmulTooLarge_Traps) {
  // element_width=16, LMUL=8, SEW=8 bits (1 byte) -> emul = 128 (Too large)
  SetupVectorState(0b11000011, /*vl=*/1);  // SEW=8 bits (000), LMUL=8 (011)
  SetXRegValue(1, kGoodLsuAddress);
  SetXRegValue(2, 16);  // stride

  std::unique_ptr<Instruction> inst = CreateStridedVectorStoreInstruction(
      /*element_width=*/16, /*v_src=*/0, x_regs_[2]->CreateSourceOperand(),
      /*mask_operand=*/nullptr);
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kIllegalInstruction);
}

TEST_F(CoralNPUV2InstructionTest, Vsm_AtMemoryBoundary_Succeeds) {
  // LSU range is [0x10000, 0x18000).
  // End address is 0x18000. Last valid byte is 0x17fff.
  // vl=8 -> num_bytes = 1.
  const uint32_t kTestAddress = kLsuAccessStartAddress + kLsuAccessLength - 1;
  rv_vector_->set_vector_length(8);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst = CreateVsmInstruction(/*v_src=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Vsm_AddressAtStart_Succeeds) {
  // LSU range is [0x10000, 0x18000).
  const uint32_t kTestAddress = kLsuAccessStartAddress;
  rv_vector_->set_vector_length(8);
  SetXRegValue(1, kTestAddress);

  std::unique_ptr<Instruction> inst = CreateVsmInstruction(/*v_src=*/1);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, Jal_NoExecutePermission_Traps) {
  const uint32_t kTargetAddress = 0x50000;
  state_->AddMemoryRegion(/*start_address=*/kTargetAddress, /*length=*/0x1000,
                          coralnpu::sim::MemoryPermission::kRead);

  std::unique_ptr<Instruction> inst = CreateJalInstruction(kTargetAddress - 0);
  inst->set_address(0);

  inst->Execute(/*context=*/nullptr);

  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(exception_code_, ExceptionCode::kInstructionAccessFault);
}

TEST_F(CoralNPUV2InstructionTest, VsIndexed_MultiRegisterIndices_Succeeds) {
  // SEW=32, vl=5. vlen=16 bytes.
  // index_width = 4 (32-bit). indices_per_reg = 16 / 4 = 4.
  // vl=5 means we use 2 registers for indices.
  SetupVectorState(0b11010001, /*vl=*/5);  // SEW=32, LMUL=2

  // Indices: {0, 4, 8, 12, 16}.
  // Index i=0,1,2,3 are in V16.
  // Index i=4 is in V17.
  SetVRegData<uint32_t>(16, std::vector<uint32_t>{0, 4, 8, 12});
  SetVRegData<uint32_t>(17, std::vector<uint32_t>{16, 0, 0, 0});

  SetXRegValue(1, kGoodLsuAddress);
  // Source data: v8[0..3]=kTestData[0..3], v9[0]=kTestData[4].
  SetVRegData<uint32_t>(8, absl::MakeSpan(kTestData).subspan(0, 4));
  SetVRegData<uint32_t>(9, std::vector<uint32_t>{kTestData[4], 0, 0, 0});

  std::unique_ptr<Instruction> inst = CreateVsIndexedInstruction(
      /*index_width=*/4, /*v_index=*/16, /*v_src=*/8, /*mask_operand=*/nullptr,
      /*num_regs=*/2);
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 0), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 4), kTestData[1]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 8), kTestData[2]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 12), kTestData[3]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 16), kTestData[4]);
}

TEST_F(CoralNPUV2InstructionTest, VsSegment_Nf1_Succeeds) {
  // nf=1 (2 fields), vl=2, element_width=4.
  // segment_byte_size = 8.
  // i=0: [base, base+7]. Fields: v8[0] at base, v9[0] at base+4.
  // i=1: [base+8, base+15]. Fields: v8[1] at base+8, v9[1] at base+12.
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress);
  SetVRegData<uint32_t>(8, absl::MakeSpan(kTestData).subspan(0, 2));
  SetVRegData<uint32_t>(9, absl::MakeSpan(kTestData).subspan(2, 2));

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_src_start=*/8);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 0), kTestData[0]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 4), kTestData[2]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 8), kTestData[1]);
  EXPECT_EQ(GetMemoryContents<uint32_t>(kGoodLsuAddress + 12), kTestData[3]);
}

TEST_F(CoralNPUV2InstructionTest, VsSegment_LastByteAtBoundary_Succeeds) {
  // SEW=32, vl=1, nf=0 (1 field). segment_byte_size = 4.
  // Last valid byte is 0x17fff.
  // Set base to 0x17ffc. Range [0x17ffc, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 4;
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, base);
  SetVRegData<uint32_t>(8, std::vector<uint32_t>{100});

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/8);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
  EXPECT_EQ(GetMemoryContents<uint32_t>(base), 100);
}

// Verifies that whole register store succeeds at the exact memory boundary.
TEST_F(CoralNPUV2InstructionTest, VsRegister_AtExactBoundary_Succeeds) {
  // VsRegister with num_regs=1 stores 2 elements of 8 bytes = 16 bytes.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 16;
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst =
      CreateVsRegisterInstruction(/*num_regs=*/1, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

// Verifies that indexed vector store traps when indices_per_reg is incorrectly
// calculated. This catches a mutant where indices_per_reg is (vlen/width) - 1.
TEST_F(CoralNPUV2InstructionTest, VsIndexed_IndicesPerReg_Traps) {
  // SEW=32, vl=4. vlen=16 bytes.
  // index_width = 4 (32-bit). indices_per_reg = 16 / 4 = 4.
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  uint32_t base = kGoodLsuAddress;
  SetXRegValue(1, base);

  // Set V16[3] to an invalid address.
  // V16[0..2] are valid, V16[3] is invalid.
  uint32_t invalid_addr = kBadLsuAddress;
  std::vector<uint32_t> index_data = {0, 4, 8, invalid_addr - base};
  SetVRegData<uint32_t>(16, index_data);

  // If indices_per_reg is 4: i=3 uses V16[3] -> invalid_addr -> TRAPS.
  // If indices_per_reg is 3: i=3 uses V17[0]. If V17[0] is valid -> SUCCEEDS.
  SetVRegData<uint32_t>(17, std::vector<uint32_t>{0, 0, 0, 0});

  std::unique_ptr<Instruction> inst = CreateVsIndexedInstruction(
      /*index_width=*/4, /*v_index=*/16, /*v_src=*/8);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, invalid_addr);
}

// Verifies that segment vector store traps when num_fields is incorrectly
// calculated. This catches a mutant where num_fields is nf instead of nf + 1.
TEST_F(CoralNPUV2InstructionTest, VsSegment_NumFields_Traps) {
  // nf=1 (2 fields), vl=1, element_width=4.
  // num_fields should be 2. segment_byte_size = 8.
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 4;
  SetXRegValue(1, base);

  // Field 0: [base, base+4) -> [boundary-4, boundary) (Valid).
  // Field 1: [base+4, base+8) -> [boundary, boundary+4) (Invalid).
  // Original (num_fields=2) should TRAP.
  // Mutant (num_fields=1) would SUCCEED.

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/1, /*element_width=*/4, /*v_src_start=*/8);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, base);
}

// Verifies that Vsm traps when the last mask byte is invalid.
// This catches a mutant where num_bytes is decremented by 1.
TEST_F(CoralNPUV2InstructionTest, Vsm_LastByteInvalid_Traps) {
  // vl=16 -> num_bytes = 2.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 1;
  // Byte 0: base (0x17FFF) - VALID.
  // Byte 1: base + 1 (0x18000) - INVALID.
  rv_vector_->set_vector_length(16);
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst = CreateVsmInstruction(/*v_src=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, boundary);
}

// Verifies that VsSegment traps when the segment byte size is too small.
// This catches a mutant where segment_byte_size is decremented by 1.
TEST_F(CoralNPUV2InstructionTest, VsSegment_SegmentByteSize_Traps) {
  // nf=0 (1 field), element_width=4. segment_byte_size = 4.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 3;
  // [base, base+4) -> [boundary-3, boundary+1) -> TRAPS at 0x18000.
  // Mutant (size=3) would check [boundary-3, boundary) -> SUCCEEDS.
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst = CreateVsSegmentInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, base);
}

// Verifies that VsSegmentStrided succeeds with correct number of fields.
// This catches a mutant where num_fields is incremented by 1.
TEST_F(CoralNPUV2InstructionTest, VsSegmentStrided_NumFields_Succeeds) {
  // nf=0 (1 field), vl=1, element_width=4, stride=4.
  // num_fields should be 1.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 4;
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, base);

  // Field 0: [base, base+4) -> [boundary-4, boundary) (Valid).
  // Mutant (num_fields=2) would check field 1: [base+4, base+8) -> TRAPS.
  std::unique_ptr<Instruction> inst = CreateVsSegmentStridedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*v_src_start=*/1,
      /*stride_operand=*/new ImmediateOperand<int64_t>(4));

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

// Verifies that VsRegister traps when the last element is invalid.
// This catches a mutant where num_elements is decremented by 1.
TEST_F(CoralNPUV2InstructionTest, VsRegister_LastElementInvalid_Traps) {
  // VsRegister with num_regs=1 stores 2 elements of 8 bytes = 16 bytes.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 8;
  // Element 0: [base, base+8) -> [boundary-8, boundary) (Valid).
  // Element 1: [base+8, base+16) -> [boundary, boundary+8) (Invalid).
  SetXRegValue(1, base);

  std::unique_ptr<Instruction> inst =
      CreateVsRegisterInstruction(/*num_regs=*/1, /*v_src_start=*/1);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, boundary);
}

// Verifies that VsSegmentIndexed traps when indices_per_reg is incorrectly
// calculated. This catches a mutant where indices_per_reg is incremented by 1.
TEST_F(CoralNPUV2InstructionTest, VsSegmentIndexed_IndicesPerReg_Traps) {
  // SEW=32, vl=5. vlen=16 bytes.
  // index_width = 4 (32-bit). indices_per_reg = 16 / 4 = 4.
  SetupVectorState(0b11'010'000, /*vl=*/5);  // SEW=32, LMUL=1
  uint32_t base = kGoodLsuAddress;
  SetXRegValue(1, base);

  // V16[0..3] are valid indices.
  SetVRegData<uint32_t>(16, std::vector<uint32_t>{0, 4, 8, 12});
  // V17[0] is an invalid index for i=4.
  uint32_t invalid_addr = kBadLsuAddress;
  SetVRegData<uint32_t>(17,
                        std::vector<uint32_t>{invalid_addr - base, 0, 0, 0});

  // If indices_per_reg is 4: i=4 uses V17[0] -> invalid_addr -> TRAPS.
  // If indices_per_reg is 5: i=4 uses V16[4] -> out of bounds.
  std::unique_ptr<Instruction> inst = CreateVsSegmentIndexedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_src_start=*/8, /*v_index_start=*/16, /*mask_operand=*/nullptr,
      /*num_index_regs=*/2);

  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, invalid_addr);
}

// Verifies that VsSegmentIndexed succeeds with correct number of fields.
// This catches a mutant where num_fields is incremented by 1.
TEST_F(CoralNPUV2InstructionTest, VsSegmentIndexed_NumFields_Succeeds) {
  // nf=0 (1 field), vl=1, element_width=4, index_width=4.
  // num_fields should be 1.
  // LSU range is [0x10000, 0x18000).
  uint32_t boundary = kLsuAccessStartAddress + kLsuAccessLength;
  uint32_t base = boundary - 4;
  SetupVectorState(0b11'010'000, /*vl=*/1);  // SEW=32, LMUL=1
  SetXRegValue(1, base);
  SetVRegData(16, std::vector<uint32_t>{0});

  // Field 0: [base, base+4) -> [boundary-4, boundary) (Valid).
  // Mutant (num_fields=2) would check field 1: [base+4, base+8) -> TRAPS.
  std::unique_ptr<Instruction> inst = CreateVsSegmentIndexedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_src_start=*/8, /*v_index_start=*/16, /*mask_operand=*/nullptr);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

// Verifies that VsSegmentIndexed succeeds when indices_per_reg is correctly
// calculated. This catches a mutant where indices_per_reg is decremented by 1.
TEST_F(CoralNPUV2InstructionTest,
       VsSegmentIndexed_IndicesPerReg_Decrement_Succeeds) {
  // SEW=32, vl=4. vlen=16 bytes.
  // index_width = 4 (32-bit). indices_per_reg = 16 / 4 = 4.
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  uint32_t base = kGoodLsuAddress;
  SetXRegValue(1, base);

  // V16[0..3] are valid indices.
  SetVRegData<uint32_t>(16, std::vector<uint32_t>{0, 4, 8, 12});
  // V17[0] is an invalid index for the mutant (which incorrectly uses reg_idx=1
  // for i=3).
  uint32_t invalid_addr = kBadLsuAddress;
  SetVRegData<uint32_t>(17,
                        std::vector<uint32_t>{invalid_addr - base, 0, 0, 0});

  // If indices_per_reg is 4: i=3 uses V16[3] -> valid -> SUCCEEDS.
  // If indices_per_reg is 3: i=3 uses V17[0] -> invalid_addr -> TRAPS.
  std::unique_ptr<Instruction> inst = CreateVsSegmentIndexedInstruction(
      /*nf_value=*/0, /*element_width=*/4, /*index_width=*/4,
      /*v_src_start=*/8, /*v_index_start=*/16, /*mask_operand=*/nullptr,
      /*num_index_regs=*/2);

  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);
}

TEST_F(CoralNPUV2InstructionTest, VlStrided_NegativeStride_Succeeds) {
  // 4 elements, SEW=32, stride=-4.
  // base = GoodLsuAddress + 12.
  // addresses: base(12), base-4(8), base-8(4), base-12(0).
  SetMemoryContents(kGoodLsuAddress, kTestData);
  SetupVectorState(0b11'010'000, /*vl=*/4);  // SEW=32, LMUL=1
  SetXRegValue(1, kGoodLsuAddress + 12);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(-4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_FALSE(was_trap_handler_called_);

  // Addresses: +12, +8, +4, +0
  // Values: kTestData[3], [2], [1], [0]
  std::vector<uint32_t> expected = {kTestData[3], kTestData[2], kTestData[1],
                                    kTestData[0]};
  EXPECT_THAT(v_regs_[0]->data_buffer()->Get<uint32_t>(),
              ElementsAreArray(expected));
}

TEST_F(CoralNPUV2InstructionTest, VlStrided_NegativeStride_TrapsAtBoundary) {
  // 2 elements, SEW=32, stride=-4.
  // base = kLsuAccessStartAddress (0x10000).
  // i=0: 0x10000 (Valid)
  // i=1: 0x10000 - 4 = 0x0FFFC (Invalid)
  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, kLsuAccessStartAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(-4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, kLsuAccessStartAddress - 4);
}

TEST_F(CoralNPUV2InstructionTest,
       VlStrided_PositiveStride_64BitWrapAround_Traps) {
  // 3 elements, SEW=32, stride=0x80000000.
  // base = kLsuAccessStartAddress (0x10000).
  // i=0: 0x10000 (Valid)
  // i=1: 0x80010000 (Invalid - should trap!)
  // i=2: 0x10000 (Valid)
  SetupVectorState(0b11'010'000, /*vl=*/3);  // SEW=32, LMUL=1
  SetXRegValue(1, kLsuAccessStartAddress);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(0x80000000LL),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);

  // It MUST trap on the second element (0x80010000)
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, 0x80010000);
}

TEST_F(CoralNPUV2InstructionTest,
       VlStrided_NegativeStride_FastPathMutant_Traps) {
  // We want to test the span calculation.
  // stride = -4, element_size = 4, count = 2.
  // base = 0x20004.
  // Addresses accessed: 0x20004, 0x20000.
  // Original fast path checks [0x20000, 0x20008).
  // Mutant (span + 1) checks [0x20001, 0x20008).
  // If we map exactly [0x20001, 0x20008), the mutant fast path will succeed!
  // But the real access 0x20000 is unmapped, so it should trap.
  // If the mutant bypasses the trap, this test will fail, killing the mutant.
  state_->AddMemoryRegion(/*start_address=*/0x20001, /*length=*/0x1000,
                          MemoryPermission::kReadWrite);

  SetupVectorState(0b11'010'000, /*vl=*/2);  // SEW=32, LMUL=1
  SetXRegValue(1, 0x20004);

  std::unique_ptr<Instruction> inst = CreateStridedVectorLoadInstruction(
      /*element_width=*/4, /*v_dest=*/0,
      /*stride_operand=*/new ImmediateOperand<int64_t>(-4),
      /*mask_operand=*/new RV32VectorTrueOperand(state_.get()));
  inst->Execute(/*context=*/nullptr);

  // It MUST trap on the second element (0x20000) because 0x20000 is not mapped.
  EXPECT_TRUE(was_trap_handler_called_);
  EXPECT_EQ(trap_value_, 0x20000);
}

}  // namespace
