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

"""Expand the source templates listed in mk/gen_sources.json.

Equivalent of Bazel's ctx.actions.expand_template: each key of `substitutions`
is replaced literally in the template. Output paths are written under --outdir
keeping their repo-relative path.

    expand_templates.py --spec mk/gen_sources.json --outdir build/gen
"""
import argparse
import json
import pathlib


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--spec", required=True)
    p.add_argument("--outdir", required=True)
    p.add_argument("--root", default=".", help="repo root holding the templates")
    a = p.parse_args()

    root = pathlib.Path(a.root)
    out = pathlib.Path(a.outdir)
    n = 0
    for job in json.loads(pathlib.Path(a.spec).read_text()):
        text = (root / job["template"]).read_text()
        for key, value in job["substitutions"].items():
            text = text.replace(key, value)
        dst = out / job["output"]
        dst.parent.mkdir(parents=True, exist_ok=True)
        if not dst.exists() or dst.read_text() != text:  # keep mtimes stable
            dst.write_text(text)
        n += 1
    print(f"expanded {n} sources into {out}")


if __name__ == "__main__":
    main()
