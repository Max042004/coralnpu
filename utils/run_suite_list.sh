#!/bin/bash
# Run every suite named in a list file (one per line) via run_cocotb_suites.sh.
#   utils/run_suite_list.sh build/rest_suites.txt build/suites_rest.txt
cd "$(dirname "$0")/.."
mapfile -t suites < "$1"
bash utils/run_cocotb_suites.sh "${suites[@]}" > "$2" 2>&1
