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

# CoralNPU firmware (RV32 ELFs) built with the prebuilt cross toolchain.
#
#   make elf T=tests/cocotb/wfi_slot_0     one binary  -> build/runfiles/tests/cocotb/wfi_slot_0.elf
#   make elfs-cocotb                       every tests/cocotb firmware
#   make list-elfs
#
# Port of rules/coralnpu_v2.bzl + toolchain/. Target table: mk/sw_targets.mk.
# ELFs land in build/runfiles/<package>/ where the tests' runfiles shim looks.
#
#   SW_COPT=fastbuild|opt|dbg   same meaning as bazel -c (default fastbuild)

include mk/sw_targets.mk

CORALNPU_TC ?= $(TOOLS_DIR)/toolchain_coralnpu_v2
RV_PREFIX   ?= $(CORALNPU_TC)/bin/riscv64-unknown-elf-
RV_CC       := $(RV_PREFIX)gcc
RV_CXX      := $(RV_PREFIX)g++
RV_OBJCOPY  := $(RV_PREFIX)objcopy

SW_COPT ?= fastbuild
SW_OPTFLAGS_fastbuild := -g3 -O1 -DNDEBUG
SW_OPTFLAGS_opt       := -O3 -DNDEBUG -ffunction-sections -fdata-sections
SW_OPTFLAGS_dbg       := -g3 -Og

SW_ARCH   := rv32imf_zve32f_zicsr_zifencei_zbb_zfbfmin_zvfbfmin_zvfbfwma
SW_COMMON := -march=$(SW_ARCH) -mabi=ilp32 -mcmodel=medany -nostdlib \
             -Wall -Werror -Wno-unused-function \
             -DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS -DTF_LITE_USE_GLOBAL_MIN -DTF_LITE_USE_GLOBAL_MAX \
             -DTF_LITE_MCU_DEBUG_LOG $(if $(filter opt,$(SW_COPT)),-DTF_LITE_STRIP_ERROR_STRINGS) -iquote $(ROOT) -I$(ROOT) \
             $(SW_OPTFLAGS_$(SW_COPT)) -ffunction-sections -fdata-sections
SW_CFLAGS   := $(SW_COMMON) -std=gnu11
SW_CXXFLAGS := $(SW_COMMON) -std=c++17 -fno-rtti -fno-exceptions
SW_LDFLAGS_NANO := --specs=nano.specs -Wl,--gc-sections -nostartfiles \
                   -Wl,--start-group -lstdc++ -lm -lc -lgcc -Wl,--end-group
SW_LDFLAGS_HTIF := --specs=htif_nano.specs -lsemihost -Wl,--gc-sections \
                   -Wl,--start-group -lstdc++ -lm -lc -lgcc -Wl,--end-group

CRT_DIR       := toolchain/crt
CRT_SRCS      := coralnpu_exceptions.cc coralnpu_gloss.cc coralnpu_start.S crt.S cxx_guards.cc
CRT_SRCS_HTIF := coralnpu_exceptions.cc coralnpu_htif_gloss.cc coralnpu_start.S crt.S

SW_OUT := $(BUILD)/sw
ELF_DIR := $(BUILD)/runfiles

# Sources generated from templates (RVV arithmetic tests, ...): mk/gen_sources.json.
GEN_DIR   := $(BUILD)/gen
GEN_STAMP := $(GEN_DIR)/.stamp
GEN_INPUTS := $(shell python3 -c "import json;print(' '.join(sorted({j.get('template') or j['src'] for j in json.load(open('mk/gen_sources.json'))})))")

# Templates are expanded literally; "cc_array" jobs turn .tflite files into C arrays
# with TFLM's generate_cc_arrays.py (needs numpy/PIL, hence the venv python).
$(GEN_STAMP): mk/gen_sources.json utils/expand_templates.py $(GEN_INPUTS)
	@mkdir -p $(GEN_DIR)
	$(PYTHON) utils/expand_templates.py --spec mk/gen_sources.json --outdir $(GEN_DIR)
	@touch $@

$(GEN_DIR)/%.cc $(GEN_DIR)/%.h: $(GEN_STAMP)
	@:

# Generated headers (genrules of the Bazel build); compiled sources see them via -iquote $(GEN_DIR).
GEN_HDRS := $(GEN_DIR)/coralnpu_test_utils/rvv_cpp_util.h

$(GEN_DIR)/coralnpu_test_utils/rvv_cpp_util.h: coralnpu_test_utils/rvv_cpp_util_header_generator.py
	@mkdir -p $(dir $@)
	python3 $< > $@

# $(call sw_compile,<src path>,<obj>,<extra flags>)
define sw_cc_rule
$(2): $(1) | $$(GEN_HDRS) $$(GEN_STAMP)
	@mkdir -p $$(dir $$@)
	$$(if $$(filter %.cc %.cpp,$(1)),$$(RV_CXX) $$(SW_CXXFLAGS),$$(RV_CC) $$(SW_CFLAGS)) -iquote $$(GEN_DIR) -iquote $$(GEN_DIR)/genfiles $(3) -MMD -MP -c $$< -o $$@
endef

# ---- TensorFlow Lite Micro (vendor/tflite_micro) as a static library ----
include mk/tflm_sources.mk
TFLM_LIB   := $(SW_OUT)/libtflm.a
TFLM_FLAGS := -Ivendor/tflite_micro -Ivendor/flatbuffers/include -Ivendor/gemmlowp \
              -Ivendor/ruy -Ivendor/kissfft \
              -fno-asynchronous-unwind-tables -Wno-unused-parameter -Wnon-virtual-dtor \
              -DFLATBUFFERS_LOCALE_INDEPENDENT=0 -DTF_LITE_STATIC_MEMORY=1 -DTF_LITE_USE_CTIME
TFLM_OBJS  := $(addprefix $(SW_OUT)/tflm/,$(addsuffix .o,$(subst /,_,$(TFLM_SRCS))))
$(foreach s,$(TFLM_SRCS),$(eval $(call sw_cc_rule,$(s),$(SW_OUT)/tflm/$(subst /,_,$(s)).o,$(TFLM_FLAGS))))

$(TFLM_LIB): $(TFLM_OBJS)
	@mkdir -p $(dir $@)
	rm -f $@
	$(RV_PREFIX)ar rcs $@ $^

define sw_rule
$(1)_OBJDIR := $(SW_OUT)/$(1)
$(1)_FLAGS  := $$($(1)_COPTS) $$(addprefix -D,$$($(1)_DEFS)) $$(if $$($(1)_TFLM),$$(TFLM_FLAGS))
$(1)_ELF    := $(ELF_DIR)/$$($(1)_PKG)/$$($(1)_NAME).elf
$(1)_LD     := $$(or $$($(1)_LDSCRIPT),$(SW_OUT)/$(1)/link.ld)
$(1)_CRT    := $$(if $$($(1)_NOCRT),,$$(if $$($(1)_SEMIHOSTING),$$(CRT_SRCS_HTIF),$$(CRT_SRCS)))
$(1)_CRTDEF := $$(if $$($(1)_SEMIHOSTING),,-DSKIP_HTIF_SYMBOLS)
$(1)_OBJS   := $$(addprefix $$($(1)_OBJDIR)/crt_,$$(addsuffix .o,$$(notdir $$($(1)_CRT)))) \
               $$(addprefix $$($(1)_OBJDIR)/,$$(addsuffix .o,$$(subst /,_,$$($(1)_SRCS) $$($(1)_GENSRCS))))

$$($(1)_LD): toolchain/coralnpu_tcm.ld.tpl utils/gen_linker_script.py
	@mkdir -p $$(dir $$@)
	python3 utils/gen_linker_script.py --template $$< --out $$@ \
	  --itcm $$(word 1,$$($(1)_LDS)) --dtcm $$(word 2,$$($(1)_LDS)) --stack $$(word 3,$$($(1)_LDS)) \
	  --heap-size $$(word 4,$$($(1)_LDS)) --heap-location $$(word 5,$$($(1)_LDS))

$$(foreach s,$$($(1)_SRCS),$$(eval $$(call sw_cc_rule,$$(s),$$($(1)_OBJDIR)/$$(subst /,_,$$(s)).o,$$($(1)_FLAGS))))
$$(foreach s,$$($(1)_GENSRCS),$$(eval $$(call sw_cc_rule,$$(GEN_DIR)/$$(s),$$($(1)_OBJDIR)/$$(subst /,_,$$(s)).o,$$($(1)_FLAGS) -iquote $$(dir $$(GEN_DIR)/$$(s)))))
$$(foreach s,$$($(1)_CRT),$$(eval $$(call sw_cc_rule,$$(CRT_DIR)/$$(s),$$($(1)_OBJDIR)/crt_$$(s).o,$$($(1)_CRTDEF))))

$$($(1)_ELF): $$($(1)_OBJS) $$($(1)_LD) $$(if $$($(1)_TFLM),$$(TFLM_LIB))
	@mkdir -p $$(dir $$@)
	$$(RV_CXX) $$(SW_CXXFLAGS) $$($(1)_OBJS) $$(if $$($(1)_TFLM),$$(TFLM_LIB)) -o $$@ -Wl,-T,$$($(1)_LD) \
	  $$(if $$($(1)_SEMIHOSTING),$$(SW_LDFLAGS_HTIF),$$(SW_LDFLAGS_NANO))
	$$(RV_OBJCOPY) -O binary $$@ $$(basename $$@).bin
endef
$(foreach t,$(SW_TARGETS),$(eval $(call sw_rule,$(t))))

-include $(shell find $(SW_OUT) -name '*.d' 2>/dev/null)

SW_ELFS := $(foreach t,$(SW_TARGETS),$($(t)_ELF))
COCOTB_ELFS := $(foreach t,$(filter tests_cocotb_%,$(SW_TARGETS)),$($(t)_ELF))

.PHONY: elf elfs-cocotb list-elfs
list-elfs:
	@printf '%s\n' $(foreach t,$(SW_TARGETS),$($(t)_PKG)/$($(t)_NAME))

elf:
ifndef T
	$(error usage: make elf T=<package>/<name>, e.g. tests/cocotb/wfi_slot_0)
endif
	$(MAKE) $(ELF_DIR)/$(T).elf

elfs-cocotb: $(COCOTB_ELFS)

# Tests that drive gdb (core_mini_axi_debug) look it up as coralnpu_hw/toolchain/gdb.
# The toolchain's gdb links libmpfr.so.4 (the old Docker image symlinked it to
# libmpfr.so.6); provide that without touching system directories.
$(ELF_DIR)/toolchain/gdb: $(RV_PREFIX)gdb
	@mkdir -p $(dir $@) $(BUILD)/libcompat
	@for d in /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/lib64 /lib64; do \
	  if [ -e $$d/libmpfr.so.6 ]; then ln -sf $$d/libmpfr.so.6 $(BUILD)/libcompat/libmpfr.so.4; break; fi; done
	printf '#!/bin/sh\nLD_LIBRARY_PATH=%s:$$LD_LIBRARY_PATH exec %sgdb "$$@"\n' '$(BUILD)/libcompat' '$(RV_PREFIX)' > $@
	chmod +x $@

# Memory initialisation files for ROM images / FPGA block RAMs (the old enable_vmem output).
%.vmem: %.bin
	srec_cat $< -binary -byte-swap 4 -fill 0xff -within $< -binary -range-pad 4 -o $@ -vmem 32
