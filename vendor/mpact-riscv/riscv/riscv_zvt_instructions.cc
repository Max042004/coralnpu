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

#include "riscv/riscv_zvt_instructions.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "absl/base/casts.h"
#include "absl/log/check.h"
#include "absl/types/span.h"
#include "mpact/sim/generic/data_buffer.h"
#include "mpact/sim/generic/instruction.h"
#include "riscv/riscv_register.h"
#include "riscv/riscv_state.h"
#include "riscv/riscv_vector_state.h"
#include "riscv/riscv_zvt_state.h"

namespace mpact::sim::riscv {

using ::mpact::sim::generic::DataBuffer;
using ::mpact::sim::generic::GetInstructionSource;
using ::mpact::sim::generic::operator*;  // NOLINT
using ::mpact::sim::riscv::RiscVVectorState;
using ::mpact::sim::riscv::RV32VectorDestinationOperand;
using ::mpact::sim::riscv::RV32VectorSourceOperand;

namespace {

// ELEN for the Zve32{x,f} kBase.
constexpr int kElen = 32;

// Decoded kTile subset specifier (TSS). Spec 15.1.1.5.
struct Tss {
  int kTile;       // bits[30:27]
  int pattern;     // bits[26:24]: 0 = row, 1 = col.
  uint32_t index;  // bits[23:0].
};

Tss DecodeTss(uint32_t tss) {
  return Tss{
      .kTile = static_cast<int>((tss >> 27) & 0xF),
      .pattern = static_cast<int>((tss >> 24) & 0x7),
      .index = tss & 0xFFFFFF,
  };
}

constexpr int kTssPatternCol = 1;  // pattern 0 = row (the default branch).

// Reinterprets the low 16 bits of a bf16 value as the high half of an fp32.
float Bf16ToF32(uint16_t bits) {
  uint32_t u = static_cast<uint32_t>(bits) << 16;
  return absl::bit_cast<float>(u);
}

// Reads kTile element (row,col) of `kTile` at width `tile_element_width` as a
// generic 64-bit container, and the inverse. Used by the move/load/store paths
// which carry the element width dynamically.
uint64_t LoadTileBits(RiscVZvtMatrixState* matrix_state, int kTile, int row,
                      int col, int tile_element_width) {
  const int kEffectiveTileEdge = tile_element_width < 64
                                     ? matrix_state->tile_elements()
                                     : matrix_state->tile_elements() / 2;
  CHECK(row >= 0 && row < kEffectiveTileEdge) << "row out of bounds";
  CHECK(col >= 0 && col < kEffectiveTileEdge) << "col out of bounds";
  switch (tile_element_width) {
    case 8:
      return matrix_state->GetElem<uint8_t>(kTile, row, col, 8);
    case 16:
      return matrix_state->GetElem<uint16_t>(kTile, row, col, 16);
    case 32:
      return matrix_state->GetElem<uint32_t>(kTile, row, col, 32);
    case 64:
      return matrix_state->GetElem<uint64_t>(kTile, row, col, 64);
    default:
      return 0;
  }
}

void TrapIllegalInstruction(RiscVState* state, const Instruction* inst) {
  uint64_t address = inst->address();
  auto* db = state->db_factory()->Allocate<uint32_t>(1);
  state->LoadMemory(inst, address, db, nullptr, nullptr);
  uint32_t inst_word = db->Get<uint32_t>(0);
  db->DecRef();
  if ((inst_word & 0b11) != 0b11) inst_word &= 0xffff;
  state->Trap(/*is_interrupt=*/false, /*trap_value=*/inst_word,
              *ExceptionCode::kIllegalInstruction,
              /*epc=*/address, inst);
}

void StoreTileBits(RiscVZvtMatrixState* matrix_state, int kTile, int row,
                   int col, int tile_element_width, uint64_t value) {
  matrix_state->MarkDirty();
  const int kEffectiveTileEdge = tile_element_width < 64
                                     ? matrix_state->tile_elements()
                                     : matrix_state->tile_elements() / 2;
  CHECK(row >= 0 && row < kEffectiveTileEdge) << "row out of bounds";
  CHECK(col >= 0 && col < kEffectiveTileEdge) << "col out of bounds";
  switch (tile_element_width) {
    case 8:
      matrix_state->SetElem<uint8_t>(kTile, row, col, 8,
                                     static_cast<uint8_t>(value));
      break;
    case 16:
      matrix_state->SetElem<uint16_t>(kTile, row, col, 16,
                                      static_cast<uint16_t>(value));
      break;
    case 32:
      matrix_state->SetElem<uint32_t>(kTile, row, col, 32,
                                      static_cast<uint32_t>(value));
      break;
    case 64:
      matrix_state->SetElem<uint64_t>(kTile, row, col, 64, value);
      break;
  }
}

// Writes a 32-bit value to the scalar destination operand at `dest_index`.
void WriteScalar(const Instruction* inst, int dest_index, uint32_t value) {
  auto* dest = inst->Destination(dest_index);
  DataBuffer* db = dest->AllocateDataBuffer();
  db->Set<uint32_t>(0, value);
  db->Submit();
}

}  // namespace

void RiscVZvtMsetMtype(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  const uint32_t kRs1 = GetInstructionSource<uint32_t>(inst, 0);  // rs1
  const uint32_t kRs2 = GetInstructionSource<uint32_t>(inst, 1);  // rs2

  // Spec 15.1.1.4: vl = 0; mtype = rs1; then vtype = rs2 as if by vsetvl.
  vector_state->set_vector_length(0);
  matrix_state->set_mtype(kRs1);
  if (matrix_state->mtwiden() == 0) {
    matrix_state->set_mtype(0);
    vector_state->SetVectorType(kRs2 & ~0x100u);
    return;
  }

  // If the matrix unit is configured, force vma/vta = 1 and derive vlmul per
  // Zvt spec §15.1.1.4: LMUL = min(8/KMAX, 8/TWIDEN, ceil(ETE/EVE)).
  // For VLEN=128, TE=16: SEW8 -> LMUL1 (0), SEW16 -> LMUL2 (1), SEW32 -> LMUL4 (2).
  const uint32_t kVsew = (kRs2 >> 3) & 0x7;
  uint32_t vlmul = 4;  // Reserved if SEW is unsupported.
  switch (kVsew) {
    case 0: vlmul = 0; break;  // SEW8 -> LMUL1
    case 1: vlmul = 1; break;  // SEW16 -> LMUL2
    case 2: vlmul = 2; break;  // SEW32 -> LMUL4
    default: vlmul = 4; break;
  }
  uint32_t new_vtype =
      (kRs2 & ~0b1100'0111u) | (1u << 7) | (1u << 6) | vlmul;
  vector_state->SetVectorType(new_vtype);
  if (vector_state->is_vill()) {
    matrix_state->set_mtype(0);
    return;
  }

  const int kSewBits = vector_state->selected_element_width() * 8;
  const int kTwiden = matrix_state->twiden();
  const int kTileElementWidth = kSewBits * kTwiden;
  const int kEffectiveTileElements = kTileElementWidth < 64
                                         ? matrix_state->tile_elements()
                                         : matrix_state->tile_elements() / 2;
  const int kmax = RiscVZvtKmax(kSewBits);
  const int kLmulElements =
      vector_state->max_vector_length();  // LMUL * (VLEN/SEW).

  // kVill if the resulting TILE_ELEMENT_WIDTH is unsupported for the kBase
  // (SEW*TWIDEN > ELEN).
  const bool kVill = kTileElementWidth > vector_state->max_elen();
  if (kVill) {
    vector_state->SetVectorType(kVillBitMask);
    matrix_state->set_mtype(0);
    return;
  }

  // kTileK = min(kTileK, KMAX); kTileM = min(kTileM, LMUL*EVE,
  // EFFECTIVE_TILE_EDGE).
  matrix_state->set_tile_k(std::min(matrix_state->tile_k(), kmax));
  matrix_state->set_tile_m(std::min(
      {matrix_state->tile_m(), kLmulElements, kEffectiveTileElements}));
}

void RiscVZvtMsetTn(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  const uint32_t kRs1 = GetInstructionSource<uint32_t>(inst, 0);

  uint32_t kTileN;
  if (matrix_state->mtwiden() == 0) {
    // Degenerates to vsetvl using the current vtype: vl = min(rs1, VLMAX).
    kTileN = std::min<uint32_t>(kRs1, vector_state->max_vector_length());
  } else {
    const int kBound = std::min(vector_state->max_vector_length(),
                                matrix_state->effective_tile_edge());
    kTileN = std::min<uint32_t>(kRs1, kBound);
  }
  vector_state->set_vector_length(kTileN);
  WriteScalar(inst, 0, kTileN);
}

void RiscVZvtMsetTm(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  const uint32_t kRs1 = GetInstructionSource<uint32_t>(inst, 0);

  uint32_t kTileM;
  if (matrix_state->mtwiden() == 0) {
    kTileM = 0;
  } else {
    const int kBound = std::min(vector_state->max_vector_length(),
                                matrix_state->effective_tile_edge());
    kTileM = std::min<uint32_t>(kRs1, kBound);
  }
  matrix_state->set_tile_m(kTileM);
  vector_state->MarkDirty();
  WriteScalar(inst, 0, kTileM);
}

void RiscVZvtMsetTk(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  const uint32_t kRs1 = GetInstructionSource<uint32_t>(inst, 0);

  uint32_t kTileK;
  if (matrix_state->mtwiden() == 0) {
    kTileK = 0;
  } else {
    const int kSewBits = vector_state->selected_element_width() * 8;
    kTileK = std::min<uint32_t>(kRs1, RiscVZvtKmax(kSewBits));
  }
  matrix_state->set_tile_k(kTileK);
  vector_state->MarkDirty();
  WriteScalar(inst, 0, kTileK);
}

void RiscVZvtMsetMtypei(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  const uint32_t kUimm = GetInstructionSource<uint32_t>(inst, 0);
  const uint32_t kZimm10 = GetInstructionSource<uint32_t>(inst, 1);
  const uint32_t kMtwiden = kUimm & 0x3;
  const uint32_t kVsew = (kZimm10 >> 3) & 0x3;

  vector_state->set_vector_length(0);
  matrix_state->set_mtype(kMtwiden);

  if (kMtwiden != 0) {
    uint32_t vlmul = 4;
    switch (kVsew) {
      case 0: vlmul = 0; break;  // SEW8 -> LMUL1
      case 1: vlmul = 1; break;  // SEW16 -> LMUL2
      case 2: vlmul = 2; break;  // SEW32 -> LMUL4
      default: vlmul = 4; break;
    }
    uint32_t new_vtype = (kVsew << 3) | (1u << 6) | (1u << 7) | vlmul;
    vector_state->SetVectorType(new_vtype);
    if (vector_state->is_vill()) {
      matrix_state->set_mtype(0);
      return;
    }
    const int kSewBits = vector_state->selected_element_width() * 8;
    const int kTileElementWidth = kSewBits * matrix_state->twiden();
    if (kTileElementWidth > vector_state->max_elen()) {
      vector_state->SetVectorType(kVillBitMask);
      matrix_state->set_mtype(0);
      return;
    }
  } else {
    matrix_state->set_mtype(0);
    uint32_t new_vtype = (kVsew << 3);  // LMUL1, ta=0, ma=0
    vector_state->SetVectorType(new_vtype);
  }
}

// ----------------------------------------------------------------------------
// Tile manipulation
// ----------------------------------------------------------------------------

void RiscVZvtVtZero(const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  if (matrix_state->IsMatrixStatusOff() || vector_state->is_vill()) {
    TrapIllegalInstruction(matrix_state->riscv_state(), inst);
    return;
  }
  matrix_state->MarkDirty();
  const int kTile = GetInstructionSource<uint32_t>(inst, 0);  // mtd
  const int kTileElementWidth = matrix_state->tile_element_width_bits();
  const int kTileM = matrix_state->tile_m();
  const int kEffectiveTileEdge = kTileElementWidth < 64
                                     ? matrix_state->tile_elements()
                                     : matrix_state->tile_elements() / 2;
  const int kTileN =
      std::min(vector_state->vector_length(), kEffectiveTileEdge);
  for (int row = 0; row < kTileM; row++) {
    for (int col = 0; col < kTileN; col++) {
      StoreTileBits(matrix_state, kTile, row, col, kTileElementWidth, 0);
    }
  }
}

void RiscVZvtVtDiscard(const Instruction* inst) {
  // Architecturally hints the kTile state is dead. Transition to Initial.
  auto m_result = GetMatrixState(inst);
  if (!m_result.ok()) return;
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  if (matrix_state->IsMatrixStatusOff() || vector_state->is_vill()) {
    TrapIllegalInstruction(matrix_state->riscv_state(), inst);
    return;
  }
  matrix_state->MarkInitial();
  vector_state->MarkDirty();
}

// Number of kTile-subset elements processed: min(vl, EFFECTIVE_TILE_EDGE) for
// the given TILE_ELEMENT_WIDTH.
static int TileSubsetCount(RiscVZvtMatrixState* matrix_state,
                           RiscVVectorState* vector_state,
                           int tile_element_width) {
  const int kEffectiveTileEdge = tile_element_width < 64
                                     ? matrix_state->tile_elements()
                                     : matrix_state->tile_elements() / 2;
  return std::min(vector_state->vector_length(), kEffectiveTileEdge);
}

void RiscVZvtVtmvVt(const Instruction* inst) {
  // Tile row/col subset -> vector register group. Move TILE_ELEMENT_WIDTH =
  // SEW.
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  if (matrix_state->IsMatrixStatusOff() || vector_state->is_vill()) {
    TrapIllegalInstruction(matrix_state->riscv_state(), inst);
    return;
  }
  vector_state->MarkDirty();
  const int kVstart = vector_state->vstart();
  vector_state->clear_vstart();
  const Tss kTss = DecodeTss(GetInstructionSource<uint32_t>(inst, 0));
  const int kSewBytes = vector_state->selected_element_width();
  const int kTileElementWidth = kSewBytes * 8;
  const int kCount =
      TileSubsetCount(matrix_state, vector_state, kTileElementWidth);
  if (kVstart >= kCount) return;
  const int kElementsPerVector =
      vector_state->vector_register_byte_length() / kSewBytes;
  auto* vd = static_cast<RV32VectorDestinationOperand*>(inst->Destination(0));
  const int kNumRegs = (kCount + kElementsPerVector - 1) / kElementsPerVector;
  for (int r = 0; r < kNumRegs; r++) {
    DataBuffer* db = vd->AllocateDataBuffer(r);
    absl::Span<uint8_t> bytes = db->Get<uint8_t>();
    std::memset(bytes.data(), 0, bytes.size());  // tail-agnostic fill.
    for (int kE = 0; kE < kElementsPerVector; kE++) {
      const int kElem = r * kElementsPerVector + kE;
      if (kElem >= kCount) break;
      const uint64_t kVal =
          (kTss.pattern == kTssPatternCol)
              ? LoadTileBits(matrix_state, kTss.kTile, kElem, kTss.index,
                             kTileElementWidth)
              : LoadTileBits(matrix_state, kTss.kTile, kTss.index, kElem,
                             kTileElementWidth);
      std::memcpy(&bytes[kE * kSewBytes], &kVal, kSewBytes);
    }
    db->Submit();
  }
}

void RiscVZvtVtmvTv(const Instruction* inst) {
  // Vector register group -> kTile row/col subset. Move TILE_ELEMENT_WIDTH =
  // SEW.
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  if (matrix_state->IsMatrixStatusOff() || vector_state->is_vill()) {
    TrapIllegalInstruction(matrix_state->riscv_state(), inst);
    return;
  }
  matrix_state->MarkDirty();
  const int kVstart = vector_state->vstart();
  vector_state->clear_vstart();
  auto* vs = static_cast<RV32VectorSourceOperand*>(inst->Source(0));
  const Tss kTss = DecodeTss(GetInstructionSource<uint32_t>(inst, 1));
  const int kSewBytes = vector_state->selected_element_width();
  const int kTileElementWidth = kSewBytes * 8;
  const int kCount =
      TileSubsetCount(matrix_state, vector_state, kTileElementWidth);
  if (kVstart >= kCount) return;
  const int kElementsPerVector =
      vector_state->vector_register_byte_length() / kSewBytes;
  for (int kElem = kVstart; kElem < kCount; kElem++) {
    const int kReg = kElem / kElementsPerVector;
    const int kE = kElem % kElementsPerVector;
    auto bytes = vs->GetRegister(kReg)->data_buffer()->Get<uint8_t>();
    uint64_t kVal = 0;
    std::memcpy(&kVal, &bytes[kE * kSewBytes], kSewBytes);
    if (kTss.pattern == kTssPatternCol) {
      StoreTileBits(matrix_state, kTss.kTile, kElem, kTss.index,
                    kTileElementWidth, kVal);
    } else {
      StoreTileBits(matrix_state, kTss.kTile, kTss.index, kElem,
                    kTileElementWidth, kVal);
    }
  }
}

void RiscVZvtVtle(int eew_bits, const Instruction* inst) {
  // Memory -> kTile row/col subset. EEW is statically encoded.
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* state = matrix_state->riscv_state();
  auto* vector_state = state->rv_vector();
  if (eew_bits > vector_state->max_elen() || matrix_state->IsMatrixStatusOff() ||
      vector_state->is_vill()) {
    TrapIllegalInstruction(state, inst);
    return;
  }
  matrix_state->MarkDirty();
  const int kVstart = vector_state->vstart();
  vector_state->clear_vstart();
  const uint64_t kBase = GetInstructionSource<uint64_t>(inst, 0);       // rs1
  const Tss kTss = DecodeTss(GetInstructionSource<uint32_t>(inst, 1));  // rs2
  const int kElementSize = eew_bits / 8;
  const int kCount = TileSubsetCount(matrix_state, vector_state, eew_bits);
  if (kVstart >= kCount) return;
  const int kActiveCount = kCount - kVstart;
  DataBuffer* db = matrix_state->GetScratchDb(kActiveCount * kElementSize);
  state->memory()->Load(kBase + static_cast<uint64_t>(kVstart) * kElementSize,
                        db, nullptr, nullptr);
  absl::Span<uint8_t> bytes = db->Get<uint8_t>();
  for (int kE = kVstart; kE < kCount; kE++) {
    uint64_t kVal = 0;
    std::memcpy(&kVal, &bytes[(kE - kVstart) * kElementSize], kElementSize);
    if (kTss.pattern == kTssPatternCol) {
      StoreTileBits(matrix_state, kTss.kTile, kE, kTss.index, eew_bits, kVal);
    } else {
      StoreTileBits(matrix_state, kTss.kTile, kTss.index, kE, eew_bits, kVal);
    }
  }
}

void RiscVZvtVtse(int eew_bits, const Instruction* inst) {
  // Tile row/col subset -> memory. EEW is statically encoded.
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* state = matrix_state->riscv_state();
  auto* vector_state = state->rv_vector();
  if (eew_bits > vector_state->max_elen() || matrix_state->IsMatrixStatusOff() ||
      vector_state->is_vill()) {
    TrapIllegalInstruction(state, inst);
    return;
  }
  vector_state->MarkDirty();
  const int kVstart = vector_state->vstart();
  vector_state->clear_vstart();
  const uint64_t kBase = GetInstructionSource<uint64_t>(inst, 0);       // rs1
  const Tss kTss = DecodeTss(GetInstructionSource<uint32_t>(inst, 1));  // rs2
  const int kElementSize = eew_bits / 8;
  const int kCount = TileSubsetCount(matrix_state, vector_state, eew_bits);
  if (kVstart >= kCount) return;
  const int kActiveCount = kCount - kVstart;
  DataBuffer* db = matrix_state->GetScratchDb(kActiveCount * kElementSize);
  absl::Span<uint8_t> bytes = db->Get<uint8_t>();
  for (int kE = kVstart; kE < kCount; kE++) {
    const uint64_t kVal =
        (kTss.pattern == kTssPatternCol)
            ? LoadTileBits(matrix_state, kTss.kTile, kE, kTss.index, eew_bits)
            : LoadTileBits(matrix_state, kTss.kTile, kTss.index, kE, eew_bits);
    std::memcpy(&bytes[(kE - kVstart) * kElementSize], &kVal, kElementSize);
  }
  state->memory()->Store(kBase + static_cast<uint64_t>(kVstart) * kElementSize,
                         db);
}

// ----------------------------------------------------------------------------
// Matrix multiply: C[m,n] += sum_k A[k,m] * B[k,n]
// ----------------------------------------------------------------------------

void RiscVZvtMatmul(RiscVZvtMatmulKind kind, const Instruction* inst) {
  auto m_result = GetMatrixState(inst);
  auto* matrix_state = *m_result;
  auto* state = matrix_state->riscv_state();
  auto* vector_state = matrix_state->riscv_state()->rv_vector();
  if (matrix_state->IsMatrixStatusOff() || vector_state->is_vill()) {
    TrapIllegalInstruction(state, inst);
    return;
  }
  const bool kIsFp =
      (kind == RiscVZvtMatmulKind::kFp32 || kind == RiscVZvtMatmulKind::kBf16);
  matrix_state->MarkDirty();
  if (kIsFp && state->mstatus() != nullptr &&
      state->mstatus()->fs() != RiscVMStatus::kStatusOff &&
      state->mstatus()->fs() != RiscVMStatus::kStatusDirty) {
    state->mstatus()->set_fs(RiscVMStatus::kStatusDirty);
    state->mstatus()->Submit();
  }
  const int kSewBytes = vector_state->selected_element_width();
  const int kSewBits = kSewBytes * 8;
  const int kElementsPerVector =
      vector_state->vector_register_byte_length() / kSewBytes;  // elems/kReg.
  const int kStride = 8 / RiscVZvtKmax(kSewBits);  // regs between rows.
  const int kTileK =
      matrix_state->tile_k();  // K: the inner dimension of the dot product.
  const int kTileM =
      matrix_state
          ->tile_m();  // M: the number of rows of the accumulator kTile.
  const int kDestTile = GetInstructionSource<uint32_t>(inst, 2);  // mtd
  const int kTileElementWidth =
      matrix_state
          ->tile_element_width_bits();  // == 32 for the supported datatypes.
  const int kEffectiveTileEdge = kTileElementWidth < 64
                                     ? matrix_state->tile_elements()
                                     : matrix_state->tile_elements() / 2;
  // N: the number of columns of the accumulator kTile (derived from vector
  // length).
  const int kTileN =
      std::min(vector_state->vector_length(), kEffectiveTileEdge);
  if (!kIsFp && (kSewBytes != 1 || kTileElementWidth != 32)) {
    return;
  }

  auto* a = static_cast<RV32VectorSourceOperand*>(inst->Source(0));
  auto* b = static_cast<RV32VectorSourceOperand*>(inst->Source(1));

  absl::Span<uint8_t> a_spans[32] = {};
  absl::Span<uint8_t> b_spans[32] = {};

  // Reads element `col` of row `kk` from a vector-group operand.
  auto read_fp = [&](RV32VectorSourceOperand* op, absl::Span<uint8_t>* spans,
                     int kk, int col) -> float {
    const int kReg = kk * kStride + col / kElementsPerVector;
    const int kE = col % kElementsPerVector;
    if (spans[kReg].empty()) {
      spans[kReg] = op->GetRegister(kReg)->data_buffer()->Get<uint8_t>();
    }
    absl::Span<uint8_t> span = spans[kReg];
    if (kind == RiscVZvtMatmulKind::kBf16) {
      uint16_t u;
      std::memcpy(&u, &span[kE * kSewBytes], sizeof(u));
      return Bf16ToF32(u);
    }
    float f;
    std::memcpy(&f, &span[kE * kSewBytes], sizeof(f));
    return f;
  };
  const bool kVs2Signed = (kind == RiscVZvtMatmulKind::kInt8Signed);
  const bool kVs1Signed = ((vector_state->vtype() & 0x100) != 0);
  auto read_int = [&](RV32VectorSourceOperand* op, absl::Span<uint8_t>* spans,
                      int kk, int col, bool is_signed) -> int32_t {
    const int kReg = kk * kStride + col / kElementsPerVector;
    const int kE = col % kElementsPerVector;
    if (spans[kReg].empty()) {
      spans[kReg] = op->GetRegister(kReg)->data_buffer()->Get<uint8_t>();
    }
    absl::Span<uint8_t> span = spans[kReg];
    if (is_signed) {
      return static_cast<int8_t>(span[kE]);
    }
    return static_cast<uint8_t>(span[kE]);
  };

  for (int mm = 0; mm < kTileM; mm++) {
    for (int nn = 0; nn < kTileN; nn++) {
      if (kIsFp) {
        float acc =
            matrix_state->GetElem<float>(kDestTile, mm, nn, kTileElementWidth);
        for (int kk = 0; kk < kTileK; kk++) {
          float a_val = read_fp(a, a_spans, kk, mm);
          float b_val = read_fp(b, b_spans, kk, nn);
          acc = std::fma(a_val, b_val, acc);
        }
        if (std::isnan(acc)) {
          uint32_t nan_bits = 0x7fc00000;
          std::memcpy(&acc, &nan_bits, sizeof(float));
        }
        matrix_state->SetElem<float>(kDestTile, mm, nn, kTileElementWidth, acc);
      } else {
        int32_t acc = matrix_state->GetElem<int32_t>(kDestTile, mm, nn,
                                                     kTileElementWidth);
        for (int kk = 0; kk < kTileK; kk++) {
          acc += read_int(a, a_spans, kk, mm, kVs2Signed) *
                 read_int(b, b_spans, kk, nn, kVs1Signed);
        }
        matrix_state->SetElem<int32_t>(kDestTile, mm, nn, kTileElementWidth,
                                       acc);
      }
    }
  }
}

}  // namespace mpact::sim::riscv
