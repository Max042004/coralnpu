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

# UVM testbench (tests/uvm) on Verilator, co-simulated against MPACT.
# Port of verilator_model / verilator_batch_uvm_test and tests/uvm/BUILD.
#
#   make uvm-model [DUT=rvv|vme]                 build build/uvm/<dut>/Vuvm
#   make uvm-test ELFS="tests/cocotb/wfi_slot_0 ..." [DUT=rvv] [SPIKE=1] [TIMEOUT=100000]
#
# The VCS flavour of the testbench (vcs_model, needs a Synopsys licence) is not
# built here; its sources and defines are the same ones used below.

UVM_OUT   := $(BUILD)/uvm
UVM_LIB   := vendor/uvm-verilator
DUT       ?= rvv
SPIKE     ?= 0
TIMEOUT   ?= 100000

UVM_SV := \
  tests/uvm/common/coralnpu_axi_master/coralnpu_axi_master_if.sv \
  tests/uvm/common/coralnpu_axi_slave/coralnpu_axi_slave_if.sv \
  tests/uvm/common/coralnpu_irq/coralnpu_irq_if.sv \
  tests/uvm/common/cosim/coralnpu_cosim_dpi_if.sv \
  tests/uvm/common/cosim/coralnpu_spike_cosim_dpi_if.sv \
  tests/uvm/common/memory_map_pkg.sv \
  tests/uvm/common/transaction_item/transaction_item_pkg.sv \
  tests/uvm/common/coralnpu_axi_master/coralnpu_axi_master_agent_pkg.sv \
  tests/uvm/common/coralnpu_axi_slave/coralnpu_axi_slave_agent_pkg.sv \
  tests/uvm/common/coralnpu_irq/coralnpu_irq_agent_pkg.sv \
  tests/uvm/common/cosim/coralnpu_rvvi_agent_pkg.sv \
  tests/uvm/common/cosim/coralnpu_cosim_checker_pkg.sv \
  tests/uvm/env/coralnpu_env_pkg.sv \
  tests/uvm/tests/coralnpu_test_pkg.sv \
  tests/uvm/tb/coralnpu_tb_top.sv
UVM_INC_DIRS := common/coralnpu_axi_master common/coralnpu_axi_slave common/coralnpu_irq \
                common/cosim common/transaction_item env tb tests
UVM_ALL_SRCS := $(UVM_SV) $(shell find tests/uvm -name '*.sv' -o -name '*.svh' 2>/dev/null)

UVM_VFLAGS := +define+UVM_NO_DEPRECATED +define+UVM_NO_REG +define+UVM_NO_TLM2 \
              +define+TB_SUPPORT +define+VLEN_128 +define+ZVE32F_ON \
              -Wno-PINMISSING -Wno-lint -Wno-style -Wno-fatal --timing --timescale 1ns/1ps
uvm_dut_flags_rvv =
uvm_dut_flags_vme = +define+ZVT_ON +define+ZVTBF16FMM_ON +define+DUT_MODULE=VmeCoreMiniVerificationAxi
uvm_dut_design_rvv = RvvCoreMiniVerificationAxi
uvm_dut_design_vme = VmeCoreMiniVerificationAxi

# C++ side: UVM DPI, SRAM backdoor, Spike co-simulation DPI (+ the MPACT cosim library).
UVM_DPI_CC := $(UVM_LIB)/src/dpi/uvm_dpi.cc hdl/verilog/sram_backdoor.cc \
              sw/coralnpu_sim/spike_cosim_dpi.cc sw/coralnpu_sim/spike_simulator.cc
UVM_CFLAGS = -std=c++20 -faligned-new -O1 -DVERILATOR=1 -DVL_TIME_CONTEXT \
             -Wno-vla-cxx-extension -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable \
             -I$(ROOT) -I$(ROOT)/$(UVM_LIB)/src/dpi \
             -I$(SPIKE_SRC) -iquote $(SPIKE_SRC)/riscv -iquote $(SPIKE_SRC)/fesvr \
             -iquote $(SPIKE_SRC)/softfloat -iquote $(SPIKE_BUILD) -iquote $(SPIKE_SRC)/fdt
UVM_LDFLAGS = $(MPACT_LINK_coralnpu_mpact_sim_cosim_coralnpu_cosim_lib) $(MPACT_LDLIBS) \
              -Wl,--start-group $(addprefix $(SPIKE_BUILD)/,$(SPIKE_LIBS)) -Wl,--end-group \
              -lboost_system -lboost_regex -latomic -lstdc++ -lm

.PHONY: uvm-model uvm-test

define uvm_model_rule
$(UVM_OUT)/$(1)/Vuvm: $(BUILD)/rtl/$(uvm_dut_design_$(1))/$(uvm_dut_design_$(1)).sv $$(UVM_ALL_SRCS) mk/default.vlt.tpl \
                      $$(MPACT_ARCHIVES_coralnpu_mpact_sim_cosim_coralnpu_cosim_lib) \
                      $(SPIKE_BUILD)/spike $(MPACT_LINENOISE) $(DEPS_PREFIX)/lib/libantlr4-runtime.a
	@mkdir -p $(UVM_OUT)/$(1)
	sed 's/{HDL_TOPLEVEL}/coralnpu_tb_top/g' mk/default.vlt.tpl > $(UVM_OUT)/$(1)/coralnpu_tb_top.vlt
	cd $(ROOT) && $$(VERILATOR) -cc --exe --main --build -j $$(JOBS) --top-module coralnpu_tb_top --vpi \
	  --prefix Vtop -Mdir $(UVM_OUT)/$(1) -o Vuvm \
	  -I$(UVM_LIB)/src $(UVM_LIB)/src/uvm_pkg.sv \
	  $$(addprefix -Itests/uvm/,$$(UVM_INC_DIRS)) $$(UVM_VFLAGS) $$(uvm_dut_flags_$(1)) \
	  --output-split 20000 --output-split-cfuncs 2000 --output-groups $$(JOBS) \
	  -CFLAGS "$$(UVM_CFLAGS)" -LDFLAGS "$$(UVM_LDFLAGS)" \
	  $(UVM_OUT)/$(1)/coralnpu_tb_top.vlt $$(UVM_SV) $(BUILD)/rtl/$(uvm_dut_design_$(1))/$(uvm_dut_design_$(1)).sv \
	  $$(addprefix $(ROOT)/,$$(UVM_DPI_CC))
endef
$(foreach d,rvv vme,$(eval $(call uvm_model_rule,$(d))))

uvm-model: $(UVM_OUT)/$(DUT)/Vuvm

# Regression over a list of firmware ELFs (the former verilator_batch_uvm_test).
# Each ELF is <package>/<name>; "SPIKE=1" additionally cross-checks against Spike.
uvm-test: $(UVM_OUT)/$(DUT)/Vuvm $(foreach e,$(ELFS),$(ELF_DIR)/$(e).elf) $(SPIKE_BUILD)/spike
ifndef ELFS
	$(error usage: make uvm-test ELFS="tests/cocotb/wfi_slot_0 ..." [DUT=rvv|vme] [SPIKE=1])
endif
	cd $(ROOT) && UVM_MODEL_RLOCATION=$(UVM_OUT)/$(DUT)/Vuvm \
	  UVM_ENABLE_SPIKE=$(SPIKE) UVM_SPIKE_RLOCATION=$(if $(filter 1,$(SPIKE)),SPIKE) \
	  UVM_CORALNPU_ELFS="$$(for e in $(ELFS); do \
	    printf '%s\t%s\t%s\t%s\n' $(ELF_DIR)/$$e.elf $$e $(TIMEOUT) $(if $(filter 1,$(SPIKE)),True,False); done)" \
	  CORALNPU_RUNFILES=$(ELF_DIR) PYTHONPATH=$(ROOT)/utils:$(ROOT)/utils/pyshim \
	  $(PYTHON) utils/uvm_batch_runner.py

# Full regression: every firmware binary / riscv-test that was a UVM regression target in the
# Bazel build (list: mk/uvm_regression.tsv, from utils/gen_uvm_regression.py).
UVM_REG_TSV  := mk/uvm_regression.tsv
UVM_REG_FW   := $(addprefix $(ELF_DIR)/,$(filter-out third_party/%,$(shell cut -f1 $(UVM_REG_TSV))))

.PHONY: uvm-regression
uvm-regression: $(UVM_OUT)/$(DUT)/Vuvm $(UVM_REG_FW) riscv-tests $(SPIKE_BUILD)/spike
	cd $(ROOT) && UVM_MODEL_RLOCATION=$(UVM_OUT)/$(DUT)/Vuvm \
	  UVM_ENABLE_SPIKE=$(SPIKE) UVM_SPIKE_RLOCATION=$(if $(filter 1,$(SPIKE)),SPIKE) \
	  UVM_CORALNPU_ELFS="$$(awk -F'\t' -v d=$(ELF_DIR) -v sp=$(SPIKE) \
	    '{ printf "%s/%s\t%s\t%s\t%s\n", d, $$1, $$2, $$3, ($$4 == "True" && sp == "1") ? "True" : "False" }' \
	    $(UVM_REG_TSV))" \
	  CORALNPU_RUNFILES=$(ELF_DIR) PYTHONPATH=$(ROOT)/utils:$(ROOT)/utils/pyshim \
	  $(PYTHON) utils/uvm_batch_runner.py
