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
"""Run c_baremetal_demo.c (plain C) on the MPACT simulator and report its checks."""

import asyncio

import numpy as np

from coralnpu_test_utils import runfiles
from coralnpu_test_utils.sim_backends.mpact_npusim_test_fixture import (
    MpactNpuSimTestFixture,
)


async def main_async():
    elf = runfiles.Create().Rlocation(
        "coralnpu_hw/tests/npusim_examples/c_baremetal_demo.elf")
    fixture = await MpactNpuSimTestFixture.Create(
        highmem=True, exit_on_ebreak=True)
    await fixture.load_elf_and_lookup_symbols(
        elf, ["checks_total", "checks_passed"])
    await fixture.run_to_halt()
    total = int((await fixture.read("checks_total", size=4, dtype=np.uint8)
                 ).view(np.int32)[0])
    passed = int((await fixture.read("checks_passed", size=4, dtype=np.uint8)
                  ).view(np.int32)[0])
    print(f"program reports {passed}/{total} checks passed, "
          f"{fixture.get_cycle_count()} simulated cycles")
    assert total > 0 and passed == total


if __name__ == "__main__":
    asyncio.run(main_async())
