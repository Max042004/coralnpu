#!/bin/bash
# Re-express patches that `git apply` rejects (zero-context `diff -ur` hunks
# were fine for `patch`) as context-carrying git patches.
# Usage: regen_patches.sh <upstream-url> <rev> <patch_dir>
set -euo pipefail
URL="$1"; REV="$2"; PDIR="$(readlink -f "$3")"
W="$(mktemp -d)"
git clone -q "$URL" "$W/src"
cd "$W/src"
git checkout -q "$REV"
git config user.email regen@example.com
git config user.name regen
for p in "$PDIR"/*.patch; do
  if git apply -p1 "$p" 2>/dev/null; then
    echo "git-apply ok: $(basename "$p")"
    git add -A; git commit -q -m "$(basename "$p")"
  else
    echo "REGEN: $(basename "$p")"
    patch -p1 --no-backup-if-mismatch < "$p"
    git add -A
    git commit -q -m "$(basename "$p")"
    git diff HEAD~1 HEAD > "$p"
  fi
done
echo "done"
