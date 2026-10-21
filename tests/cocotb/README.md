# CoralNPU Cocotb Verification Environment

This directory contains the Cocotb verification environment and tests for the CoralNPU processor subsystems.

## Running Tests

Cocotb tests run against a Verilated model of the design. The Makefile builds
everything they need (RTL, model, firmware) on demand:

```bash
make list-suites                                   # suites and their models
make cocotb-suite SUITE=tests_cocotb_core_mini_axi_sim_cocotb
make cocotb-suite SUITE=... TESTCASE=core_mini_axi_slow_bready   # one testcase
make ci-cocotb                                     # everything CI runs
```

A suite is a Python test module plus the Verilated model it runs on (see
`mk/cocotb_suites.mk` and `mk/models.mk`); `utils/cocotb_run.py` is the runner.
Firmware the tests load is staged under `build/runfiles/` and found through
`coralnpu_test_utils/runfiles.py`.

---

## Synopsys VCS

The VCS flavours of these cocotb flows (single-step and split compile/run for
gate-level power analysis) were part of the previous build system and are not
ported; only the UVM testbench has a VCS entry point (`make uvm-model-vcs`,
unexercised -- see `mk/vcs.mk`).

---

## Gemma Model Tensor Extraction (Manual Utility)

[`tests/cocotb/dump_gemma_tensors.py`](dump_gemma_tensors.py) extracts reference Attention and RMSNorm tensors from `google/gemma-3-270m-it` on Hugging Face for local kernel evaluation.

### Why This Tool is Run Separately

Downloading pre-trained weights from Hugging Face requires public internet access
and the PyTorch and Transformers wheels are several gigabytes, so this utility
is not part of the standard environment (`requirements.txt`) or of CI. Automated
tests use deterministic synthetic test vectors instead.

### Manual Execution Path

To run the extraction utility manually:

```bash
# 1. Set up a virtual environment
python3 -m venv .venv
source .venv/bin/activate

# 2. Install pinned dependencies
pip install -r third_party/gemma_requirements.txt

# 3. Extract sample tensors
python3 tests/cocotb/dump_gemma_tensors.py --out_dir tests/cocotb/rvv/ml_ops/gemma_kernels/test_data
```

When present in `test_data/`, these `.npy` tensors are automatically loaded by [`rvv_flashattention_cocotb_test.py`](rvv/ml_ops/gemma_kernels/cocotb_tests/rvv_flashattention_cocotb_test.py).
