# CoralNPU UVM Testbench

This document describes the structure and usage of the UVM testbench for
verifying the `RvvCoreMiniVerificationAxi` DUT.

## Overview

This testbench provides a basic UVM environment to:

* Instantiate the `RvvCoreMiniVerificationAxi` DUT.
* Connect AXI Master, AXI Slave, and IRQ interfaces to the DUT.
* Provide basic stimulus generation via UVM sequences.
* Include a simple reactive AXI Slave model.
* Load a binary file into the DUT's memory using backdoor access.
* Kick off the DUT execution using initial AXI writes.
* Check for basic test completion via DUT status signals (`halted`, `fault`) or
  a timeout.

## Prerequisites

* **Verilator (default):** The testbench runs on Verilator 5 with the vendored
  `uvm-verilator` UVM library (installed by `utils/setup_vm.sh`).
* **Synopsys VCS (optional):** `make uvm-model-vcs` builds a VCS model; it
  needs a licence and UVM 1.2 support, and is untested in CI.
* **CoralNPU Hardware Repository:** Access to the repository containing the
  `RvvCoreMiniVerificationAxi` source code is required to generate the DUT
  Verilog and the test binary.
* **Bender, sbt, firtool:** Used by `make` to generate the Verilog from the
  Chisel source (see `doc/build.md`).
* **RISC-V Toolchain:** A RISC-V toolchain compatible with the CoralNPU
  project is needed to generate the `.elf` file.
* **MPACT co-simulation:** MPACT comes from `vendor/` and is built by `make`;
  no environment variable is needed.

## Generating the Test Binary (program.elf)

The test program run by the DUT needs to be compiled to an elf format.

1. Navigate to the CoralNPU HW repository root:

   ```bash
   cd /path/to/your/coralnpu/hw/repo
   ```

2. Build the test program with Make:

   ```bash
   make elf T=tests/cocotb/tutorial/coralnpu_v2_program
   ```

3. The `coralnpu_v2_program.elf` file is generated in `build/runfiles/tests/cocotb/tutorial/`.
4. Copy this `coralnpu_v2_program.elf` file to the `bin/` directory of this UVM testbench structure (or update the `TEST_ELF` path in the `Makefile` or run command).

## Directory Structure

The testbench follows a standard UVM directory structure:

```text
.
├── common/                  # Common components
│   ├── coralnpu_axi_master/ # Files related to the TB acting as AXI Master
│   │   ├── coralnpu_axi_master_if.sv
│   │   └── coralnpu_axi_master_agent_pkg.sv
│   ├── coralnpu_axi_slave/  # Files related to the TB acting as AXI Slave
│   │   ├── coralnpu_axi_slave_if.sv
│   │   └── coralnpu_axi_slave_agent_pkg.sv
│   ├── coralnpu_irq/        # Files related to the IRQ/Control interface
│   │   ├── coralnpu_irq_if.sv
│   │   └── coralnpu_irq_agent_pkg.sv
│   └── transaction_item/    # Transaction item definitions
│       └── transaction_item_pkg.sv
├── env/                     # UVM Environment definition
│   └── coralnpu_env_pkg.sv
├── tb/                      # Top-level testbench module
│   └── coralnpu_tb_top.sv
├── tests/                   # UVM Tests and Sequences
│   └── coralnpu_test_pkg.sv
├── coralnpu_dv.f            # File list for compilation
└── bin/                     # Directory for test binaries
    └── program.elf          # (Needs to be generated and copied here)
```

## Running the Testbench using Make

The UVM testbench models and regressions are built and run with Make
(targets live in `mk/uvm.mk`; `DUT=rvv` is the default, `DUT=vme` selects the
VME toplevel).

**1. Compiling the Simulator Models:**

* **Verilator Model:**

  ```bash
  make uvm-model [DUT=rvv|vme]       # -> build/uvm/<dut>/Vuvm
  ```

* **VCS Model (requires a Synopsys licence, untested):**

  ```bash
  make uvm-model-vcs [DUT=rvv|vme]
  ```

**2. Running Tests:**

* **Specific binaries (optionally checked against Spike):**

  ```bash
  make uvm-test ELFS="tests/cocotb/nop_test tests/cocotb/math" [SPIKE=1] [TIMEOUT=100000]
  ```

* **Full Regression Suite** (firmware and riscv-tests listed in
  `mk/uvm_regression.tsv`, run in batches):

  ```bash
  make uvm-regression
  ```

**3. Running on the VME (Zvt) Toplevel:**

The bench can instantiate `VmeCoreMiniVerificationAxi` (RVV plus the Zvt
matrix extension) instead of `RvvCoreMiniVerificationAxi`. The VME model is
built with `+define+ZVT_ON`, which enables the Zvt datapath in the RTL, selects
the VME DUT in `coralnpu_tb_top`, and configures the MPACT co-simulator for the
M4 (Zvt) architecture. Rerun any of the above with `DUT=vme`:

```bash
make uvm-regression DUT=vme
```
