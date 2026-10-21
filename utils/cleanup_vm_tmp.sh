#!/bin/bash
# Remove scratch left behind by the provisioning/research scripts of this migration:
# mktemp directories in /tmp created since the migration started, and the
# research scratch trees. Does not touch anything else.
set -euo pipefail
shopt -s nullglob
for d in /tmp/tmp.??????????; do
  [ -d "$d" ] && [ "$(stat -c %U "$d")" = "$USER" ] && rm -rf "$d"
done
rm -rf "$HOME/tflm_scratch" "$HOME/ot_scratch" "$HOME/mpact_scratch"
df -h / | tail -1
