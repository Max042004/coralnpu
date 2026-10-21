#!/bin/bash
set -e

# Runs the ISP camera demo on the SoC Verilator model:
#   ./run_isp_cam_sim.sh [--trace] [--run_time SECONDS]
# This is a thin wrapper around `make isp-cam-sim` (see mk/fpga.mk).

TRACE=""
RUN_TIME=10

while [[ $# -gt 0 ]]; do
  case $1 in
    --trace)
      TRACE=1
      echo "Tracing enabled. Waveform will be saved to build/fpga/run/isp_cam/isp_cam_trace.fst"
      shift
      ;;
    --run_time)
      RUN_TIME="$2"
      echo "Run time set to ${RUN_TIME}s"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

cd "$(dirname "$0")"
exec make isp-cam-sim RUN_TIME="${RUN_TIME}" TRACE="${TRACE}"
