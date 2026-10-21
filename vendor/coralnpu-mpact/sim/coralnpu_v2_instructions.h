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

#ifndef SIM_CORALNPU_V2_INSTRUCTIONS_H_
#define SIM_CORALNPU_V2_INSTRUCTIONS_H_

#include "absl/base/nullability.h"
#include "riscv/riscv_state.h"
#include "mpact/sim/generic/instruction.h"

namespace coralnpu::sim {

using ::mpact::sim::generic::Instruction;
using ::mpact::sim::riscv::ExceptionCode;

// Semantic function for the mpause instruction. This instruction is used to
// halt execution.
// No source operands.
// No destination operands.
void CoralNPUV2Mpause(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the load word (lw) instruction. If the address access
// is valid, the data is loaded into the destination register. Otherwise, a
// trap is triggered.
// Source operand 1: scalar register containing the base address to load.
// Source operand 2: immediate operand containing the offset to load.
// Destination operand 1: scalar register to store the loaded data.
void CoralNPUV2Lw(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the load half word (lh) instruction. If the address
// access is valid, the data is loaded into the destination register. Otherwise,
// a trap is triggered.
// Source operand 1: scalar register containing the base address to load.
// Source operand 2: immediate operand containing the offset to load.
// Destination operand 1: scalar register to store the sign extended data.
void CoralNPUV2Lh(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the load unsigned half word (lhu) instruction. If the
// address access is valid, the data is loaded into the destination register.
// Otherwise, a trap is triggered.
// Source operand 1: scalar register containing the base address to load.
// Source operand 2: immediate operand containing the offset to load.
// Destination operand 1: scalar register to store the unsigned data.
void CoralNPUV2Lhu(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the load byte (lb) instruction. If the address access
// is valid, the data is loaded into the destination register. Otherwise, a
// trap is triggered.
// Source operand 1: scalar register containing the base address to load.
// Source operand 2: immediate operand containing the offset to load.
// Destination operand 1: scalar register to store the sign extended data.
void CoralNPUV2Lb(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the load unsigned byte (lbu) instruction. If the
// address access is valid, the data is loaded into the destination register.
// Otherwise, a trap is triggered.
// Source operand 1: scalar register containing the base address to load.
// Source operand 2: immediate operand containing the offset to load.
// Destination operand 1: scalar register to store the unsigned data.
void CoralNPUV2Lbu(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the store word (sw) instruction. If the address access
// is valid, the data is stored to the specified address. Otherwise, a trap is
// triggered.
// Source operand 1: scalar register containing the base address to store.
// Source operand 2: immediate operand containing the offset to store.
// Source operand 3: scalar register containing the data to store.
void CoralNPUV2Sw(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the store half word (sh) instruction. If the address
// access is valid, the data is stored to the specified address. Otherwise, a
// trap is triggered.
// Source operand 1: scalar register containing the base address to store.
// Source operand 2: immediate operand containing the offset to store.
// Source operand 3: scalar register containing the data to store. The lower
// 16 bits of this register are used to store.
void CoralNPUV2Sh(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the store byte (sb) instruction. If the address access
// is valid, the data is stored to the specified address. Otherwise, a trap is
// triggered.
// Source operand 1: scalar register containing the base address to store.
// Source operand 2: immediate operand containing the offset to store.
// Source operand 3: scalar register containing the data to store. The lower 8
// bits of this register are used to store.
void CoralNPUV2Sb(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for the float store word (fsw) instruction. If the address
// access is valid, the data is stored to the specified address. Otherwise, a
// trap is triggered.
// Source operand 1: scalar register containing the base address to store.
// Source operand 2: immediate operand containing the offset to store.
// Source operand 3: float register containing the data to store.
void CoralNPUV2Fsw(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for jump and link (jal) instruction.
// If the address access is valid, the control is transferred to the target
// address. Otherwise, a trap is triggered.
// J-type instruction, offsets PC by a signed 21-bit immediate.
// Destination operand 1: scalar register containing the return address.
void CoralNPUV2Jal(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for jump and link register (jalr) instruction.
// If the address access is valid, the control is transferred to the target
// address. Otherwise, a trap is triggered.
// I-type instruction, rs1 + imm12
// Destination operand 1: scalar register containing the return address.
void CoralNPUV2Jalr(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for strided vector loads (vle8.v, vlse8.v, vle8ff.v, etc).
// Validates all computed element addresses against valid memory ranges before
// performing the load. If an invalid access is detected, it triggers a trap.
// Source operand 0: base address.
// Source operand 1: stride (in bytes).
// Source operand 2: vector mask.
void CoralNPUV2VlStrided(int element_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector mask loads (vlm.v).
// Validates all computed byte addresses against valid memory ranges before
// performing the load. If an invalid access is detected, it triggers a trap.
// Source operand 0: base address.
void CoralNPUV2Vlm(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector register loads (vl1re8.v, vl2re16.v, etc).
// Validates all computed byte addresses against valid memory ranges before
// performing the load. If an invalid access is detected, it triggers a trap.
// Source operand 0: base address.
void CoralNPUV2VlRegister(int num_regs, int element_width_bytes,
                          const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector indexed loads (vluxei8.v, vluxei16.v, etc).
// Validates all computed element addresses against valid memory ranges before
// performing the load. If an invalid access is detected, it triggers a trap.
// Source operand 0: base address.
// Source operand 1: index vector operand.
// Source operand 2: vector mask.
void CoralNPUV2VlIndexed(int index_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for unit-stride vector segment loads (vlseg2e8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the load. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: base address.
// Source operand 1: vector mask.
// Source operand 2: number of fields - 1 (nf).
void CoralNPUV2VlSegment(int element_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for strided vector segment loads (vlsseg2e8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the load. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: base address.
// Source operand 1: segment stride (in bytes).
// Source operand 2: vector mask.
// Source operand 3: number of fields - 1 (nf).
void CoralNPUV2VlSegmentStrided(int element_width,
                                const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for indexed vector segment loads (vluxseg2ei8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the load. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: base address.
// Source operand 1: index vector operand.
// Source operand 2: vector mask.
// Source operand 3: number of fields - 1 (nf).
void CoralNPUV2VlSegmentIndexed(int index_width,
                                const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector mask stores (vsm.v).
// Validates all computed byte addresses against valid memory ranges before
// performing the store. If an invalid access is detected, it triggers a trap.
// Source operand 0: vector mask source.
// Source operand 1: base address.
void CoralNPUV2Vsm(const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for strided vector stores (vse8.v, vsse8.v, etc).
// Validates all computed element addresses against valid memory ranges before
// performing the store. If an invalid access is detected, it triggers a trap.
// Source operand 0: vector source.
// Source operand 1: base address.
// Source operand 2: stride (in bytes).
// Source operand 3: vector mask.
void CoralNPUV2VsStrided(int element_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector register stores (vs1re8.v, vs2re16.v, etc).
// Validates all computed byte addresses against valid memory ranges before
// performing the store. If an invalid access is detected, it triggers a trap.
// Source operand 0: vector register source.
// Source operand 1: base address.
void CoralNPUV2VsRegister(int num_regs,
                          const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for vector indexed stores (vsuxei8.v, vsoxei16.v, etc).
// Validates all computed element addresses against valid memory ranges before
// performing the store. If an invalid access is detected, it triggers a trap.
// Source operand 0: vector source.
// Source operand 1: base address.
// Source operand 2: index vector operand.
// Source operand 3: vector mask.
void CoralNPUV2VsIndexed(int index_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for unit-stride vector segment stores (vsseg2e8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the store. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: vector source.
// Source operand 1: base address.
// Source operand 2: vector mask.
// Source operand 3: number of fields - 1 (nf).
void CoralNPUV2VsSegment(int element_width,
                         const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for strided vector segment stores (vssseg2e8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the store. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: vector source.
// Source operand 1: base address.
// Source operand 2: segment stride (in bytes).
// Source operand 3: vector mask.
// Source operand 4: number of fields - 1 (nf).
void CoralNPUV2VsSegmentStrided(int element_width,
                                const Instruction* /*absl_nonnull*/ instruction);

// Semantic function for indexed vector segment stores (vsuxseg2ei8.v, etc).
// Validates all computed element addresses across all segments against valid
// memory ranges before performing the store. If an invalid access is detected,
// it triggers a trap.
// Source operand 0: vector source.
// Source operand 1: base address.
// Source operand 2: index vector operand.
// Source operand 3: vector mask.
// Source operand 4: number of fields - 1 (nf).
void CoralNPUV2VsSegmentIndexed(int index_width,
                                const Instruction* /*absl_nonnull*/ instruction);

}  // namespace coralnpu::sim

#endif  // SIM_CORALNPU_V2_INSTRUCTIONS_H_
