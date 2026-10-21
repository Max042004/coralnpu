#!/bin/bash
# Run one python test file with the environment the Make flow gives tests:
#   utils/run_py_test.sh path/to/test.py [args...]
# Firmware comes from build/runfiles, the MPACT python module from build/mpact/python.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-${TOOLS_DIR:-$HOME/tools}/venv/bin/python}"
export CORALNPU_ROOT="$ROOT"
export CORALNPU_RUNFILES="${CORALNPU_RUNFILES:-$ROOT/build/runfiles}"
export PYTHONPATH="$ROOT/build/mpact/python:$ROOT/sw/coralnpu_sim:$ROOT/utils:$ROOT${PYTHONPATH:+:$PYTHONPATH}"
cd "$ROOT"
exec "$PYTHON" "$@"
