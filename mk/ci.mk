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

# Aggregate targets used by .github/workflows (and handy locally).
#
#   make ci-lint | ci-rtl | ci-chisel | ci-sim | ci-cocotb | ci-uvm
#
# *_slow suites, rvv_matmul and mobilenet_v1 (too long for CI), and anything
# that needs a Synopsys licence (VCS) are not part of the CI suites.

CI_COCOTB_EXCLUDE := %_slow %rvv_matmul_cocotb_test %cocotb_run_mobilenet_v1
CI_COCOTB_SUITES  := $(filter-out $(CI_COCOTB_EXCLUDE),$(COCOTB_SUITES))

.PHONY: ci-lint ci-rtl ci-chisel ci-sim ci-cocotb ci-uvm

ci-lint:
	utils/run_linters.sh

# Generate the release RTL artifact (also what the "RTL Build" workflow publishes).
ci-rtl:
	$(MAKE) rtl DESIGN=RvvCoreMiniAxi

ci-chisel:
	$(MAKE) chisel-test

ci-sim: spike-test mpact-pytest sim-tests tool-tests py-tests
	$(MAKE) fpga-sim-test-fpga_clk_sim_test fpga-sim-test-fpga_trivial_pass_sim_test fpga-sim-test-fpga_dma_sim_test

ci-cocotb:
	$(MAKE) riscv-tests
	utils/run_cocotb_suites.sh $(CI_COCOTB_SUITES)

ci-uvm:
	$(MAKE) uvm-regression
