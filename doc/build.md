# Building CoralNPU with Bender + Make

The tools split the work the same way OpenHW/PULP projects do:

| Concern | Tool | Files |
|---|---|---|
| Third-party RTL and sources, file lists | [Bender](https://github.com/pulp-platform/bender) | `Bender.yml`, `vendor/`, `vendor/patches/`, `tools/Bender.yml` |
| Chisel → SystemVerilog, Chisel unit tests | sbt + firtool | `build.sbt`, `project/`, `hdl/chisel/src` |
| Orchestration | GNU Make | `Makefile`, `mk/*.mk` |
| Reproducible environment | Ubuntu 22.04 (container or OrbStack VM) | `utils/setup_vm.sh`, `utils/coralnpu.dockerfile` |

## Environment

Build on an Ubuntu 22.04 x86_64 machine. On macOS use an OrbStack Linux
machine: it sees the checkout directly, so edit on the Mac and build in the VM.

```bash
bash utils/setup_vm.sh                  # once: apt packages, bender, sbt, firtool,
                                        # verilator, RISC-V toolchain, python venv
export PATH="$HOME/bin:$PATH"           # (and TOOLS_DIR=... if not ~/tools)
orb -m <vm> -w "$PWD" make rtl DESIGN=CoreMiniAxi
```

`setup_vm.sh` pins Bender, sbt, firtool, Verilator 5.052, the prebuilt CoralNPU
RISC-V toolchain and a Python 3.11 venv (`requirements.txt`) under `~/tools`.
Single steps can be run with e.g. `bash utils/setup_vm.sh bender sbt`.
`utils/coralnpu.dockerfile` builds the same environment as a container image
(used by CI).

## Third-party sources

Small RTL and C++ dependencies are `vendor_package` entries in `Bender.yml`
(common_cells, cvfpu, RVVI, rocket-chip, riscv-tests, FreeRTOS, TFLite Micro,
MPACT, Spike, uvm-verilator, ...). The patched result is committed under
`vendor/`; patch series live in `vendor/patches/<pkg>/` and are applied with
`git apply`. To bump a revision or change a patch:

```bash
make vendor-update
```

Large unmodified host dependencies (abseil, SystemC, libsystemctlm-soc,
pybind11, antlr4, ELFIO, linenoise) are pinned in `tools/Bender.yml` and fetched
into `tools/.bender/`. Chisel BlackBoxes find vendored RTL as classpath resources
(`external/cvfpu/...`, `hdl/verilog/...`); `build.sbt` stages them.

## Generating RTL, Chisel tests

```bash
make list-designs
make rtl DESIGN=VmeCoreMiniAxi       # -> build/rtl/VmeCoreMiniAxi/VmeCoreMiniAxi.sv
make rtl-all
make chisel-test T=coralnpu.AluSpec  # one test class; omit T for everything
make flist                           # build/coralnpu.f from Bender.yml
```

## Verilator models, firmware and cocotb

```bash
make list-models
make model MODEL=core_mini_axi_model            # Verilate (generates the RTL first)
make elfs-cocotb                                # firmware for tests/cocotb (make list-elfs)
make elf T=tests/cocotb/wfi_slot_0              # a single firmware binary
make riscv-tests                                # riscv-tests ELFs (vendored, patched)
make cocotb-suite SUITE=tests_cocotb_core_mini_axi_sim_cocotb
make cocotb-test MODEL=core_mini_axi_model MODULE=tests.cocotb.core_mini_axi_sim TESTCASE=core_mini_axi_slow_bready
```

* `mk/designs.mk`, `mk/models.mk`, `mk/sw_targets.mk`, `mk/cocotb_suites.mk` and
  `mk/fpga_tests.mk` are the tables of designs, Verilated models, firmware
  targets, cocotb suites and SoC tests. Add a new test or firmware binary by
  adding an entry to the matching table.
* Firmware uses the prebuilt toolchain and `utils/gen_linker_script.py`.
  `SW_COPT=opt|dbg` selects the optimisation mode (default `fastbuild`).
  RVV template tests are expanded by `utils/expand_templates.py`
  (`mk/gen_sources.json`); TensorFlow Lite Micro is `libtflm.a` from the source
  list in `mk/tflm_sources.mk`.
* Python tests find build products through `coralnpu_test_utils/runfiles.py`
  (C++: `coralnpu_test_utils/runfiles.h`), which resolves `coralnpu_hw/<path>`
  in `build/runfiles/` (built artifacts), then in the source tree.
  `CORALNPU_RUNFILES` and `CORALNPU_ROOT` override the locations.
* `utils/cocotb_run.py` runs a cocotb module against a prebuilt Verilated model;
  `utils/run_cocotb_suites.sh` runs suites by name.

## Host-side C++: Spike, SystemC, MPACT

```bash
make host-deps absl systemc antlr4     # fetch with Bender, build with CMake
make spike spike-test                  # patched riscv-isa-sim + co-sim wrapper tests
make sim SIM=core_mini_axi_sim         # Verilator/SystemC testbench (make sim-list)
make sim-tests hwsim                   # other testbenches, libcoralnpu_simulator*.so
make mpact mpact-pytest                # MPACT ISS + python binding tests
make npusim-mobilenet                  # MPACT run of the MobileNet example
make nexus-loader fst-query            # host tools -> build/tools/
make tool-tests py-tests               # tool unit tests, python unit tests
```

The MPACT stack (mpact-sim, mpact-riscv, coralnpu-mpact; vendored and patched)
is compiled with clang by `mk/mpact_rules.mk`; the ISA / bin-format decoder and
ANTLR generator steps are scripts in `mk/mpact_cmds/`.

## FPGA SoC and UVM

```bash
make fpga-deps                                   # pinned OpenTitan + ISP sources
make chip-verilator VARIANT=vector_normal        # full-chip Verilator model (4 variants)
make fpga-sim-tests                              # clk / trivial_pass (SPI loader) / dma / highmem
make chip-nexus VARIANT=vector_normal TYPE=elab_only|synth_only|bitstream
make uvm-model [DUT=rvv|vme]                     # UVM testbench on Verilator (+ MPACT, Spike DPI)
make uvm-test ELFS="tests/cocotb/wfi_slot_0" [SPIKE=1]
make uvm-regression [SPIKE=1]                    # 821 firmware / riscv-tests ELFs
```

The SoC embeds OpenTitan IP whose cores are FuseSoC-native, so `fusesoc` is used
to resolve the file sets (and to drive Verilator/Vivado through edalize); Bender
and make own everything around it: pinned downloads, the generated Chisel
subsystem core, the search path and the flags (`mk/fpga.mk`). `make
uvm-model-vcs` (`mk/vcs.mk`) builds the UVM bench with Synopsys VCS; it needs a
licence and is untested.

## Continuous integration

`.github/workflows/ci.yml` builds `utils/coralnpu.dockerfile` and runs one Make
target per job; `main.yml` publishes the generated RTL. The same targets work
locally:

| Target | Runs |
|---|---|
| `make ci-lint` | `utils/run_linters.sh` (formatters, license headers, ...) |
| `make ci-rtl` | RTL for `RvvCoreMiniAxi` |
| `make ci-chisel` | all Chisel unit tests |
| `make ci-sim` | Spike, MPACT, C++ testbench, tool and python unit tests, FPGA SoC sim tests |
| `make ci-cocotb` | riscv-tests and every cocotb suite except the slow ones |
| `make ci-uvm` | UVM Verilator regression |

`ci-cocotb` skips `*_slow` suites, `rvv_matmul_cocotb_test` and
`cocotb_run_mobilenet_v1` (hours-long runs).

## Known limitations

Reproduced upstream problems, not introduced by the build system:

* `fpga/rom_test*` (ROM-boot images) do not link: `fpga/sw/rom_boot/crt0.S` uses
  `_stack_top`, `_data_load`, ... that neither `rom.ld` nor `rom_highmem.ld` define.
  The ROM boot variants of the SoC therefore have no working image.
* `coralnpu_soc.sv` instantiates `CoralNPUChiselSubsystem`, while the Chisel
  emitter names the matrix/highmem subsystems after their configuration; the make
  flow renames the top module when it stages the generated core.

Not verified: Vivado builds (the Vivado 2019.2 used for testing lacks the
xcvu13p device; the FuseSoC project setup itself works), VCS flows (need a
licence), the `*_slow` cocotb suites, `mobilenet_v1`, and the 2D `rvv_matmul`
cocotb test (times out).
