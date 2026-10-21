# Simulation

## VCS Support

CoralNPU supports using VCS simulator. To enable VCS support, the following
environment variables need to be set:

```bash
export VCS_HOME=${PATH_TO_YOUR_VCS_HOME}
export LM_LICENSE_FILE=${YOUR_LICENSE_FILE}
```

`LD_LIBRARY_PATH` and `PATH` should also be updated.

```bash
export LD_LIBRARY_PATH="${VCS_HOME}"/linux64/lib
export PATH=$PATH:${VCS_HOME}/bin/
```

The VCS entry point is the UVM testbench: `make uvm-model-vcs` (see
[`mk/vcs.mk`](../mk/vcs.mk)) compiles `tests/uvm` with the same sources, defines
and co-simulation libraries as the Verilator model (`make uvm-model`). It needs a
VCS installation and licence and has not been exercised by the maintainers of the
Make build; the previous build system's other VCS rules (single-testbench
runner, cocotb flows) were not carried over.

For everything that does not need VCS use Verilator: [build.md](build.md).
