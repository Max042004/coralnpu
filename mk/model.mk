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

# Verilated cocotb models and cocotb test runs.
#
#   make model MODEL=core_mini_axi_model
#   make cocotb-test MODEL=core_mini_axi_model MODULE=tests.cocotb.core_mini_axi_sim \
#                    [TESTCASE=core_mini_axi_basic_write_read_memory]
#
# Verilator models for cocotb. The per-model settings are
# in mk/models.mk; the generated SystemVerilog comes from `make rtl`.

include mk/models.mk

TOOLS_DIR ?= $(HOME)/tools
PYTHON    ?= $(TOOLS_DIR)/venv/bin/python
VERILATOR ?= verilator
JOBS      ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)

COCOTB_DIR  := $(shell $(PYTHON) -c 'import cocotb,os;print(os.path.dirname(cocotb.__file__))' 2>/dev/null)
COCOTB_LIBS := $(COCOTB_DIR)/libs
COCOTB_CPP  := $(COCOTB_DIR)/share/lib/verilator/verilator.cpp

DPI_SRCS := hdl/verilog/sram_backdoor.cc

define model_rule
$(BUILD)/model/$(1)/$$($(1)_TOP): $(BUILD)/rtl/$$($(1)_DESIGN)/$$($$($(1)_DESIGN)_SV) $$(DPI_SRCS) hdl/verilog/sram_backdoor.h $$(if $$($(1)_VLT),$$($(1)_VLT),mk/default.vlt.tpl)
	@test -n "$$(COCOTB_DIR)" || { echo "cocotb not found: run utils/setup_vm.sh python"; exit 1; }
	@mkdir -p $(BUILD)/model/$(1)
	sed 's/{HDL_TOPLEVEL}/$$($(1)_TOP)/g' $$(if $$($(1)_VLT),$$($(1)_VLT),mk/default.vlt.tpl) > $(BUILD)/model/$(1)/$$($(1)_TOP).vlt
	cd $(ROOT) && $$(VERILATOR) -cc --exe -Mdir $(BUILD)/model/$(1) \
	  --top-module $$($(1)_TOP) --vpi --prefix Vtop -o $$($(1)_TOP) \
	  -LDFLAGS "-rdynamic -Wl,-rpath,$$(COCOTB_LIBS) -L$$(COCOTB_LIBS) -lcocotbvpi_verilator" \
	  $$(if $$(filter 1,$$($(1)_TRACE)),--trace) \
	  $$($(1)_CFLAGS) -I. -Ihdl/verilog -CFLAGS "-I$(ROOT) -I$(ROOT)/hdl/verilog" \
	  $$(COCOTB_CPP) $(BUILD)/model/$(1)/$$($(1)_TOP).vlt \
	  $$(addprefix $(ROOT)/,$$(DPI_SRCS)) \
	  $(BUILD)/rtl/$$($(1)_DESIGN)/$$($$($(1)_DESIGN)_SV)
	$$(MAKE) -j$$(JOBS) -C $(BUILD)/model/$(1) -f Vtop.mk OPT_FAST=-O3 $$(if $$(filter 1,$$($(1)_TRACE)),VM_TRACE=1)
endef
$(foreach m,$(MODELS),$(eval $(call model_rule,$(m))))

.PHONY: model list-models cocotb-test

list-models:
	@printf '%s\n' $(MODELS)

model:
ifndef MODEL
	$(error usage: make model MODEL=<name>; see `make list-models`)
endif
	$(MAKE) $(BUILD)/model/$(MODEL)/$($(MODEL)_TOP)

# Firmware ELFs that tests look up via coralnpu_test_utils/runfiles.py live in build/runfiles.
cocotb-test: model $(BUILD)/runfiles/toolchain/gdb
ifndef MODULE
	$(error usage: make cocotb-test MODEL=<name> MODULE=<python module>)
endif
	CORALNPU_RUNFILES=$(BUILD)/runfiles $(PYTHON) utils/cocotb_run.py \
	  --model $(BUILD)/model/$(MODEL)/$($(MODEL)_TOP) \
	  --hdl_toplevel $($(MODEL)_TOP) --test_module $(MODULE) \
	  $(if $(TESTCASE),--testcase $(TESTCASE)) $(if $(SEED),--seed $(SEED))

# ---- Suites (module + model pairs from the former cocotb_test_suite targets) ----

include mk/cocotb_suites.mk

.PHONY: list-suites cocotb-suite

list-suites:
	@printf '%s\n' $(COCOTB_SUITES)

cocotb-suite:
ifndef SUITE
	$(error usage: make cocotb-suite SUITE=<name>; see `make list-suites`)
endif
	$(MAKE) cocotb-test MODEL=$($(SUITE)_MODEL) MODULE=$($(SUITE)_MODULE) TESTCASE=$(TESTCASE) SEED=$(or $(SEED),$($(SUITE)_SEED))
