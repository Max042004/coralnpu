#!/bin/bash
# Run several cocotb suites and print one summary line per suite.
#   utils/run_cocotb_suites.sh <suite> [<suite>...]     (names from `make list-suites`)
# Logs go to build/cocotb_logs/<suite>.log. Exit status is non-zero if any suite failed.
set -u
cd "$(dirname "$0")/.."
mkdir -p build/cocotb_logs
rc=0
for s in "$@"; do
  log="build/cocotb_logs/$s.log"
  if make cocotb-suite SUITE="$s" > "$log" 2>&1; then
    status=PASS
  else
    status=FAIL; rc=1
  fi
  summary=$(grep -E '^cocotb: ' "$log" | tail -1)
  printf '%-4s %-80s %s\n' "$status" "$s" "${summary:-no cocotb summary (build error?)}"
done
exit $rc
