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

## Status of the migration

Done: RTL dependency management, Chisel generation and unit tests, tool
provisioning. Still on the former Bazel rules (to be ported): Verilator
simulation models, cocotb tests, RISC-V toolchain/firmware, MPACT, Spike,
SystemC, TFLite Micro, FPGA (FuseSoC `.core`) flows.
