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

# riscv-tests ISA suites (rv32ui/um/uzbb/uf) built for CoralNPU.
#
#   make riscv-tests
#
# Sources: vendor/riscv-tests (Bender vendor_package, pinned + patched).
# The test environment (env/p, env/v) is CoralNPU-specific and lives in
# third_party/riscv-tests/env; both env variants link with the CoralNPU TCM
# linker script. Results are staged where tests/cocotb/core_mini_axi_sim.py
# looks for them:
#   build/runfiles/third_party/riscv-tests/copy_riscv_tests_rv32<S>/riscv_tests_rv32<S>/isa/

RISCV_TEST_SUITES := ui um uzbb uf
RT_ROOT := $(BUILD)/riscv-tests
RT_ENV  := $(RT_ROOT)/env
RT_OUT  := $(ELF_DIR)/third_party/riscv-tests

# Mirrors COMMON_ARGS of the former Bazel target.
RT_GCC_OPTS := -march=rv32imf_zve32x_zicsr_zifencei_zbb -mabi=ilp32 -mcmodel=medany -nostdlib

.PHONY: riscv-tests
riscv-tests: $(foreach s,$(RISCV_TEST_SUITES),$(RT_OUT)/copy_riscv_tests_rv32$(s)/.stamp)

$(RT_ENV)/.stamp: third_party/riscv-tests/env/p/riscv_test.h third_party/riscv-tests/env/v/riscv_test.h \
                  third_party/riscv-tests/env/v/entry.S third_party/riscv-tests/env/v/vm.c \
                  toolchain/coralnpu_tcm.ld.tpl utils/gen_linker_script.py
	@mkdir -p $(RT_ENV)/p $(RT_ENV)/v
	cp third_party/riscv-tests/env/p/riscv_test.h $(RT_ENV)/p/
	cp third_party/riscv-tests/env/v/riscv_test.h third_party/riscv-tests/env/v/entry.S third_party/riscv-tests/env/v/vm.c $(RT_ENV)/v/
	python3 utils/gen_linker_script.py --template toolchain/coralnpu_tcm.ld.tpl --out $(RT_ENV)/p/link.ld
	cp $(RT_ENV)/p/link.ld $(RT_ENV)/v/link.ld
	@touch $@

# isa/Makefile builds in the current directory, so work on a copy of the vendored tree.
$(RT_ROOT)/isa/.copied: $(shell find vendor/riscv-tests/isa -type f 2>/dev/null)
	@mkdir -p $(RT_ROOT)
	rm -rf $(RT_ROOT)/isa
	cp -R vendor/riscv-tests/isa $(RT_ROOT)/isa
	@touch $@

define riscv_test_suite
$(RT_OUT)/copy_riscv_tests_rv32$(1)/.stamp: $(RT_ROOT)/isa/.copied $(RT_ENV)/.stamp
	rm -rf $(RT_ROOT)/isa-$(1)
	cp -R $(RT_ROOT)/isa $(RT_ROOT)/isa-$(1)
	$$(MAKE) -C $(RT_ROOT)/isa-$(1) rv32$(1) XLEN=32 \
	  RISCV_PREFIX=$$(RV_PREFIX) RISCV_GCC_OPTS='$$(RT_GCC_OPTS)' RISCV_ENV_DIR=$$(RT_ENV)
	@mkdir -p $(RT_OUT)/copy_riscv_tests_rv32$(1)/riscv_tests_rv32$(1)/isa
	for f in $(RT_ROOT)/isa-$(1)/rv32$(1)-[pv]-*; do \
	  case $$$$f in *.dump|*.o|*fence_i*) ;; *) cp $$$$f $(RT_OUT)/copy_riscv_tests_rv32$(1)/riscv_tests_rv32$(1)/isa/;; esac; \
	done
	@touch $$@
endef
$(foreach s,$(RISCV_TEST_SUITES),$(eval $(call riscv_test_suite,$(s))))
