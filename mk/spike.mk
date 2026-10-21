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

# Spike (vendor/riscv-isa-sim, patched with the CoralNPU deviations) built with its
# own autotools flow, out of tree. Port of third_party/spike.BUILD.
#
#   make spike          -> build/spike/{spike,libriscv.a,libsoftfloat.a,libfesvr.a,libdisasm.a,libfdt.a}
#   make spike-test     -> sw/coralnpu_sim/spike_simulator_test against the co-sim wrapper

SPIKE_SRC   := $(ROOT)/vendor/riscv-isa-sim
SPIKE_BUILD := $(BUILD)/spike
SPIKE_ISA   := rv32imf_zve32f_zvl128b_zicsr_zifencei_zbb_zicclsm_zfbfmin_zvfbfa
SPIKE_LIBS  := libriscv.a libsoftfloat.a libfesvr.a libdisasm.a libfdt.a

.PHONY: spike spike-test
spike: $(SPIKE_BUILD)/spike

$(SPIKE_BUILD)/Makefile: $(SPIKE_SRC)/configure $(wildcard vendor/patches/riscv-isa-sim/*.patch)
	@mkdir -p $(SPIKE_BUILD)
	cd $(SPIKE_BUILD) && CXXFLAGS="-std=c++20" $(SPIKE_SRC)/configure \
	  --enable-commitlog --with-isa=$(SPIKE_ISA)

$(SPIKE_BUILD)/spike: $(SPIKE_BUILD)/Makefile
	$(MAKE) -C $(SPIKE_BUILD) -j$(JOBS)
	@touch $@

$(addprefix $(SPIKE_BUILD)/,$(SPIKE_LIBS)): $(SPIKE_BUILD)/spike

# ---- C++ co-simulation wrapper and its unit test (gtest from the distro) ----
VERILATOR_INC ?= $(TOOLS_DIR)/verilator/share/verilator/include
HOST_CXX ?= clang++
# Spike's own headers are included as <fesvr/htif.h>, so its root is a normal -I;
# the sub-directories are -iquote only, otherwise fesvr/syscall.h shadows the
# system <syscall.h> (SYS_futex).
SPIKE_INCS := -isystem $(VERILATOR_INC)/vltstd -isystem utils/cppshim -iquote $(ROOT) -I$(SPIKE_SRC) \
              -iquote $(SPIKE_SRC)/riscv -iquote $(SPIKE_SRC)/fesvr \
              -iquote $(SPIKE_SRC)/softfloat -iquote $(SPIKE_BUILD) -iquote $(SPIKE_SRC)/fdt
# Link the static archives explicitly (spike also builds libriscv.so, which -l would prefer).
SPIKE_LDLIBS := -Wl,--start-group $(addprefix $(SPIKE_BUILD)/,$(SPIKE_LIBS)) -Wl,--end-group \
                -lboost_system -lboost_regex -lpthread -ldl
SPIKE_TEST := $(SPIKE_BUILD)/spike_simulator_test

$(SPIKE_TEST): sw/coralnpu_sim/spike_simulator_test.cc sw/coralnpu_sim/spike_simulator.cc \
               sw/coralnpu_sim/spike_cosim_dpi.cc $(SPIKE_BUILD)/spike
	$(HOST_CXX) -std=c++20 -O1 $(SPIKE_INCS) $(filter %.cc,$^) -o $@ $(SPIKE_LDLIBS) -lgtest -lgtest_main

spike-test: $(SPIKE_TEST) $(ELF_DIR)/tests/cocotb/rvv/arithmetics/rvv_add_int8_m1.elf
	CORALNPU_RUNFILES=$(BUILD)/runfiles CORALNPU_ROOT=$(ROOT) $(SPIKE_TEST)
