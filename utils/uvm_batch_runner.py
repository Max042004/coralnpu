#!/usr/bin/env python3
#
# This script acts as a backend for running UVM regression tests (see mk/uvm.mk)
#
# Env vars (set by `make uvm-test` / `make uvm-regression`):
#   UVM_MODEL_RLOCATION: rlocation path of the model binary
#   UVM_CORALNPU_ELFS: newline-separated "<elf path>\t<label>\t<timeout>\t<spike True/False>" lines (or UVM_CORALNPU_ELFS_FILE: a file of such lines)
#   UVM_SPIKE_RLOCATION: rlocation path of the spike binary, if any

from __future__ import annotations

import csv
import logging
import os
import shutil
import sys
import tempfile
from pathlib import Path
from typing import IO, Optional, TypedDict

from coralnpu_test_utils import runfiles
from uvm_regression_lib import (
    format_batch_entry,
    get_entry_point,
    get_tohost_addr,
    run_uvm_batch,
)


class TestInfo(TypedDict):
    """Metadata describing a single regression test."""

    elf: str
    tohost: int
    entry: int
    timeout: int
    spike: str
    safe_log: str


TestInfoMap = dict[str, TestInfo]


def write_entry(
    batch_file: IO[str],
    elf_path: Path,
    log_name: str,
    label: str,
    timeout: int,
    spike: bool,
    test_info_map: TestInfoMap,
) -> None:
    entry = get_entry_point(str(elf_path))
    tohost = get_tohost_addr(str(elf_path))
    if tohost is None:
        tohost = 0xFFFFFFFF

    spike_option = "SPIKE" if spike else "NONE"

    batch_file.write(
        format_batch_entry(
            str(elf_path),
            tohost,
            entry,
            timeout,
            spike_option,
            label,
        )
    )

    test_info_map[label] = TestInfo(
        elf=str(elf_path.resolve()),
        tohost=tohost,
        entry=entry,
        timeout=timeout,
        spike=spike_option,
        safe_log=log_name,
    )


def label_to_fname(label: str) -> str:
    return label.replace("//", "").replace(":", "_").replace("/", "_")


def build_batch_file(
    batch_path: Path, coralnpu_elfs: list[tuple[Path, str, int, bool]],
    spike: bool
) -> TestInfoMap:
    test_info_map: TestInfoMap = {}
    with batch_path.open("w") as batch_file:
        for elf_path, label, timeout, enable_spike in coralnpu_elfs:
            write_entry(
                batch_file,
                elf_path,
                f"{label_to_fname(label)}.log",
                label,
                timeout,
                spike if enable_spike else False,
                test_info_map,
            )

    return test_info_map


def main():
    r = runfiles.Create()
    model_rloc = os.environ["UVM_MODEL_RLOCATION"]
    model = r.Rlocation(model_rloc)
    if not model or not os.path.isfile(model):
        sys.exit(f"ERROR: model not found: {model_rloc}")

    spike_env = os.environ.get("UVM_ENABLE_SPIKE")
    if spike_env is not None:
        spike = spike_env in ("1", "true", "True")
    else:
        spike = bool(os.environ.get("UVM_SPIKE_RLOCATION", ""))

    coralnpu_elfs = []
    # A full regression (hundreds of ELFs) does not fit in one environment variable;
    # UVM_CORALNPU_ELFS_FILE names a file with the same tab-separated lines.
    elf_list_file = os.environ.get("UVM_CORALNPU_ELFS_FILE")
    elf_lines = (Path(elf_list_file).read_text() if elf_list_file
                 else os.environ.get("UVM_CORALNPU_ELFS", ""))
    for line in elf_lines.splitlines():
        rloc, label, timeout, enable_spike = line.split("\t")
        coralnpu_elfs.append((
            Path(r.Rlocation(rloc)), label, int(timeout),
            enable_spike == "True"
        ))

    results_tmp = tempfile.TemporaryDirectory()
    results_dir_path = Path(results_tmp.name)
    batch_path = results_dir_path / "uvm_batch_list.txt"
    log_path = results_dir_path / "logs"
    log_path.mkdir()
    regression_log_path = log_path / "regression.log"
    test_info_map = build_batch_file(batch_path, coralnpu_elfs, spike)

    logging.info("Starting UVM regression...")
    cmd = [
        model,
        "+UVM_TESTNAME=coralnpu_regression_test",
        "+UVM_VERBOSITY=UVM_LOW",
        f"+REGRESSION_LIST={batch_path}",
        "+TEST_ELF=dummy",
        "+TEST_TIMEOUT=100000",
        "+MISA_VALUE='h40801120'",
    ]
    results, _ = run_uvm_batch(
        cmd,
        os.environ.copy(),
        regression_log_path.as_posix(),
        log_path.as_posix(),
        test_info_map,
    )

    with open(results_dir_path / "uvm_results.csv", "w",
              newline="") as csvfile:
        fieldnames = ["Target", "Status", "Reason", "Log Path"]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        for row in results:
            writer.writerow(row)

    any_failed = any(r["Status"] != "PASS" for r in results)
    if any_failed:
        num_failed = sum(1 for r in results if r["Status"] != "PASS")
        logging.error(f"Regression FAILED: {num_failed} tests failed.")
        sys.exit(1)

    logging.info(f"Regression PASSED: {len(results)} tests total.")

    test_undeclared_outputs = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR")
    if test_undeclared_outputs:
        shutil.copytree(
            results_dir_path.as_posix(),
            (Path(test_undeclared_outputs) /
             "uvm_regression_batch").as_posix(),
        )


if __name__ == "__main__":
    main()
