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

"""Locate data files (firmware ELFs, test inputs) from tests and helpers.

    from coralnpu_test_utils import runfiles
    r = runfiles.Create()
    elf = r.Rlocation("coralnpu_hw/tests/cocotb/foo.elf")

A path `<workspace>/<repo path>` is resolved by looking, in order, in

  1. each directory of $CORALNPU_RUNFILES (os.pathsep separated; the Makefile
     points this at build/runfiles, where built artifacts such as firmware
     ELFs are staged), then
  2. the source tree ($CORALNPU_ROOT, default: this checkout).

The leading workspace component (conventionally `coralnpu_hw`) is dropped. A
path that exists nowhere still maps to a location under the source tree, so
callers can probe with os.path.exists() to skip binaries that were not built.
"""

import os

_ROOT = os.environ.get(
    "CORALNPU_ROOT",
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir)),
)


class Runfiles:

    def _roots(self):
        extra = os.environ.get("CORALNPU_RUNFILES", "")
        return [p for p in extra.split(os.pathsep) if p] + [_ROOT]

    def Rlocation(self, path, source_repo=None):
        if os.path.isabs(path):
            return path if os.path.exists(path) else None
        parts = path.split("/", 1)
        candidates = [path]
        if len(parts) == 2:
            candidates.append(parts[1])
        for root in self._roots():
            for rel in candidates:
                full = os.path.join(root, rel)
                if os.path.exists(full):
                    return full
        return os.path.join(_ROOT, candidates[-1])

    def EnvVars(self):
        return {}

    def CurrentRepository(self, frame=1):
        return "coralnpu_hw"


def Create(env=None):
    return Runfiles()
