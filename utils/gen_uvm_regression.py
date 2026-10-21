#!/usr/bin/env python3
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generate mk/uvm_regression.tsv: the firmware the UVM regression runs.

Mirrors collect_coralnpu_elfs() / collect_coralnpu_riscv_tests() of the Bazel
build: every firmware binary that uses the default linker script and is not on
the UVM denylist, plus the riscv-tests ELFs. Columns: ELF path relative to
build/runfiles, Bazel-style label, timeout (cycles), run Spike co-sim (True/False).

The denylists and timeouts come from tests/uvm/denylist.py;
the firmware table from mk/sw_targets.mk. Re-run after either changes:

    python3 utils/gen_uvm_regression.py
"""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent


def load_py(path):
    ns = {}
    exec(compile(path.read_text(), str(path), "exec"), ns)
    return ns


def main():
    deny = load_py(ROOT / "tests/uvm/denylist.py")
    # SPIKE_DENYLIST / DENYLIST / TIMEOUT_MAP / is_uvm_denylisted
    is_denied = deny["is_uvm_denylisted"]
    timeout_map = deny["TIMEOUT_MAP"]
    spike_deny = set(deny.get("SPIKE_DENYLIST", []))

    mk = (ROOT / "mk/sw_targets.mk").read_text()
    targets = re.findall(r"^SW_TARGETS \+= (\S+)$", mk, re.M)
    fields = {}
    for line in mk.splitlines():
        m = re.match(r"^(\S+?)_(PKG|NAME|LDSCRIPT|LDS)\s*:=\s*(.*)$", line)
        if m:
            fields.setdefault(m.group(1), {})[m.group(2)] = m.group(3).strip()

    # Only packages that called collect_coralnpu_elfs() contribute firmware.
    packages = {"examples", "fpga", "hw_sim", "sw/opt/litert-micro/test", "tests/cocotb",
                "tests/cocotb/coralnpu_isa", "tests/cocotb/csr_test", "tests/cocotb/exceptions",
                "tests/cocotb/freertos_app", "tests/cocotb/rvv", "tests/cocotb/rvv/arithmetics",
                "tests/cocotb/rvv/load_store", "tests/cocotb/rvv/ml_ops",
                "tests/cocotb/rvv/ml_ops/static_reference_tests", "tests/cocotb/rvv/rvv_opt",
                "tests/cocotb/tutorial", "tests/cocotb/tutorial/counters",
                "tests/cocotb/tutorial/tfmicro", "tests/npusim_examples", "tests/verilator_sim"}
    rows = []
    for t in targets:
        f = fields.get(t, {})
        if "LDSCRIPT" in f:
            continue            # custom linker script: not a UVM regression target
        if f.get("LDS") != "8 32 128 - DTCM":
            # collect_coralnpu_elfs() only picks binaries whose generated linker script is
            # named after the target, i.e. the default 8KB ITCM / 32KB DTCM / 128B stack
            # configuration the UVM DUT is built for.
            continue
        pkg, name = f["PKG"], f["NAME"]
        if pkg not in packages:
            continue
        label = f"//{pkg}:{name}"
        if is_denied(label):
            continue
        rows.append((f"{pkg}/{name}.elf", label))

    defs = load_py(ROOT / "third_party/riscv-tests/defs.bzl")
    for var in ("OUT_BINARIES_RV32UI", "OUT_BINARIES_RV32UM", "OUT_BINARIES_RV32UF",
                "OUT_BINARIES_RV32UZBB"):
        suffix = var.split("RV32")[1].lower()
        for b in defs[var]:
            if "fence_i" in b:
                continue
            label = f"//third_party/riscv-tests:{b}"
            if is_denied(label):
                continue
            rows.append((f"third_party/riscv-tests/copy_riscv_tests_rv32{suffix}/"
                         f"riscv_tests_rv32{suffix}/isa/{b}", label))

    out = []
    for path, label in rows:
        out.append("\t".join([path, label, str(timeout_map.get(label, 100000)),
                              "False" if label in spike_deny else "True"]))
    (ROOT / "mk/uvm_regression.tsv").write_text("\n".join(out) + "\n")
    print(f"{len(out)} UVM regression ELFs")


if __name__ == "__main__":
    main()
