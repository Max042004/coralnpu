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
"""Element-wise add of two 8 x uint32_t buffers (program_rvv.cc) on the MPACT
instruction-set simulator, the same program test_rvv.py runs on the RTL."""

import asyncio

import numpy as np

from coralnpu_test_utils import runfiles
from coralnpu_test_utils.sim_backends.mpact_npusim_test_fixture import (
    MpactNpuSimTestFixture,
)


async def _run():
    r = runfiles.Create()
    elf_path = r.Rlocation(
        "coralnpu_hw/tests/cocotb/tutorial/coralnpu_v2_program_rvv.elf")

    fixture = await MpactNpuSimTestFixture.Create()
    await fixture.load_elf_and_lookup_symbols(
        elf_path, ["input1_buffer", "input2_buffer", "output_buffer"])

    input1 = np.arange(8, dtype=np.uint32)
    input2 = 8994 * np.ones(8, dtype=np.uint32)
    await fixture.write("input1_buffer", input1)
    await fixture.write("input2_buffer", input2)

    await fixture.run_to_halt()

    result = (await fixture.read("output_buffer", 32)).view(np.uint32)
    print(f"I got {result}")
    expected = input1 + input2
    assert np.array_equal(result, expected), \
        f"Result mismatch: {result} vs {expected}"
    print(f"PASS (cycles: {fixture.get_cycle_count()})")


def main():
    asyncio.run(_run())


if __name__ == "__main__":
    main()
