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

import fnmatch
import unittest

from tests.uvm import denylist
from utils import uvm_regression_lib


class UvmRegressionTest(unittest.TestCase):

    def test_zvfbf_and_first_ml_ops_targets_are_denylisted(self):
        deny = denylist.DENYLIST

        self.assertIn("//tests/cocotb:zvfbf_test", deny)
        self.assertIn(
            "//tests/cocotb/rvv/ml_ops:rvv_matmul_assembly_highmem", deny
        )
        self.assertTrue(
            any(
                fnmatch.fnmatch("//tests/cocotb:zvfbf_test", pattern)
                for pattern in deny
            )
        )
        self.assertIn("//tests/cocotb/vme_test:vme_mset_test", deny)
        self.assertIn("//tests/cocotb/vme_test:vme_matmul_test", deny)

    def test_is_uvm_denylisted(self):
        self.assertTrue(denylist.is_uvm_denylisted("//tests/cocotb:plic_test"))
        self.assertIn("//tests/cocotb:loop", denylist.SPIKE_DENYLIST)
        self.assertFalse(denylist.is_uvm_denylisted("//tests/cocotb:math"))

    def test_format_batch_entry(self):
        line = uvm_regression_lib.format_batch_entry(
            "/path/to/test.elf", 0x1000, 0x2000, 5000, "spike.log",
            "//tests:target"
        )
        self.assertEqual(
            line,
            "/path/to/test.elf 00001000 00002000 5000 spike.log //tests:target\n"
        )


if __name__ == "__main__":
    unittest.main()
