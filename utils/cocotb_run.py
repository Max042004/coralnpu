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

"""Run a cocotb test module against a pre-built Verilator model.

The model is the executable produced by `make model DESIGN=...`.

    cocotb_run.py --model build/model/CoreMiniAxi/CoreMiniAxi \
        --hdl_toplevel CoreMiniAxi \
        --test_module tests.cocotb.core_mini_axi_sim [--testcase NAME]
"""

import argparse
import os
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--model", required=True, help="Verilated cocotb model")
    p.add_argument("--hdl_toplevel", required=True)
    p.add_argument("--test_module", required=True, nargs="+",
                   help="module(s), e.g. tests.cocotb.core_mini_axi_sim")
    p.add_argument("--testcase", default=None)
    p.add_argument("--seed", default=None)
    p.add_argument("--waves", action="store_true")
    p.add_argument("--results_xml", default="results.xml")
    p.add_argument("--workdir", default=None,
                   help="run directory (default: next to the model)")
    args = p.parse_args()

    model = pathlib.Path(args.model).resolve()
    if not model.exists():
        sys.exit(f"model not found: {model} (run `make model`)")

    # Make the repo importable and let tests find firmware via coralnpu_test_utils/runfiles.py.
    runfiles = ROOT / "build" / "runfiles"
    env_runfiles = os.pathsep.join(
        filter(None, [os.environ.get("CORALNPU_RUNFILES", ""), str(runfiles)]))
    os.environ["CORALNPU_RUNFILES"] = env_runfiles
    os.environ["CORALNPU_ROOT"] = str(ROOT)
    for d in (str(ROOT),):
        if d not in sys.path:
            sys.path.insert(0, d)
    # cocotb_tools.runner hands sys.path to the simulator process as PYTHONPATH.
    os.environ["PYTHONPATH"] = os.pathsep.join(
        sys.path + [os.environ.get("PYTHONPATH", "")])

    from cocotb_tools.check_results import get_results
    from cocotb_tools.runner import get_runner

    runner = get_runner("verilator")
    run_dir = pathlib.Path(args.workdir or model.parent).resolve()
    run_dir.mkdir(parents=True, exist_ok=True)

    extra_env = {}
    if args.testcase:
        extra_env["COCOTB_TEST_FILTER"] = args.testcase

    results = runner.test(
        hdl_toplevel=args.hdl_toplevel,
        hdl_toplevel_lang="verilog",
        test_module=args.test_module,
        build_dir=str(model.parent),
        test_dir=str(run_dir),
        results_xml=args.results_xml,
        seed=args.seed,
        waves=args.waves or None,
        extra_env=extra_env,
    )
    num_tests, num_failed = get_results(results)
    import xml.etree.ElementTree as ET
    cases = list(ET.parse(results).getroot().iter("testcase"))
    skipped = sum(1 for c in cases if c.find("skipped") is not None)
    ran = len(cases) - skipped
    print(f"cocotb: ran {ran}, failed {num_failed}, skipped {skipped}")
    sys.exit(1 if (num_failed > 0 or ran == 0) else 0)


if __name__ == "__main__":
    main()
