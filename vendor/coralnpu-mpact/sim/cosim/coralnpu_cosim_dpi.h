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

// Defines the DPI-C interface for cosimulation.
//
// These C-style functions allow a SystemVerilog testbench to control an
// MPACT-based golden reference model, running it in lock-step with a
// design under test (DUT).
//
// Note: This interface is designed for a single simulator instance and is not
// thread-safe.

#ifndef SIM_COSIM_CORALNPU_COSIM_DPI_H_
#define SIM_COSIM_CORALNPU_COSIM_DPI_H_

#include <cstdint>

#include "sim/coralnpu_architecture.h"
#include "svdpi.h"

typedef struct {
  uint32_t itcm_start_address;  // Start address of the ITCM range.
  uint32_t itcm_length;         // Length of the ITCM range.
  uint32_t initial_misa_value;  // Initial value of the misa register.
  coralnpu_architecture_t architecture;
} sim_config_t;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the MPACT simulator. This function must be called before any
// other MPACT functions.
// Return 0 on success, non-zero on failure.
int mpact_init();

// Configure the MPACT simulator. This function should be called after
// mpact_init. If not called, the default configuration is used.
// Return 0 on success, non-zero on failure.
int mpact_config(const sim_config_t* config);

// Configure the MPACT simulator to allow LOAD/STORE access to a memory range.
// Return 0 on success, non-zero on failure.
int mpact_add_load_store_range(uint32_t start_address, uint32_t length);

// Loads an ELF file into the simulation memory.
// Return 0 on success, non-zero on failure.
int mpact_load_program(const char* elf_file);

// Reset the MPACT simulator's architectural state.
// Return 0 on success, non-zero on failure.
int mpact_reset();

// Step the MPACT simulator by executing a single provided instruction.
// The instruction is provided as a SystemVerilog datatype - svLogicVecVal*.
// Return 0 on success, non-zero on failure.
int mpact_step(const svLogicVecVal* instruction);

// Check if the MPACT simulator has reached a halted state. Some tests may
// require the simulator to be halted before checking the results.
// Currently unimplemented and always returns false.
bool mpact_is_halted();

// Return the value of the specified register. Register names are provided as
// null-terminated c-style strings.
// Returns 0 on success, non-zero on failure.
// The register value is returned in the uint32_t argument.
int mpact_get_register(const char* name, uint32_t* value);

// Set the value of the specified register. Register names are provided as
// null-terminated c-style strings.
// Returns 0 on success, non-zero on failure.
int mpact_set_register(const char* name, uint32_t value);

// Return the value of the specified vector register. Register names are
// provided as null-terminated c-style strings.
// Returns 0 on success, non-zero on failure.
// The register value is returned in the svLogicVecVal argument.
int mpact_get_vector_register(const char* name, svLogicVecVal* value);

// Return the number of 32-bit words required to hold one logical matrix tile
// at the requested TEW. The tile data is exported row-major and each element
// is little-endian within the word stream.
int mpact_get_matrix_tile_word_count(uint32_t tew, uint32_t* word_count);

// Return one logical matrix tile.  is the architectural 4-bit tile
// specifier; ignored low bits are canonicalized according to TEW. The caller
// must provide at least the word count returned by
// mpact_get_matrix_tile_word_count().
int mpact_get_matrix_tile(uint32_t tile, uint32_t tew, svLogicVecVal* value,
                          uint32_t word_count);

// Return one logical matrix tile using MPACT's current matrix configuration.
//  must provide at least TE*TE 32-bit words, where TE is the configured
// matrix tile edge. The function returns the current TEW and the number of
// valid words written. Tile aliases are canonicalized according to current TEW.
int mpact_get_current_matrix_tile(uint32_t tile, svLogicVecVal* value,
                                  uint32_t* tew, uint32_t* valid_word_count);

// Return the configured architecture of the simulator.
// Returns 0 on success, non-zero on failure.
int mpact_get_architecture(coralnpu_architecture_t* architecture);

// Finalize and clean up MPACT simulator resources.
// Return 0 on success, non-zero on failure.
int mpact_fini();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SIM_COSIM_CORALNPU_COSIM_DPI_H_
