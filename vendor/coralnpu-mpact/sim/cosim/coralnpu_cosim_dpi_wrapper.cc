// Copyright 2025 Google LLC
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
#include <string>
#include <vector>

#include "sim/coralnpu_architecture.h"
#include "sim/coralnpu_simulator.h"
#include "sim/coralnpu_v2_state.h"
#include "sim/cosim/coralnpu_cosim_dpi.h"
#include "riscv/riscv_zvt_state.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "mpact/sim/generic/data_buffer.h"
#include "svdpi.h"

using ::coralnpu::sim::Architecture;
using ::coralnpu::sim::CoralNPUSimulator;
using ::coralnpu::sim::CoralNPUSimulatorOptions;
using ::coralnpu::sim::CoralNPUV2State;
using ::mpact::sim::riscv::RiscVZvtMatrixState;

namespace {

class MpactHandle {
 public:
  MpactHandle() = default;

  bool Init(const sim_config_t* cosim_config) {
    if (is_initialized_) {
      LOG(ERROR) << "[DPI] Init: is_initialized_ is already true.";
      return false;
    }
    if (cosim_config != nullptr) {
      options_.architecture =
          static_cast<Architecture>(cosim_config->architecture);
      options_.itcm_start_address = cosim_config->itcm_start_address;
      options_.memory_regions.push_back(
          {.start_address = cosim_config->itcm_start_address,
           .length = cosim_config->itcm_length,
           .permissions =
               ::coralnpu::sim::MemoryPermission::kReadWriteExecute});
      options_.initial_misa_value = cosim_config->initial_misa_value;
    }
    simulator_ = std::make_unique<CoralNPUSimulator>(options_);
    simulator_->state()->mie()->set_read_mask(0x888);
    simulator_->state()->mie()->set_write_mask(0x888);
    auto status = simulator_->WriteRegister("pc", options_.itcm_start_address);
    if (!status.ok()) {
      LOG(ERROR) << "[DPI] Init: Failed to set initial PC: "
                 << status.message();
      return false;
    }
    is_initialized_ = true;
    return true;
  }

  absl::Status LoadProgram(const std::string& elf_file) {
    if (!is_initialized_) {
      return absl::FailedPreconditionError(
          "[DPI] LoadProgram: Simulator not initialized.");
    }
    return simulator_->LoadProgram(elf_file);
  }

  CoralNPUV2State* state() const { return simulator_->state(); }
  CoralNPUSimulator* simulator() const { return simulator_.get(); }

  bool is_initialized() const { return is_initialized_; }

  coralnpu_architecture_t architecture() const {
    switch (options_.architecture) {
      case Architecture::kM4:
        return kCoralNPUM4;
      case Architecture::kM3:
        return kCoralNPUM3;
      case Architecture::kV2:
      default:
        return kCoralNPUV2;
    }
  }

  uint32_t get_pc() {
    auto read_reg_status = simulator_->ReadRegister("pc");
    if (!read_reg_status.ok()) {
      LOG(ERROR) << "[DPI] get_pc: Failed to read PC: "
                 << read_reg_status.status().message();
      return 0;
    }
    return static_cast<uint32_t>(read_reg_status.value());
  }

 private:
  CoralNPUSimulatorOptions options_;
  std::unique_ptr<CoralNPUSimulator> simulator_;
  bool is_initialized_ = false;
};

MpactHandle* g_mpact_handle = nullptr;
}  // namespace

extern "C" {

int mpact_init() {
  if (g_mpact_handle != nullptr) {
    LOG(ERROR) << "[DPI] mpact_init: g_mpact_handle is not null. "
               << "mpact_fini() must be called first.";
    return -1;
  }
  g_mpact_handle = new MpactHandle();
  return 0;
}

int mpact_config(const sim_config_t* config) {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_config: g_mpact_handle is null.";
    return -1;
  }
  if (g_mpact_handle->is_initialized()) {
    LOG(ERROR) << "[DPI] mpact_config: g_mpact_handle is already initialized.";
    return -1;
  }
  if (!g_mpact_handle->Init(config)) {
    return -1;
  }
  return 0;
}

int mpact_add_load_store_range(uint32_t start_address, uint32_t length) {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_add_load_store_range: g_mpact_handle is null.";
    return -1;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(WARNING) << "[DPI] mpact_add_load_store_range: mpact_config must be "
                 << "run before mpact_add_load_store_range.";
    return -2;
  }
  g_mpact_handle->state()->AddMemoryRegion(
      start_address, length, ::coralnpu::sim::MemoryPermission::kReadWrite);
  return 0;
}

int mpact_load_program(const char* elf_file) {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_load_program: g_mpact_handle is null.";
    return -1;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(ERROR)
        << "[DPI] mpact_load_program: g_mpact_handle is not initialized.";
    return -1;
  }
  if (elf_file == nullptr) {
    LOG(ERROR) << "[DPI] mpact_init: received a null elf program.";
    return -1;
  }
  auto status = g_mpact_handle->LoadProgram(elf_file);
  if (!status.ok()) {
    LOG(ERROR) << "[DPI] Failed to load program " << elf_file << ": "
               << status.message();
    return -1;
  }
  return 0;
}

int mpact_reset() {
  if (g_mpact_handle != nullptr) {
    mpact_fini();
  }
  return mpact_init();
}

int mpact_step(const svLogicVecVal* instruction) {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_step: g_mpact_handle is null.";
    return -1;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(INFO) << "[DPI] mpact_step: Lazy initialization of g_mpact_handle. "
              << "Using default DTCM start address and length.";
    if (!g_mpact_handle->Init(nullptr)) {
      return -1;
    }
  }

  uint32_t inst_word = instruction->aval;
  if (!g_mpact_handle->simulator()
           ->WriteMemory(g_mpact_handle->get_pc(), &inst_word,
                         sizeof(inst_word))
           .ok()) {
    LOG(ERROR) << "[DPI] mpact_step: Failed to write instruction to memory.";
    return 1;
  }

  absl::StatusOr<int> step_res = g_mpact_handle->simulator()->Step(1);
  if (!step_res.ok()) {
    LOG(ERROR) << "[DPI] mpact_step: Failed to step the simulator.";
    return 2;
  }
  if (step_res.value() != 1) {
    LOG(ERROR) << "[DPI] mpact_step: Failed to step the simulator by 1 "
                  "instruction.";
    return 3;
  }
  return 0;
}

bool mpact_is_halted() {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_is_halted: g_mpact_handle is null.";
    return false;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(INFO) << "[DPI] mpact_is_halted: Lazy initialization of "
                 "g_mpact_handle.";
    g_mpact_handle->Init(nullptr);
  }
  LOG(ERROR) << "[DPI] mpact_is_halted: Unimplemented.";
  return false;
}

int mpact_get_vector_register(const char* name, svLogicVecVal* value) {
  if (value == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_vector_register: value is null.";
    return -1;
  }
  if (name == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_vector_register: name is null.";
    return -3;
  }
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_vector_register: g_mpact_handle is null.";
    return -2;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(INFO) << "[DPI] mpact_get_vector_register: Lazy initialization of "
                 "g_mpact_handle.";
    if (!g_mpact_handle->Init(nullptr)) {
      return -1;
    }
  }

  absl::StatusOr<::mpact::sim::generic::DataBuffer*> vector_db =
      g_mpact_handle->simulator()->GetRegisterDataBuffer(name);
  if (!vector_db.ok()) {
    LOG(ERROR)
        << "[DPI] mpact_get_vector_register: Failed to get register data "
           "buffer: "
        << name;
    return -4;
  }

  absl::Span<uint32_t> vector_data = (*vector_db)->Get<uint32_t>();
  for (int i = 0; i < vector_data.size(); ++i) {
    value[i].aval = vector_data[i];
    value[i].bval = 0;
  }
  return 0;
}

int mpact_get_register(const char* name, uint32_t* value) {
  if (value == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_register: value is null.";
    return -1;
  }
  if (name == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_register: name is null.";
    return -3;
  }
  *value = 0;
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_register: g_mpact_handle is null.";
    return -2;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(INFO) << "[DPI] mpact_get_register: Lazy initialization of "
                 "g_mpact_handle.";
    if (!g_mpact_handle->Init(nullptr)) {
      return -1;
    }
  }

  if (std::string(name) == "pc") {
    *value = g_mpact_handle->get_pc();
    return 0;
  }

  auto read_reg_status = g_mpact_handle->simulator()->ReadRegister(name);
  if (!read_reg_status.ok()) {
    LOG(ERROR) << "[DPI] mpact_get_register: Failed to read register: " << name;
    return -4;
  }
  // CoralNPU V2 is a 32bit system. RiscVTop::ReadRegister outputs 64bit values
  // for both 32bit and 64bit systems. We can safely cast the value to uint32_t.
  *value = static_cast<uint32_t>(*read_reg_status);
  return 0;
}

int mpact_set_register(const char* name, uint32_t value) {
  if (name == nullptr) {
    LOG(ERROR) << "[DPI] mpact_set_register: name is null.";
    return -3;
  }
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_set_register: g_mpact_handle is null.";
    return -2;
  }
  if (!g_mpact_handle->is_initialized()) {
    LOG(INFO) << "[DPI] mpact_set_register: Lazy initialization of "
                 "g_mpact_handle.";
    if (!g_mpact_handle->Init(nullptr)) {
      return -1;
    }
  }

  auto write_reg_status =
      g_mpact_handle->simulator()->WriteRegister(name, value);
  if (!write_reg_status.ok()) {
    LOG(ERROR) << "[DPI] mpact_set_register: Failed to write register: "
               << name;
    return -4;
  }
  return 0;
}

int mpact_get_architecture(coralnpu_architecture_t* architecture) {
  if (g_mpact_handle == nullptr) {
    return -1;
  }
  if (architecture == nullptr) {
    return -1;
  }
  *architecture = g_mpact_handle->architecture();
  return 0;
}

namespace {
int NumTilesForTewWrapper(uint32_t tew) {
  switch (tew) {
    case 8:  return 16;
    case 16: return 8;
    case 32: return 4;
    case 64: return 8;
    default: return 0;
  }
}
}  // namespace

int mpact_get_matrix_tile_word_count(uint32_t tew, uint32_t* word_count) {
  if (word_count == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_matrix_tile_word_count: output is null.";
    return -1;
  }
  *word_count = 0;
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_matrix_tile_word_count: handle is null.";
    return -2;
  }
  if (!g_mpact_handle->is_initialized() &&
      !g_mpact_handle->Init(nullptr)) {
    return -3;
  }
  auto* matrix = g_mpact_handle->simulator()->matrix_state();
  if (matrix == nullptr) {
    LOG(ERROR) << "[DPI] Matrix state is unavailable for this architecture.";
    return -4;
  }
  if (NumTilesForTewWrapper(tew) == 0) {
    LOG(ERROR) << "[DPI] Unsupported matrix TEW: " << tew;
    return -5;
  }
  const uint32_t ete = static_cast<uint32_t>(tew == 64 ? matrix->tile_elements() / 2 : matrix->tile_elements());
  const uint32_t byte_count = ete * ete * (tew / 8);
  *word_count = (byte_count + sizeof(uint32_t) - 1) / sizeof(uint32_t);
  return 0;
}

int mpact_get_matrix_tile(uint32_t tile, uint32_t tew, svLogicVecVal* value,
                          uint32_t word_count) {
  if (value == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_matrix_tile: value is null.";
    return -1;
  }
  uint32_t required_words = 0;
  const int count_status =
      mpact_get_matrix_tile_word_count(tew, &required_words);
  if (count_status != 0) return count_status;
  if (word_count < required_words) {
    LOG(ERROR) << "[DPI] mpact_get_matrix_tile: output has " << word_count
               << " words, requires " << required_words << ".";
    return -6;
  }

  auto result = g_mpact_handle->simulator()->ReadMatrixTile(tile, tew);
  if (!result.ok()) {
    LOG(ERROR) << "[DPI] mpact_get_matrix_tile: "
               << result.status().message();
    return -7;
  }
  for (uint32_t word = 0; word < required_words; ++word) {
    uint32_t packed = 0;
    for (uint32_t byte = 0; byte < sizeof(uint32_t); ++byte) {
      const uint32_t offset = word * sizeof(uint32_t) + byte;
      if (offset < result->size()) {
        packed |= static_cast<uint32_t>((*result)[offset]) << (8 * byte);
      }
    }
    value[word].aval = packed;
    value[word].bval = 0;
  }
  return 0;
}

int mpact_get_current_matrix_tile(uint32_t tile, svLogicVecVal* value,
                                  uint32_t* tew,
                                  uint32_t* valid_word_count) {
  if (value == nullptr || tew == nullptr || valid_word_count == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_current_matrix_tile: output is null.";
    return -1;
  }
  *tew = 0;
  *valid_word_count = 0;
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_get_current_matrix_tile: handle is null.";
    return -2;
  }
  if (!g_mpact_handle->is_initialized() &&
      !g_mpact_handle->Init(nullptr)) {
    return -3;
  }

  auto* matrix = g_mpact_handle->simulator()->matrix_state();
  if (matrix == nullptr) {
    LOG(ERROR) << "[DPI] Matrix state is unavailable for this architecture.";
    return -4;
  }
  if (tile >= 16) {
    LOG(ERROR) << "[DPI] Matrix tile index must be below 16: " << tile;
    return -5;
  }
  const uint32_t current_tew =
      static_cast<uint32_t>(matrix->tile_element_width_bits());
  const uint32_t ptile_bytes =
      static_cast<uint32_t>(matrix->tile_elements() * matrix->tile_elements());
  const uint32_t ptile_words = ptile_bytes / sizeof(uint32_t);
  const uint32_t max_words =
      static_cast<uint32_t>(matrix->tile_elements() * matrix->tile_elements());
  for (uint32_t word = 0; word < max_words; ++word) {
    value[word].aval = 0;
    value[word].bval = 0;
  }
  const uint8_t* raw_bytes = matrix->raw_tile_bytes(static_cast<int>(tile));
  for (uint32_t word = 0; word < ptile_words; ++word) {
    uint32_t packed = 0;
    for (uint32_t byte = 0; byte < sizeof(uint32_t); ++byte) {
      packed |= static_cast<uint32_t>(raw_bytes[word * sizeof(uint32_t) + byte])
                << (8 * byte);
    }
    value[word].aval = packed;
    value[word].bval = 0;
  }

  *tew = current_tew;
  *valid_word_count = ptile_words;
  return 0;
}

int mpact_fini() {
  if (g_mpact_handle == nullptr) {
    LOG(ERROR) << "[DPI] mpact_fini: g_mpact_handle is null.";
    return -1;
  }
  delete g_mpact_handle;
  g_mpact_handle = nullptr;
  return 0;
}

}  // extern "C"
