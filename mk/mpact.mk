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

# MPACT instruction-set simulator for CoralNPU (host C++).
#
#   make mpact          build build/mpact/bin/<coralnpu_v2_sim> (and the libraries)
#   make mpact-run ELF=tests/cocotb/wfi_slot_0
#
# mk/mpact_rules.mk is generated from the vendored Bazel build graphs by
# utils/bazel2make.py (see its docstring); this file supplies the variables it
# uses and the third-party pieces: antlr4 runtime + tool jar, linenoise, elfio.

MPACT_OUT := $(BUILD)/mpact
MPACT_GEN := $(MPACT_OUT)/gen
ANTLR_VERSION := 4.13.1
ANTLR_JAR := $(DEPS_BUILD)/antlr-$(ANTLR_VERSION)-complete.jar
ANTLR4_SRC = $(call dep_src,antlr4)
ELFIO_SRC = $(call dep_src,elfio)
LINENOISE_SRC = $(call dep_src,linenoise)

# The Bazel build compiled MPACT with clang (it needs -ffp-model=strict).
MPACT_CXX ?= clang++
MPACT_CC  ?= clang
MPACT_INCS =-Ivendor/mpact-sim -Ivendor/mpact-riscv -Ivendor/coralnpu-mpact \
             -I$(MPACT_GEN)/mpact-sim -I$(MPACT_GEN)/mpact-riscv -I$(MPACT_GEN)/coralnpu-mpact \
             -isystem $(DEPS_PREFIX)/include -isystem $(DEPS_PREFIX)/include/antlr4-runtime \
             -isystem $(ELFIO_SRC) -isystem $(LINENOISE_SRC)
MPACT_WARN = -w
MPACT_CXXFLAGS = -std=c++17 -O2 -fPIC -fexceptions $(MPACT_WARN) $(MPACT_INCS)
MPACT_CFLAGS = -O2 -fPIC $(MPACT_WARN) $(MPACT_INCS)
MPACT_LINENOISE = $(MPACT_OUT)/lib/liblinenoise.a
MPACT_LDLIBS = $(MPACT_LINENOISE) $(DEPS_PREFIX)/lib/libantlr4-runtime.a \
               -Wl,--start-group $(ABSL_LIBS) -Wl,--end-group -lre2 -lprotobuf -lpthread -ldl -lm

.PHONY: mpact mpact-run antlr4
antlr4: $(DEPS_PREFIX)/lib/libantlr4-runtime.a $(ANTLR_JAR)

$(DEPS_PREFIX)/lib/libantlr4-runtime.a: $(ROOT)/tools/.bender/.checked_out
	cmake -S $(ANTLR4_SRC)/runtime/Cpp -B $(DEPS_BUILD)/antlr4 $(CMAKE_COMMON) \
	  -DCMAKE_CXX_STANDARD=17 -DANTLR_BUILD_CPP_TESTS=OFF -DANTLR_BUILD_SHARED=OFF \
	  -DANTLR_BUILD_STATIC=ON -DWITH_DEMO=OFF
	cmake --build $(DEPS_BUILD)/antlr4 -j$(JOBS)
	cmake --install $(DEPS_BUILD)/antlr4

$(ANTLR_JAR):
	@mkdir -p $(dir $@)
	curl -fsSL -o $@ https://www.antlr.org/download/antlr-$(ANTLR_VERSION)-complete.jar
	echo "bc13a9c57a8dd7d5196888211e5ede657cb64a3ce968608697e4f668251a8487  $@" | sha256sum -c -

$(MPACT_LINENOISE): $(ROOT)/tools/.bender/.checked_out
	@mkdir -p $(dir $@) $(MPACT_OUT)/obj
	$(HOST_CC) -O2 -fPIC -c $(LINENOISE_SRC)/linenoise.c -o $(MPACT_OUT)/obj/linenoise.o
	rm -f $@ && $(AR) rcs $@ $(MPACT_OUT)/obj/linenoise.o

include mk/mpact_rules.mk

MPACT_SIM_BIN := $(MPACT_OUT)/bin/coralnpu_mpact_sim_coralnpu_v2_sim
mpact: $(MPACT_SIM_BIN)

# Order matters: tools -> generated sources -> objects. The generated rules already
# express this through their prerequisites; the third-party libs must exist first.
$(MPACT_SIM_BIN): $(MPACT_LINENOISE) $(DEPS_PREFIX)/lib/libantlr4-runtime.a $(ANTLR_JAR) $(DEPS_PREFIX)/lib/libabsl_base.a

mpact-run: mpact
	$(MPACT_SIM_BIN) $(ROOT)/build/runfiles/$(ELF).elf
