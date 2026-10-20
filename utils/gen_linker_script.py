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

"""Expand toolchain/coralnpu_tcm.ld.tpl (port of rules/linker.bzl)."""

import argparse

ITCM_DEFAULT = 8
DTCM_DEFAULT = 32
DTCM_ORIGIN_DEFAULT = "0x00010000"
DTCM_ORIGIN_HIGHMEM = "0x00100000"
STACK_DEFAULT = 128


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--template", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--itcm", type=int, default=ITCM_DEFAULT, help="KBytes")
    p.add_argument("--dtcm", type=int, default=DTCM_DEFAULT, help="KBytes")
    p.add_argument("--stack", type=int, default=STACK_DEFAULT, help="bytes")
    p.add_argument("--heap-size", default="", help='e.g. "1K"; "" or "-" = rest of DTCM')
    p.add_argument("--heap-location", default="DTCM")
    a = p.parse_args()

    heap_size = "" if a.heap_size == "-" else a.heap_size
    dtcm_origin = DTCM_ORIGIN_DEFAULT
    if a.itcm != ITCM_DEFAULT or a.dtcm != DTCM_DEFAULT:
        dtcm_origin = DTCM_ORIGIN_HIGHMEM

    if a.heap_location == "DTCM" and (not heap_size or heap_size == "MAX"):
        heap_size_spec = ". = ORIGIN(DTCM) + LENGTH(DTCM) - STACK_SIZE;"
        stack_start_spec = ""
    else:
        heap_size_spec = f". += {heap_size if heap_size else '1K'};"
        stack_start_spec = ". = ORIGIN(DTCM) + LENGTH(DTCM) - STACK_SIZE;"

    subs = {
        "@@ITCM_LENGTH@@": str(a.itcm),
        "@@DTCM_LENGTH@@": str(a.dtcm),
        "@@DTCM_ORIGIN@@": dtcm_origin,
        "@@STACK_SIZE@@": str(a.stack),
        "@@HEAP_LOCATION@@": a.heap_location,
        "@@HEAP_SIZE_SPEC@@": heap_size_spec,
        "@@STACK_START_SPEC@@": stack_start_spec,
    }
    text = open(a.template).read()
    for k, v in subs.items():
        text = text.replace(k, v)
    with open(a.out, "w") as f:
        f.write(text)


if __name__ == "__main__":
    main()
