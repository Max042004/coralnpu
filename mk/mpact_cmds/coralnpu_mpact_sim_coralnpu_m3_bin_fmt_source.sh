#!/bin/bash
# Code generation step for @coralnpu-mpact//sim:coralnpu_m3_bin_fmt_source
set -euo pipefail
rm -rf build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source && mkdir -p build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source build/mpact/gen/coralnpu-mpact/sim
ARR=(vendor/coralnpu-mpact/sim/coralnpu_m3.bin_fmt build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_decoder.h vendor/coralnpu-mpact/sim/coralnpu_v2.bin_fmt vendor/mpact-riscv/riscv/riscv_zvt.bin_fmt); DIRS=(); for f in "${ARR[@]}"; do DIRS+=("$(dirname "$f")"); DIRS+=("$(dirname "$(dirname "$f")")"); done; DIRS_STR=$(IFS=,; echo "${DIRS[*]}"); build/mpact/bin/mpact_sim_mpact_sim_decoder_bin_format_gen ${ARR[0]}  --prefix coralnpu_m3 --output_dir build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source --include "$DIRS_STR" --decoder_name CoralNPUM3
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source/coralnpu_m3_bin_decoder.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_bin_decoder.h
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source/coralnpu_m3_bin_decoder.cc build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_bin_decoder.cc
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source/coralnpu_m3_bin_encoder.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_bin_encoder.h
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source/coralnpu_m3_bin_encoder.cc build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_bin_encoder.cc
cp build/mpact/tmp/coralnpu_mpact_sim_coralnpu_m3_bin_fmt_source/coralnpu_m3_bin_types.h build/mpact/gen/coralnpu-mpact/sim/coralnpu_m3_bin_types.h
