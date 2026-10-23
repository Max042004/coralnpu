# Building on Windows (WSL2)

The build runs on Linux only (Ubuntu 22.04, x86_64). On Windows use WSL2; do
not build with native Windows tools.

> Status: these steps are written from the Linux setup and have **not** been
> run on a Windows machine yet. Report anything that differs.

## 1. One-time setup

In an administrator PowerShell:

```powershell
wsl --install -d Ubuntu-22.04
wsl --set-default-version 2
```

Reboot if asked, then open the *Ubuntu 22.04* terminal. Optional but useful
(`%UserProfile%\.wslconfig`), because Verilator and the C++ builds are heavy:

```ini
[wsl2]
memory=16GB
processors=8
```

Run `wsl --shutdown` after editing it.

## 2. Clone inside WSL (not under /mnt/c)

```bash
cd ~
git clone -b <branch> https://github.com/<your-fork>/coralnpu.git
cd coralnpu
```

Work in the Linux file system (`~/...`). A checkout under `/mnt/c` is slow, can
break file permissions and symlinks, and Windows path-length limits can hit the
vendored sources. Do **not** clone with Windows git and then copy the folder:
line endings (`.gitattributes` keeps scripts LF) and executable bits may be lost.

## 3. Install the tools and build

```bash
bash utils/setup_vm.sh               # apt packages, bender, sbt, firtool, verilator, toolchain, python venv
export PATH="$HOME/bin:$PATH"        # add to ~/.bashrc

make help
make rtl DESIGN=CoreMiniAxi          # smoke test: Chisel -> SystemVerilog
make demo-rvv-add                    # RVV vector add on MPACT and on the RTL
```

`setup_vm.sh` needs about 10 GB of disk and a network connection; the first
`make` of Verilator models and of MPACT takes a while.

## 4. Editing from Windows

Use VS Code with the *WSL* extension (`code .` inside the WSL terminal) or open
`\\wsl$\Ubuntu-22.04\home\<you>\coralnpu` from Windows tools.

## Known limitations

- Vivado: run it where it is installed (Linux server); the FPGA flows
  (`make chip-nexus ...`) were only tested up to FuseSoC project setup.
- TensorFlow wheels need AVX; WSL2 on a normal PC has it, but under emulation
  (e.g. x86 on Apple Silicon) it does not.
- Docker is an alternative: `utils/coralnpu.dockerfile` (not yet built or tested).
