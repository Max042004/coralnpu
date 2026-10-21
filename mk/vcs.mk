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

# Synopsys VCS flavour of the UVM testbench (tests/uvm).
#
#   make uvm-model-vcs [DUT=rvv|vme]       -> build/uvm_vcs/<dut>/simv
#
# NOT EXERCISED: this needs a VCS installation and licence (VCS_HOME, VERDI_HOME,
# LM_LICENSE_FILE), none of which exist where this port was developed. It
# reproduces the command line of the former vcs_model rule (same sources, defines,
# DPI code and co-simulation libraries as the Verilator model in mk/uvm.mk), so
# treat it as a starting point and expect to adjust it on a machine with VCS.
# The VCS variants of the cocotb/testbench rules are not ported.

UVM_VCS_OUT := $(BUILD)/uvm_vcs
VCS ?= vcs

UVM_VCS_DEFS := +define+UVM_NO_DEPRECATED +define+TB_SUPPORT +define+VLEN_128 +define+ZVE32F_ON \
                +define+USE_GENERIC -ntb_opts uvm-1.2 -timescale=1ns/1ps -debug_access+all

.PHONY: uvm-model-vcs

define uvm_vcs_rule
$(UVM_VCS_OUT)/$(1)/simv: $(BUILD)/rtl/$(uvm_dut_design_$(1))/$(uvm_dut_design_$(1)).sv $$(UVM_ALL_SRCS) \
                          $$(MPACT_ARCHIVES_coralnpu_mpact_sim_cosim_coralnpu_cosim_lib) $(SPIKE_BUILD)/spike
	@command -v $$(VCS) >/dev/null || { echo "VCS not found (set VCS_HOME / PATH)"; exit 1; }
	@mkdir -p $(UVM_VCS_OUT)/$(1)
	cd $(ROOT) && $$(VCS) -full64 -sverilog -q +define+VCS -debug_access+all +notimingcheck \
	  -timescale=1ns/1ps -Mdir=$(UVM_VCS_OUT)/$(1)/csrc -cpp clang++ -cflags -std=c++20 \
	  -cflags -I$(ROOT) -cflags -I$(SPIKE_SRC) -cflags -I$(VERILATOR_INC)/vltstd \
	  -LDFLAGS "$$(UVM_LDFLAGS)" -o $(UVM_VCS_OUT)/$(1)/simv -top coralnpu_tb_top \
	  $$(addprefix +incdir+tests/uvm/,$$(UVM_INC_DIRS)) +incdir+. +incdir+tests/uvm \
	  $$(UVM_VCS_DEFS) $$(uvm_dut_flags_$(1)) \
	  $$(UVM_SV) $(BUILD)/rtl/$(uvm_dut_design_$(1))/$(uvm_dut_design_$(1)).sv \
	  hdl/verilog/sram_backdoor.cc sw/coralnpu_sim/spike_cosim_dpi.cc sw/coralnpu_sim/spike_simulator.cc
endef
$(foreach d,rvv vme,$(eval $(call uvm_vcs_rule,$(d))))

uvm-model-vcs: $(UVM_VCS_OUT)/$(DUT)/simv
