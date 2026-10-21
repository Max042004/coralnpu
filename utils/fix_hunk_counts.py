#!/usr/bin/env python3
"""Recompute the line counts in unified-diff hunk headers.

Some of the old patches were edited by hand and carry wrong counts; Bazel's
patcher tolerated that, `git apply` (used by `bender vendor`) does not.
    fix_hunk_counts.py <patch> [<patch>...]    (rewrites in place)
"""
import re
import sys

HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(.*)$")


def fix(text):
    lines = text.split("\n")
    out = []
    i = 0
    while i < len(lines):
        m = HUNK.match(lines[i])
        if not m:
            out.append(lines[i])
            i += 1
            continue
        j = i + 1
        old = new = 0
        while j < len(lines):
            l = lines[j]
            if l.startswith("@@") or l.startswith("diff --git") or l.startswith("--- ") and j + 1 < len(lines) and lines[j + 1].startswith("+++ "):
                break
            if l.startswith("\\"):
                j += 1
                continue
            if l.startswith("-"):
                old += 1
            elif l.startswith("+"):
                new += 1
            elif l.startswith(" ") or l == "":
                # A blank line inside a hunk is an empty context line, except
                # for the trailing newline of the file (last element).
                if l == "" and j == len(lines) - 1:
                    break
                old += 1
                new += 1
            else:
                break
            j += 1
        out.append(f"@@ -{m.group(1)},{old} +{m.group(3)},{new} @@{m.group(5)}")
        out.extend(lines[i + 1:j])
        i = j
    return "\n".join(out)


for path in sys.argv[1:]:
    with open(path) as f:
        text = f.read()
    with open(path, "w") as f:
        f.write(fix(text))
