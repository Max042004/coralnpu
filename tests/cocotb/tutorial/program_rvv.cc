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

#include <riscv_vector.h>
#include <stddef.h>
#include <stdint.h>

// Two input buffers and one output buffer of 8 uint32_t each. The testbench
// finds them by symbol name and reads/writes them through the AXI interface.
uint32_t input1_buffer[8] __attribute__((section(".data")));
uint32_t input2_buffer[8] __attribute__((section(".data")));
uint32_t output_buffer[8] __attribute__((section(".data")));

int main(int argc, char** argv) {
  size_t avl = 8;
  const uint32_t* in1 = input1_buffer;
  const uint32_t* in2 = input2_buffer;
  uint32_t* out = output_buffer;

  // Strip-mined element-wise add: vl is whatever the hardware grants per pass.
  while (avl > 0) {
    size_t vl = __riscv_vsetvl_e32m1(avl);
    vuint32m1_t v1 = __riscv_vle32_v_u32m1(in1, vl);
    vuint32m1_t v2 = __riscv_vle32_v_u32m1(in2, vl);
    vuint32m1_t vsum = __riscv_vadd_vv_u32m1(v1, v2, vl);
    __riscv_vse32_v_u32m1(out, vsum, vl);
    in1 += vl;
    in2 += vl;
    out += vl;
    avl -= vl;
  }
  return 0;
}
