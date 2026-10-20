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
             -Wall -Werror -iquote $(ROOT) -I$(ROOT) \
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

# $(call sw_compile,<src path>,<obj>,<extra flags>)
define sw_cc_rule
$(2): $(1)
	@mkdir -p $$(dir $$@)
	$$(if $$(filter %.cc %.cpp,$(1)),$$(RV_CXX) $$(SW_CXXFLAGS),$$(RV_CC) $$(SW_CFLAGS)) $(3) -MMD -MP -c $$< -o $$@
endef

define sw_rule
$(1)_OBJDIR := $(SW_OUT)/$(1)
$(1)_ELF    := $(ELF_DIR)/$$($(1)_PKG)/$$($(1)_NAME).elf
$(1)_LD     := $(SW_OUT)/$(1)/link.ld
$(1)_CRT    := $$(if $$($(1)_NOCRT),,$$(if $$($(1)_SEMIHOSTING),$$(CRT_SRCS_HTIF),$$(CRT_SRCS)))
$(1)_CRTDEF := $$(if $$($(1)_SEMIHOSTING),,-DSKIP_HTIF_SYMBOLS)
$(1)_OBJS   := $$(addprefix $$($(1)_OBJDIR)/crt_,$$(addsuffix .o,$$(notdir $$($(1)_CRT)))) \
               $$(addprefix $$($(1)_OBJDIR)/,$$(addsuffix .o,$$(subst /,_,$$($(1)_SRCS))))

$$($(1)_LD): toolchain/coralnpu_tcm.ld.tpl utils/gen_linker_script.py
	@mkdir -p $$(dir $$@)
	python3 utils/gen_linker_script.py --template $$< --out $$@ \
	  --itcm $$(word 1,$$($(1)_LDS)) --dtcm $$(word 2,$$($(1)_LDS)) --stack $$(word 3,$$($(1)_LDS)) \
	  --heap-size $$(word 4,$$($(1)_LDS)) --heap-location $$(word 5,$$($(1)_LDS))

$$(foreach s,$$($(1)_SRCS),$$(eval $$(call sw_cc_rule,$$($(1)_PKG)/$$(s),$$($(1)_OBJDIR)/$$(subst /,_,$$(s)).o,$$($(1)_COPTS) $$(addprefix -D,$$($(1)_DEFS)))))
$$(foreach s,$$($(1)_CRT),$$(eval $$(call sw_cc_rule,$$(CRT_DIR)/$$(s),$$($(1)_OBJDIR)/crt_$$(s).o,$$($(1)_CRTDEF))))

$$($(1)_ELF): $$($(1)_OBJS) $$($(1)_LD)
	@mkdir -p $$(dir $$@)
	$$(RV_CXX) $$(SW_CXXFLAGS) $$($(1)_OBJS) -o $$@ -Wl,-T,$$($(1)_LD) \
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
