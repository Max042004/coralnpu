#!/bin/bash
# Download + unpack a source tarball into <dir>/<name> (migration research helper).
#   utils/fetch_scratch.sh <dir> <name> <tarball-url>
set -euo pipefail
mkdir -p "$1/$2"
cd "$1"
if [ -z "$(ls -A "$2")" ]; then
  curl -fsSL "$3" | tar xz --strip-components=1 -C "$2"
fi
find "$2" \( -name BUILD -o -name BUILD.bazel -o -name '*.bzl' \) | head -60
du -sh "$2"
