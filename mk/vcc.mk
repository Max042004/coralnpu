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

# Verilated C++/SystemC models of the Chisel designs and the C++ testbenches
# built on them. Port of verilator_cc_library (rules/verilator.bzl) and of
# tests/verilator_sim.
#
#   make vcc DESIGN=CoreMiniAxi [KIND=sc|cc]   -> build/vcc/<design>_<kind>/
#   make sim-list                              list the testbench variants
#   make sim SIM=core_mini_axi_sim             build build/sim/core_mini_axi_sim
#   make sim-run SIM=core_mini_axi_sim ELF=tests/cocotb/wfi_slot_0

VCC_DIR   := $(BUILD)/vcc
SIM_DIR   := $(BUILD)/sim
VCC_INC   := $(BUILD)/vcc_inc
KIND      ?= sc

ABSL_LIBS = $(sort $(wildcard $(DEPS_PREFIX)/lib/libabsl_*.a))
DEPS_INC  = -isystem $(DEPS_PREFIX)/include

# Verilate + compile <design> as C++ (kind cc) or SystemC (kind sc); lib: libV<design>.a
define vcc_rule
$(VCC_DIR)/$(1)_$(2)/libV$(1).a: $(BUILD)/rtl/$(1)/$$($(1)_SV) $(DEPS_PREFIX)/lib/libsystemc.a
	@mkdir -p $(VCC_DIR)/$(1)_$(2)
	cd $(VCC_DIR)/$(1)_$(2) && SYSTEMC_INCLUDE=$(DEPS_PREFIX)/include SYSTEMC_LIBDIR=$(DEPS_PREFIX)/lib \
	  $$(VERILATOR) -j $$(JOBS) --no-std --output-groups $$(JOBS) --$(2) --trace-fst \
	  --top-module $(1) --prefix V$(1) -Mdir . $$($(1)_VOPTS) --pins-bv 2 \
	  $(BUILD)/rtl/$(1)/$$($(1)_SV)
	SYSTEMC_INCLUDE=$(DEPS_PREFIX)/include SYSTEMC_LIBDIR=$(DEPS_PREFIX)/lib \
	  $$(MAKE) -C $(VCC_DIR)/$(1)_$(2) -f V$(1).mk -j$$(JOBS) \
	    CXX=$$(HOST_CXX) OPT_FAST=-O2 OPT_SLOW=-O1 \
	    USER_CPPFLAGS="-fPIC -std=c++17 -Wno-sign-compare -Wno-unused-variable -Wno-unused-parameter -Wno-bool-operation -Wno-tautological-compare" \
	    libV$(1).a libverilated.a
# The generated header path the testbenches expect: hdl/chisel/src/coralnpu/V<design>_parameters.h
$(VCC_INC)/hdl/chisel/src/coralnpu/V$(1)_parameters.h: $(BUILD)/rtl/$(1)/$$($(1)_SV)
	@mkdir -p $$(dir $$@)
	cp $(BUILD)/rtl/$(1)/V$(1)_parameters.h $$@ 2>/dev/null || true
	@touch $$@
endef
$(foreach d,$(DESIGNS),$(eval $(call vcc_rule,$(d),sc)))
$(foreach d,$(DESIGNS),$(eval $(call vcc_rule,$(d),cc)))

.PHONY: vcc sim sim-list sim-run
vcc:
ifndef DESIGN
	$(error usage: make vcc DESIGN=<name> [KIND=sc|cc])
endif
	$(MAKE) $(VCC_DIR)/$(DESIGN)_$(KIND)/libV$(DESIGN).a

# ---- tests/verilator_sim C++ testbench variants: <sim name> <design> <extra defines> ----
SIM_TB_SRCS := tests/verilator_sim/coralnpu/core_mini_axi_tb.cc tests/verilator_sim/elf.cc \
               hdl/verilog/sram_backdoor.cc tests/systemc/instruction_trace.cc
SIM_INCS = -I$(ROOT) -I$(VCC_INC) -I$(SYSTEMCTLM_SRC) -I$(SYSTEMCTLM_SRC)/tests \
           -isystem $(TOOLS_DIR)/verilator/share/verilator/include \
           -isystem $(TOOLS_DIR)/verilator/share/verilator/include/vltstd $(DEPS_INC)
SIM_CXXFLAGS = -std=c++17 -O1 -g -Wno-sign-compare -Wno-unused-variable -DSC_INCLUDE_DYNAMIC_PROCESSES \
               -DCORALNPU_SIMD=256
SIM_LDLIBS = -Wl,--start-group $(ABSL_LIBS) -Wl,--end-group $(DEPS_PREFIX)/lib/libsystemc.a -lz -llz4 -lpthread -ldl

SIM_VARIANTS := \
  core_mini_axi_sim:CoreMiniAxi: \
  core_mini_verification_axi_sim:CoreMiniVerificationAxi: \
  core_mini_highmem_axi_sim:CoreMiniHighmemAxi:HIGHMEM_RTL \
  core_mini_itcm512kb_dtcm512kb_axi_sim:CoreMini_ITCM512KB_DTCM512KBAxi:HIGHMEM_RTL \
  rvv_core_mini_axi_sim:RvvCoreMiniAxi: \
  rvv_core_mini_highmem_axi_sim:RvvCoreMiniHighmemAxi:HIGHMEM_RTL \
  rvv_core_mini_itcm512kb_dtcm512kb_axi_sim:RvvCoreMini_ITCM512KB_DTCM512KBAxi:HIGHMEM_RTL \
  rvv_core_mini_verification_axi_sim:RvvCoreMiniVerificationAxi:

sim_field = $(word $(2),$(subst :, ,$(filter $(1):%,$(SIM_VARIANTS))))

define sim_rule
$(SIM_DIR)/$(1): $(VCC_DIR)/$(2)_sc/libV$(2).a $(VCC_INC)/hdl/chisel/src/coralnpu/V$(2)_parameters.h \
                 tests/verilator_sim/coralnpu/core_mini_axi_sim.cc $$(SIM_TB_SRCS) \
                 $(DEPS_PREFIX)/lib/libabsl_base.a $(DEPS_PREFIX)/lib/libsystemc.a
	@mkdir -p $(SIM_DIR)
	$$(HOST_CXX) $$(SIM_CXXFLAGS) $$(SIM_INCS) -I$(VCC_DIR)/$(2)_sc -DVERILATOR_MODEL=V$(2) \
	  -DVM_SC=1 -DVM_TRACE=1 $$(addprefix -D,$(3)) \
	  tests/verilator_sim/coralnpu/core_mini_axi_sim.cc $$(SIM_TB_SRCS) -o $$@ \
	  $(VCC_DIR)/$(2)_sc/libV$(2).a $(VCC_DIR)/$(2)_sc/libverilated.a $$(SIM_LDLIBS)
endef
$(foreach v,$(SIM_VARIANTS),$(eval $(call sim_rule,$(word 1,$(subst :, ,$(v))),$(word 2,$(subst :, ,$(v))),$(word 3,$(subst :, ,$(v))))))

sim-list:
	@printf '%s\n' $(foreach v,$(SIM_VARIANTS),$(word 1,$(subst :, ,$(v))))

sim:
ifndef SIM
	$(error usage: make sim SIM=<name>; see `make sim-list`)
endif
	$(MAKE) $(SIM_DIR)/$(SIM)

sim-run: sim
	$(SIM_DIR)/$(SIM) $(if $(ELF),--binary $(ROOT)/build/runfiles/$(ELF).elf)

# ---- other C++ testbenches: <name>:<design>:<main source>:<uses core_mini_axi_tb (1/0)> ----
SIM_TESTS := \
  core_mini_axi_non_incr_tests:CoreMiniAxi:tests/verilator_sim/coralnpu/core_mini_axi_non_incr_tests.cc:1 \
  backdoor_load_test:CoreMiniAxi:tests/verilator_sim/coralnpu/backdoor_load_test.cc:1 \
  dbus2axi_tb:DBus2AxiV2:tests/verilator_sim/coralnpu/dbus2axi_tb.cc:0 \
  core_mini_sim:CoreMini:tests/verilator_sim/coralnpu/core_tb.cc:0

define simtest_rule
$(SIM_DIR)/$(1): $(VCC_DIR)/$(2)_sc/libV$(2).a $(3) $$(SIM_TB_SRCS) $(DEPS_PREFIX)/lib/libsystemc.a \
                 $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(SIM_DIR) $(VCC_INC)/hdl/chisel/src/coralnpu
	-cp $(BUILD)/rtl/$(2)/V$(2)_parameters.h $(VCC_INC)/hdl/chisel/src/coralnpu/ 2>/dev/null
	$$(HOST_CXX) $$(SIM_CXXFLAGS) $$(SIM_INCS) -I$(VCC_DIR)/$(2)_sc -DVERILATOR_MODEL=V$(2) \
	  -DVM_SC=1 -DVM_TRACE=1 $(3) $$(if $(filter 1,$(4)),$$(SIM_TB_SRCS)) $$(if $(filter 0,$(4)),tests/verilator_sim/elf.cc hdl/verilog/sram_backdoor.cc) -o $$@ \
	  $(VCC_DIR)/$(2)_sc/libV$(2).a $(VCC_DIR)/$(2)_sc/libverilated.a $$(SIM_LDLIBS) \
	  -Wl,--allow-multiple-definition
endef
$(foreach v,$(SIM_TESTS),$(eval $(call simtest_rule,$(word 1,$(subst :, ,$(v))),$(word 2,$(subst :, ,$(v))),$(word 3,$(subst :, ,$(v))),$(word 4,$(subst :, ,$(v))))))

.PHONY: sim-tests
sim-tests: $(addprefix $(SIM_DIR)/,$(foreach v,$(SIM_TESTS),$(word 1,$(subst :, ,$(v)))))
	@set -e; for t in core_mini_axi_non_incr_tests backdoor_load_test dbus2axi_tb; do \
	  echo "== $$t"; (cd $(ROOT) && CORALNPU_RUNFILES=$(BUILD)/runfiles CORALNPU_ROOT=$(ROOT) $(SIM_DIR)/$$t); done

# ---- hw_sim: C++ wrapper around the verilated core + libcoralnpu_simulator*.so ----
# <so name>:<design>:<extra define>
HWSIM_LIBS := \
  libcoralnpu_simulator.so:CoreMiniAxi: \
  libcoralnpu_simulator_rvv.so:RvvCoreMiniAxi:ENABLE_RVV \
  libcoralnpu_simulator_vme.so:VmeCoreMiniAxi:ENABLE_VME
HWSIM_SRCS := hw_sim/core_mini_axi_simulator.cc hw_sim/hw_primitives.cc hdl/verilog/sram_backdoor.cc
HWSIM_INCS = -I$(ROOT) $(DEPS_INC) \
             -isystem $(TOOLS_DIR)/verilator/share/verilator/include \
             -isystem $(TOOLS_DIR)/verilator/share/verilator/include/vltstd
HWSIM_CXXFLAGS = -std=c++17 -O1 -fPIC -Wno-sign-compare -Wno-unused-variable

# The wrapper header includes the model headers of all three cores, so every
# model's include directory is on the path; only the selected core is linked.
HWSIM_MODEL_INCS = $(foreach d,CoreMiniAxi RvvCoreMiniAxi VmeCoreMiniAxi,-I$(VCC_DIR)/$(d)_cc)

define hwsim_rule
$(BUILD)/hw_sim/$(1): $(VCC_DIR)/CoreMiniAxi_cc/libVCoreMiniAxi.a $(VCC_DIR)/RvvCoreMiniAxi_cc/libVRvvCoreMiniAxi.a \
                      $(VCC_DIR)/VmeCoreMiniAxi_cc/libVVmeCoreMiniAxi.a $$(HWSIM_SRCS) $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(BUILD)/hw_sim
	$$(HOST_CXX) -shared $$(HWSIM_CXXFLAGS) $$(HWSIM_INCS) $$(HWSIM_MODEL_INCS) $$(addprefix -D,$(3)) \
	  $$(HWSIM_SRCS) -o $$@ \
	  -Wl,--whole-archive $(VCC_DIR)/$(2)_cc/libV$(2).a -Wl,--no-whole-archive \
	  $(VCC_DIR)/$(2)_cc/libverilated.a -Wl,--start-group $$(ABSL_LIBS) -Wl,--end-group -lz -llz4 -lpthread
endef
$(foreach v,$(HWSIM_LIBS),$(eval $(call hwsim_rule,$(word 1,$(subst :, ,$(v))),$(word 2,$(subst :, ,$(v))),$(word 3,$(subst :, ,$(v))))))

.PHONY: hwsim
hwsim: $(addprefix $(BUILD)/hw_sim/,$(foreach v,$(HWSIM_LIBS),$(word 1,$(subst :, ,$(v)))))
