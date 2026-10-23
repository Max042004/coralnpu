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

# Small host tools and the unit tests that do not belong to a simulator flow.
#
#   make nexus-loader fst-query        build/tools/{nexus_loader,fst_query}
#   make tool-tests                    C++ unit tests of those tools
#   make py-tests                      python unit tests (+ MPACT-backed kernel tests)
#   make demo-rvv-add                  RVV element-wise add (8 x uint32_t) on MPACT and on the RTL

TOOLS_OUT := $(BUILD)/tools
TOOLS_CXXFLAGS = -std=c++20 -O1 -I$(ROOT) $(DEPS_INC)

.PHONY: nexus-loader fst-query tool-tests py-tests demo-rvv-add

# ---- nexus_loader: SPI loader for the Nexus board (FTDI) ----
NEXUS_SRCS := sw/utils/nexus_loader/spi_master.cc
$(TOOLS_OUT)/nexus_loader: sw/utils/nexus_loader/main.cc $(NEXUS_SRCS) $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(TOOLS_OUT)
	$(HOST_CXX) $(TOOLS_CXXFLAGS) -I/usr/include/libftdi1 -I/usr/include/libusb-1.0 \
	  sw/utils/nexus_loader/main.cc $(NEXUS_SRCS) -o $@ \
	  -Wl,--start-group $(ABSL_LIBS) -Wl,--end-group -lftdi1 -lusb-1.0 -lelf -lpthread
$(TOOLS_OUT)/nexus_loader_test: sw/utils/nexus_loader/test.cc $(NEXUS_SRCS) $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(TOOLS_OUT)
	$(HOST_CXX) $(TOOLS_CXXFLAGS) sw/utils/nexus_loader/test.cc $(NEXUS_SRCS) -o $@ -lgtest -lgtest_main -lpthread
nexus-loader: $(TOOLS_OUT)/nexus_loader

# ---- fst_query: query Verilator FST waveforms ----
FST_LIB_SRCS := utils/fst_query/fst_query.cc
FST_RT_SRCS = $(addprefix $(VERILATOR_INC)/,verilated.cpp verilated_dpi.cpp verilated_fst_c.cpp \
                verilated_random.cpp verilated_threads.cpp verilated_timing.cpp verilated_vpi.cpp)
FST_FLAGS = $(TOOLS_CXXFLAGS) -isystem $(VERILATOR_INC) -isystem $(VERILATOR_INC)/vltstd
FST_LIBS = -Wl,--start-group $(ABSL_LIBS) -Wl,--end-group -lre2 -lz -llz4 -lpthread
$(TOOLS_OUT)/fst_query: utils/fst_query/main.cc $(FST_LIB_SRCS) $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(TOOLS_OUT)
	$(HOST_CXX) $(FST_FLAGS) utils/fst_query/main.cc $(FST_LIB_SRCS) $(FST_RT_SRCS) -o $@ $(FST_LIBS)
$(TOOLS_OUT)/fst_query_test: utils/fst_query/fst_query_test.cc $(FST_LIB_SRCS) $(DEPS_PREFIX)/lib/libabsl_base.a
	@mkdir -p $(TOOLS_OUT)
	$(HOST_CXX) $(FST_FLAGS) utils/fst_query/fst_query_test.cc $(FST_LIB_SRCS) $(FST_RT_SRCS) -o $@ \
	  $(FST_LIBS) -lgtest -lgtest_main
fst-query: $(TOOLS_OUT)/fst_query

# fst_query_test also runs core_mini_axi_sim on math.elf to get a real trace.
$(ELF_DIR)/tests/verilator_sim/core_mini_axi_sim: $(SIM_DIR)/core_mini_axi_sim
	@mkdir -p $(dir $@) && ln -sf $< $@

tool-tests: $(TOOLS_OUT)/nexus_loader_test $(TOOLS_OUT)/fst_query_test \
            $(ELF_DIR)/tests/verilator_sim/core_mini_axi_sim $(ELF_DIR)/tests/cocotb/math.elf
	$(TOOLS_OUT)/nexus_loader_test
	cd $(ROOT) && CORALNPU_RUNFILES=$(ELF_DIR) $(TOOLS_OUT)/fst_query_test

# ---- python unit tests ----
# Plain unittest modules; firmware / the MPACT python module are built on demand.
PY_UNIT_TESTS := \
  coralnpu_test_utils/sim_backends/sim_backends_unit_test.py \
  tests/coralflow/runner_test.py \
  fpga/sw/flash_tool_test.py \
  utils/uvm_regression_lib_test.py
PY_KERNEL_TESTS := sw/opt/litert-micro/test/conv_test.py sw/opt/litert-micro/test/max_pool_test.py

py-tests: $(PYBIND_SO) $(ELF_DIR)/tests/cocotb/rvv/arithmetics/rvv_add_int8_m1.elf \
          $(ELF_DIR)/sw/opt/litert-micro/test/conv_test.elf $(ELF_DIR)/sw/opt/litert-micro/test/max_pool_test.elf
	set -e; for t in $(PY_UNIT_TESTS) $(PY_KERNEL_TESTS); do \
	  echo "== $$t"; CORALNPU_RUNFILES=$(ELF_DIR) $(ROOT)/utils/run_py_test.sh $$t; done

# ---- tutorial: RVV vector add (tests/cocotb/tutorial/program_rvv.cc) ----
RVV_ADD_ELF := $(ELF_DIR)/tests/cocotb/tutorial/coralnpu_v2_program_rvv.elf
demo-rvv-add: $(PYBIND_SO) $(RVV_ADD_ELF)
	$(ROOT)/utils/run_py_test.sh tests/cocotb/tutorial/npusim_rvv.py
	$(MAKE) cocotb-suite SUITE=tests_cocotb_tutorial_test_rvv
