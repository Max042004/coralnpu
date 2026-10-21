#!/bin/bash
# One-off: ask TFLM's own Makefile which sources make up the micro library, so
# we can freeze that list in mk/tflm_sources.mk (utils/extract_tflm.py).
#   utils/tflm_list_sources.sh <scratch dir>
set -euo pipefail
W="$1"
mkdir -p "$W"
cd "$W"
if [ ! -d tflite-micro ]; then
  git clone -q https://github.com/tensorflow/tflite-micro.git
fi
cd tflite-micro
git checkout -q b75c6ff4e2270047f2b48fa01f833c8101c31f43
make -f tensorflow/lite/micro/tools/make/Makefile list_library_sources > "$W/lib_sources.txt" 2> "$W/lib_sources.err" || true
wc -l "$W/lib_sources.txt"
tail -5 "$W/lib_sources.err"
