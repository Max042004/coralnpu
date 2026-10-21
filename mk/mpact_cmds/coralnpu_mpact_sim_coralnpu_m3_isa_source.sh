#!/bin/bash
# Code generation step for @coralnpu-mpact//sim:coralnpu_m3_isa_source
set -euo pipefail
rm -rf build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source && mkdir -p build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source build/mpact/gen/coralnpu-mpact/sim
ARR=(vendor/coralnpu-mpact/sim/coralnpu_m3.isa vendor/coralnpu-mpact/sim/coralnpu_v2.isa vendor/mpact-riscv/riscv/riscv_zvt.isa); DIRS=(); for f in "${ARR[@]}"; do DIRS+=("$(dirname "$f")"); DIRS+=("$(dirname "$(dirname "$f")")"); done; DIRS_STR=$(IFS=,; echo "${DIRS[*]}"); build/mpact/bin/mpact_sim_mpact_sim_decoder_decoder_gen ${ARR[0]} --prefix coralnpu_m3 --generator 1 --output_dir build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source --include "$DIRS_STR" --isa_name CoralNPUM3
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_decoder.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_decoder.h
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_decoder.cc build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_decoder.cc
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_enums.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_enums.h
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_enums.cc build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_enums.cc
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_encoder.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_encoder.h
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_isa_source/coralnpu_m3_encoder.cc build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_encoder.cc
