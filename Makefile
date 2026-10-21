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

# CoralNPU build entry point (Bender + Make + sbt). See doc/build.md.
#
#   make vendor-update           re-fetch + re-patch vendor/ (maintainers; vendor/ is committed)
#   make rtl DESIGN=CoreMiniAxi  generate SystemVerilog for one design
#   make rtl-all                 generate every design in mk/designs.mk
#   make list-designs
#   make flist                   Bender file list for the hand-written RTL
#   make model MODEL=core_mini_axi_model   Verilate a cocotb model (make list-models)
#   make cocotb-test MODEL=... MODULE=tests.cocotb.core_mini_axi_sim [TESTCASE=...]
#   make cocotb-suite SUITE=tests_cocotb_core_mini_axi_sim_cocotb   (make list-suites)
#   make riscv-tests                       Build the riscv-tests ELFs
#   make elf T=tests/cocotb/wfi_slot_0     Build a firmware ELF (make list-elfs, elfs-cocotb)
#   make chisel-test T=coralnpu.AluSpec   run one Chisel test class (omit T for all)
#
# Tools (bender, sbt, firtool, verilator) are installed by utils/setup_vm.sh.

SHELL     := /bin/bash
ROOT      := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD     ?= $(ROOT)/build
BENDER    ?= bender
SBT       ?= sbt
FIRTOOL   ?= $(shell command -v firtool 2>/dev/null)

# Chisel (firtool-resolver) looks for the firtool binary in this directory.
ifneq ($(FIRTOOL),)
export CHISEL_FIRTOOL_PATH := $(dir $(FIRTOOL))
endif

include mk/designs.mk
include mk/model.mk
include mk/sw.mk
include mk/riscv_tests.mk
include mk/spike.mk

.DEFAULT_GOAL := help
.PHONY: help vendor-update flist rtl rtl-all list-designs chisel-compile chisel-test clean

help:
	@sed -n '/^# CoralNPU build entry point/,/^# Tools/p' $(firstword $(MAKEFILE_LIST)) | sed 's/^# \{0,1\}//'

# ---- Third-party RTL (Bender) ------------------------------------------------

# vendor/ is committed, as is usual for Bender vendor_package; this target is
# only needed after changing a pinned revision in Bender.yml or a patch.
VENDOR_PKGS := common_cells fpu_div_sqrt_mvp cvfpu RVVI rocket_chip diplomacy riscv-tests freertos tflite_micro flatbuffers gemmlowp ruy kissfft riscv-isa-sim

vendor-update:
	rm -rf $(addprefix vendor/,$(VENDOR_PKGS))
	$(BENDER) vendor init

flist:
	@mkdir -p $(BUILD)
	$(BENDER) script flist > $(BUILD)/coralnpu.f
	@echo "wrote $(BUILD)/coralnpu.f"

# ---- Chisel -> SystemVerilog -------------------------------------------------

list-designs:
	@printf '%s\n' $(DESIGNS)

chisel-compile:
	$(SBT) compile

SCALA_SRCS := $(shell find hdl/chisel -name '*.scala') build.sbt

define design_rule
$(BUILD)/rtl/$(1)/$$($(1)_SV): $$(SCALA_SRCS)
	@mkdir -p $(BUILD)/rtl/$(1)
	$(SBT) "runMain $$($(1)_EMIT) --target-dir=$(BUILD)/rtl/$(1) $$($(1)_FLAGS)"
endef
$(foreach d,$(DESIGNS),$(eval $(call design_rule,$(d))))

ALL_RTL := $(foreach d,$(DESIGNS),$(BUILD)/rtl/$(d)/$($(d)_SV))

rtl:
ifndef DESIGN
	$(error usage: make rtl DESIGN=<name>; see `make list-designs`)
endif
	$(MAKE) $(BUILD)/rtl/$(DESIGN)/$($(DESIGN)_SV)

# sbt boots slowly, so rtl-all drives every generator from a single sbt session.
rtl-all:
	@mkdir -p $(addprefix $(BUILD)/rtl/,$(DESIGNS))
	$(SBT) $(foreach d,$(DESIGNS),"runMain $($(d)_EMIT) --target-dir=$(BUILD)/rtl/$(d) $($(d)_FLAGS)")

# ---- Chisel unit tests -------------------------------------------------------

chisel-test:
ifdef T
	$(SBT) "testOnly $(T)"
else
	$(SBT) test
endif

clean:
	rm -rf $(BUILD) target project/target project/project
