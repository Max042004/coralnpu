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

# Host-side C++ dependencies (abseil, SystemC, ...), fetched with Bender from
# tools/Bender.yml and built with CMake into $(DEPS_PREFIX).
#
#   make host-deps       fetch the sources (bender -d tools checkout)
#   make absl systemc    build + install them

TOOLS_BENDER   := $(BENDER) -d $(ROOT)/tools
DEPS_BUILD     := $(BUILD)/deps
DEPS_PREFIX    := $(DEPS_BUILD)/install
HOST_CXX       ?= g++
HOST_CC        ?= gcc

# Checkout directories (valid after `make host-deps`).
# $(shell ...) rather than $(wildcard ...): wildcard results are cached per make run.
dep_src = $(shell ls -d $(ROOT)/tools/.bender/git/checkouts/$(1)-* 2>/dev/null | head -1)
ABSEIL_SRC   = $(call dep_src,abseil_cpp)
SYSTEMC_SRC  = $(call dep_src,systemc)
SYSTEMCTLM_SRC = $(call dep_src,libsystemctlm_soc)
PYBIND11_SRC = $(call dep_src,pybind11)

CMAKE_COMMON := -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(HOST_CXX) \
                -DCMAKE_C_COMPILER=$(HOST_CC) -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$(DEPS_PREFIX) -DCMAKE_INSTALL_LIBDIR=lib

.PHONY: host-deps absl systemc

host-deps: $(ROOT)/tools/.bender/.checked_out

# Submodules are listed with ssh URLs; fetch them over https (no ssh keys needed).
GIT_HTTPS := GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=url.https://github.com/.insteadOf GIT_CONFIG_VALUE_0=git@github.com:

# The libsystemctlm-soc checkout may still fail on its (unneeded) submodules; the headers are all we use.
$(ROOT)/tools/.bender/.checked_out: tools/Bender.yml
	$(GIT_HTTPS) $(TOOLS_BENDER) update
	-$(GIT_HTTPS) $(TOOLS_BENDER) checkout
	@test -d "$(ABSEIL_SRC)" -a -d "$(SYSTEMC_SRC)" -a -d "$(SYSTEMCTLM_SRC)/soc" -a -d "$(PYBIND11_SRC)" \
	  || { echo "host dependency checkout incomplete"; exit 1; }
	@touch $@

absl: $(DEPS_PREFIX)/lib/libabsl_base.a
$(DEPS_PREFIX)/lib/libabsl_base.a: $(ROOT)/tools/.bender/.checked_out
	cmake -S $(ABSEIL_SRC) -B $(DEPS_BUILD)/absl $(CMAKE_COMMON) \
	  -DCMAKE_CXX_STANDARD=17 -DABSL_PROPAGATE_CXX_STD=ON -DABSL_BUILD_TESTING=OFF \
	  -DABSL_ENABLE_INSTALL=ON
	cmake --build $(DEPS_BUILD)/absl -j$(JOBS)
	cmake --install $(DEPS_BUILD)/absl

systemc: $(DEPS_PREFIX)/lib/libsystemc.a
$(DEPS_PREFIX)/lib/libsystemc.a: $(ROOT)/tools/.bender/.checked_out
	cmake -S $(SYSTEMC_SRC) -B $(DEPS_BUILD)/systemc $(CMAKE_COMMON) \
	  -DCMAKE_CXX_STANDARD=17 -DBUILD_SOURCE_DOCUMENTATION=OFF
	cmake --build $(DEPS_BUILD)/systemc -j$(JOBS) --target systemc
	cmake --install $(DEPS_BUILD)/systemc
