# Coral NPU

Coral NPU is a hardware accelerator for ML inferencing. Coral NPU is an Open
Source IP designed by Google Research and is freely available for integration
into ultra-low-power System-on-Chips (SoCs) targeting wearable devices such as
hearables, augmented reality (AR) glasses and smart watches.

Coral NPU is a neural processing unit (NPU), also known as an AI accelerator or
deep-learning processor. Coral NPU is based on the 32-bit RISC-V Instruction Set
Architecture (ISA).

Coral NPU includes three distinct processor components that work together:
matrix, vector (SIMD), and scalar.

![Coral NPU Archicture](doc/images/arch_data_flow.png)
[Coral NPU Architecture Datasheet](https://developers.google.com/coral/guides/hardware/datasheet)

## Coral NPU Features

Coral NPU offers the following top-level feature set:

* RV32IMF_Zve32x RISC-V instruction set (specifically `rv32imf_zve32x_zicsr_zifencei_zbb`)
* 32-bit address space for applications and operating system kernels
* Four-stage processor, in-order dispatch, out-of-order retire
* Four-way scalar, two-way vector dispatch
* 128-bit SIMD, 256-bit (future) pipeline
* 8 KB ITCM memory (tightly-coupled memory for instructions)
* 32 KB DTCM memory (tightly-coupled memory for data)
* Both memories are single-cycle-latency SRAM, more efficient than cache memory
* AXI4 bus interfaces, functioning as both manager and subordinate, to interact
  with external memory and allow external CPUs to configure Coral NPU

## System Requirements

* Linux x86_64 (Ubuntu 22.04 is what the build is verified on)
* [Bender](https://github.com/pulp-platform/bender), GNU Make, JDK 17 + sbt,
  Verilator 5.052, Python 3.11, and the CoralNPU RISC-V toolchain

`utils/setup_vm.sh` installs all of them (pinned versions) on a bare Ubuntu
22.04 machine or VM; see [coralnpu.dockerfile](utils/coralnpu.dockerfile) for the
equivalent container. Our CI systems run builds and tests in that image.
See [doc/build.md](doc/build.md) for the build system in detail. On Windows use
WSL2: [doc/windows.md](doc/windows.md).

## Verification & Testing

For details on our testing methodologies and how to run or write tests, see the
corresponding test READMEs:

* [Cocotb Tests (RTL & Netlist simulation)](tests/cocotb/README.md)
* [UVM Testbench (Co-simulation)](tests/uvm/README.md)

## Quick Start

```bash
# One-time: install the toolchain (Ubuntu 22.04), then put ~/bin on PATH
bash utils/setup_vm.sh
export PATH="$HOME/bin:$PATH"

# Ensure that a test suite passes (generates the RTL, Verilates it, builds the
# firmware and runs the cocotb tests)
make cocotb-suite SUITE=tests_cocotb_core_mini_axi_sim_cocotb

# Build a binary
make elf T=examples/coralnpu_v2_hello_world_add_floats

# Build the Simulator (non-RVV for shorter build time):
make sim SIM=core_mini_axi_sim

# Run the binary on the simulator:
build/sim/core_mini_axi_sim --binary build/runfiles/examples/coralnpu_v2_hello_world_add_floats.elf
```

`make help` lists the main targets.

![](doc/images/Coral_Logo_200px-2x.png)
