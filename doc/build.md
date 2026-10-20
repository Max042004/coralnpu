# Building CoralNPU with Bender + Make

This replaces the former Bazel build. The tools split the work the same way
OpenHW/PULP projects do:

| Concern | Tool | Files |
|---|---|---|
| Third-party RTL, file lists | [Bender](https://github.com/pulp-platform/bender) | `Bender.yml`, `vendor/`, `vendor/patches/` |
| Chisel → SystemVerilog, Chisel unit tests | sbt + firtool | `build.sbt`, `project/`, `hdl/chisel/src` |
| Orchestration | GNU Make | `Makefile`, `mk/designs.mk` |
| Reproducible environment | OrbStack Linux VM | `utils/setup_vm.sh` |

## Environment (OrbStack)

Build inside an Ubuntu 22.04 x86_64 OrbStack machine; it sees the macOS
checkout directly, so edit on the Mac and build in the VM:

```bash
orb -m vivado -w "$PWD" bash utils/setup_vm.sh      # once: bender, sbt, firtool, verilator
orb -m vivado -w "$PWD" env PATH="$HOME/bin:$PATH" make rtl DESIGN=CoreMiniAxi
```

`setup_vm.sh` pins Bender, sbt, firtool (`llvm-firtool` from Maven, same
version as the old Bazel build) and Verilator 5.052 under `~/tools`.

## Third-party RTL

`common_cells`, `fpu_div_sqrt_mvp`, `cvfpu` and `RVVI` are declared as
`vendor_package` entries in `Bender.yml`, pinned to the same commits as before.
The patched result is committed under `vendor/`; the patch series live in
`vendor/patches/<pkg>/` and are applied with `git apply`. To bump a revision
or change a patch:

```bash
make vendor-update
```

Chisel BlackBoxes still find these files as classpath resources
(`external/cvfpu/...`, `hdl/verilog/...`); `build.sbt` stages them from
`vendor/` and `hdl/verilog/`, so the Scala sources are unchanged.

## Generating RTL

```bash
make list-designs
make rtl DESIGN=VmeCoreMiniAxi       # -> build/rtl/VmeCoreMiniAxi/VmeCoreMiniAxi.sv
make rtl-all
```

`mk/designs.mk` holds the generator class and flags for every design (it was
extracted from the former `chisel_cc_library` targets).

## Tests and file lists

```bash
make chisel-test T=coralnpu.AluSpec   # one test class; omit T for everything
make flist                            # build/coralnpu.f from Bender.yml
```

## Verilator models, firmware and cocotb

```bash
make list-models
make model MODEL=core_mini_axi_model            # Verilate (generates the RTL first)
make elfs-cocotb                                # firmware for tests/cocotb (make list-elfs)
make riscv-tests                                # riscv-tests ELFs (vendored, patched)
make cocotb-suite SUITE=tests_cocotb_core_mini_axi_sim_cocotb
make cocotb-test MODEL=core_mini_axi_model MODULE=tests.cocotb.core_mini_axi_sim TESTCASE=core_mini_axi_slow_bready
```

* `mk/models.mk`, `mk/sw_targets.mk`, `mk/cocotb_suites.mk` are tables extracted
  from the former Bazel targets by `utils/extract_*.py` (kept for regeneration
  while Bazel files still exist).
* Firmware uses the prebuilt toolchain (`utils/setup_vm.sh toolchain`) and
  `utils/gen_linker_script.py` (port of `rules/linker.bzl`). `SW_COPT=opt|dbg`
  selects the optimisation mode (default is Bazel's `fastbuild`).
* The tests are unchanged: `utils/pyshim/` provides a stand-in for
  `bazel_tools.tools.python.runfiles` that resolves `coralnpu_hw/<path>` in
  `build/runfiles/` (built artifacts) and then in the source tree.
* `utils/cocotb_run.py` replaces `rules/cocotb_wrapper.py`.

Verified on the `vivado` OrbStack VM: `core_mini_axi_sim` passes 30/30 on both
`core_mini_axi_model` and `rvv_core_mini_axi_model`; a TL-UL suite also passes.

## Status of the migration

Done: RTL dependency management, Chisel generation and unit tests, tool
provisioning, Verilator cocotb models, firmware ELFs (491 targets, see
`mk/sw_targets.mk`), riscv-tests, cocotb runner.

Still on the former Bazel rules: firmware that needs extra libraries or
generated sources (RVV arithmetic templates, `sw/utils`, FreeRTOS, TFLite
Micro; listed as UNSUPPORTED in `mk/sw_targets.mk`), MPACT, Spike, SystemC,
VCS/UVM flows, FPGA (FuseSoC `.core`) flows, lint, and the Bazel files
themselves.
