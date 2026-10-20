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

"""Drop-in stand-in for bazel_tools' Python runfiles library.

The cocotb tests and helpers were written against Bazel runfiles and ask for
files as `r.Rlocation("coralnpu_hw/tests/cocotb/foo.elf")`. Without Bazel the
same path is resolved by looking in, in order:

  1. each directory in $CORALNPU_RUNFILES (os.pathsep separated; the Makefile
     points this at build/runfiles, where built artifacts such as firmware
     ELFs are staged), then
  2. the source tree ($CORALNPU_ROOT, defaulting to this checkout).

The leading workspace name (`coralnpu_hw`, or any other repo name) is dropped.
"""

import os

_ROOT = os.environ.get(
    "CORALNPU_ROOT",
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), *([os.pardir] * 6))),
)


class _Runfiles:

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
        return None

    def EnvVars(self):
        return {}

    def CurrentRepository(self, frame=1):
        return "coralnpu_hw"


def Create(env=None):
    return _Runfiles()
