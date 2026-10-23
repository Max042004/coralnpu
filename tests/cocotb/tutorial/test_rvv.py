# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import cocotb
import numpy as np

from coralnpu_test_utils import runfiles
from coralnpu_test_utils.core_mini_axi_interface import CoreMiniAxiInterface


@cocotb.test()
async def core_mini_axi_tutorial_rvv(dut):
    """Element-wise add of two 8 x uint32_t buffers with RVV intrinsics."""
    core_mini_axi = CoreMiniAxiInterface(dut)
    await core_mini_axi.init()
    await core_mini_axi.reset()
    cocotb.start_soon(core_mini_axi.clock.start())

    r = runfiles.Create()
    elf_path = r.Rlocation(
        "coralnpu_hw/tests/cocotb/tutorial/coralnpu_v2_program_rvv.elf")

    # Load the program into ITCM and look up the buffer addresses.
    with open(elf_path, "rb") as f:
        entry_point = await core_mini_axi.load_elf(f)
        input1_addr = core_mini_axi.lookup_symbol(f, "input1_buffer")
        input2_addr = core_mini_axi.lookup_symbol(f, "input2_buffer")
        output_addr = core_mini_axi.lookup_symbol(f, "output_buffer")

    input1 = np.arange(8, dtype=np.uint32)
    input2 = 8994 * np.ones(8, dtype=np.uint32)
    await core_mini_axi.write(input1_addr, input1)
    await core_mini_axi.write(input2_addr, input2)

    await core_mini_axi.execute_from(entry_point)
    await core_mini_axi.wait_for_halted()

    result = (await core_mini_axi.read(output_addr, 4 * 8)).view(np.uint32)
    print(f"I got {result}")
    expected = input1 + input2
    assert np.array_equal(result, expected), \
        f"Result mismatch: {result} vs {expected}"
