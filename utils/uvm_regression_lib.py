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

"""Helpers for running a UVM regression batch (used by uvm_batch_runner.py).

Building the model and choosing the firmware is done by make (mk/uvm.mk); this
module only formats the batch list for the testbench and parses its log output.
"""

import logging
import os
import re
import subprocess
from typing import Dict, List, Optional

from elftools.elf.elffile import ELFFile


def format_batch_entry(
    elf: str, tohost: int, entry: int, timeout: int, spike_log: str,
    target: str
) -> str:
    return "%s %08x %08x %d %s %s\n" % (
        elf, tohost, entry, timeout, spike_log, target
    )


def get_entry_point(elf_path: str) -> int:
    try:
        with open(elf_path, 'rb') as f:
            elf = ELFFile(f)
            return elf.header.e_entry
    except Exception as e:
        logging.error(f"Error reading ELF entry point: {e}")
        return 0


def get_tohost_addr(elf_path: str) -> Optional[int]:
    try:
        with open(elf_path, 'rb') as f:
            elf = ELFFile(f)
            symtab = elf.get_section_by_name('.symtab')
            if symtab:
                syms = symtab.get_symbol_by_name('tohost')
                if syms and len(syms) > 0:
                    return syms[0]['st_value']
    except Exception as e:
        logging.error(f"Error reading tohost addr: {e}")
    return None


DUT_CHOICES = ("rvv", "vme")


def run_uvm_batch(
    cmd: List[str],
    env: os._Environ,
    regression_log_path: str,
    logs_dir: str,
    test_info_map: Dict,
    cwd: Optional[str] = None
):
    current_target = None
    current_test_log = None
    current_reason = "None"
    results = []
    completed_targets = set()
    with open(regression_log_path, 'a') as f_log:
        process = subprocess.Popen(
            cmd,
            env=env,
            cwd=cwd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )

        for line in process.stdout:
            f_log.write(line)
            if current_test_log: current_test_log.write(line)

            # Detect start of test
            start_match = re.search(r'--- STARTING TEST: (.*?) ---', line)
            if start_match:
                current_target = start_match.group(1)
                current_err_count = 0
                logging.info(f"Running UVM for {current_target}...")

                log_path = os.path.join(
                    logs_dir, test_info_map[current_target]['safe_log']
                )
                if current_test_log: current_test_log.close()
                current_test_log = open(log_path, 'w')
                current_test_log.write(line)
                current_reason = "None"
                continue

            # Parse for failure reasons
            if current_target:
                # Prioritize the first error found for the summary
                err_match = re.search(
                    r"^\s*(UVM_(?:FATAL|ERROR)(?!.*:\s+0\s*$).*)$", line,
                    re.MULTILINE
                )
                if err_match:
                    err_msg = err_match.group(1).strip()
                    current_err_count += 1
                    if current_err_count <= 5:
                        logging.error(f"    {err_msg}")
                    elif current_err_count == 6:
                        logging.error(
                            f"    [Suppressing further repetitive error output for {current_target}...]"
                        )
                    if current_reason == "None":
                        current_reason = err_msg.replace(',', ';')
                    if current_err_count >= 100:
                        logging.error(
                            f"Aborting runaway test {current_target} due to excessive error limit (>= 100)."
                        )
                        process.kill()
                else:
                    # Match standard Verilog Error/Fatal/Assertion
                    verilog_err = re.search(
                        r"^(Error|Fatal|Runtime Error|Assertion failed):? (.*)$",
                        line, re.MULTILINE
                    )
                    if verilog_err:
                        err_msg = verilog_err.group(0).strip()
                        current_err_count += 1
                        if current_err_count <= 5:
                            logging.error(f"    {err_msg}")
                        elif current_err_count == 6:
                            logging.error(
                                f"    [Suppressing further repetitive error output for {current_target}...]"
                            )
                        if current_reason == "None":
                            current_reason = err_msg.replace(',', ';')
                        if current_err_count >= 100:
                            logging.error(
                                f"Aborting runaway test {current_target} due to excessive error limit (>= 100)."
                            )
                            process.kill()

                if "** UVM TEST PASSED **" in line:
                    logging.info(f"  Result: PASS - {current_reason}")
                    results.append({
                        "Target":
                        current_target,
                        "Status":
                        "PASS",
                        "Reason":
                        current_reason,
                        "Log Path":
                        os.path.join(
                            "logs", test_info_map[current_target]['safe_log']
                        )
                    })
                    completed_targets.add(current_target)
                    current_test_log.close()
                    current_test_log = None
                    current_target = None
                elif "** UVM TEST FAILED **" in line:
                    logging.info(f"  Result: FAIL - {current_reason}")
                    results.append({
                        "Target":
                        current_target,
                        "Status":
                        "FAIL",
                        "Reason":
                        current_reason,
                        "Log Path":
                        os.path.join(
                            "logs", test_info_map[current_target]['safe_log']
                        )
                    })
                    completed_targets.add(current_target)
                    current_test_log.close()
                    current_test_log = None
                    current_target = None

        process.wait()
        if current_test_log: current_test_log.close()

        # If process ended while a test was running, mark it as a crash
        if current_target:
            status = "FAIL"
            if current_reason == "None":
                current_reason = "Simulator exited early or crashed"
            logging.error(f"  Result: {status} - {current_reason}")
            results.append({
                "Target":
                current_target,
                "Status":
                status,
                "Reason":
                current_reason,
                "Log Path":
                os.path.join(
                    "logs", test_info_map[current_target]['safe_log']
                )
            })
            completed_targets.add(current_target)
            current_target = None
        elif process.returncode != 0 and not completed_targets:
            # Simulator exited or crashed before starting any test in this batch
            crashed_target = list(test_info_map.keys()
                                  )[0] if test_info_map else "unknown"
            if current_reason == "None":
                current_reason = f"Simulator startup failed (exit code {process.returncode})"
            logging.error(f"  Result: FAIL - {current_reason}")
            # Mark all pending targets in this batch as completed/failed
            for t in test_info_map.keys():
                results.append({
                    "Target":
                    t,
                    "Status":
                    "FAIL",
                    "Reason":
                    current_reason,
                    "Log Path":
                    os.path.join("logs", test_info_map[t]['safe_log'])
                })
                completed_targets.add(t)
        return results, completed_targets
