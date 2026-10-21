#!/bin/bash
# Resolve a tag of a remote repository to its commit hash (for Bender vendor_package pins).
#   utils/resolve_tag.sh <repo-url> <tag>
set -euo pipefail
out=$(git ls-remote "$1" "refs/tags/$2^{}" "refs/tags/$2")
# Prefer the peeled (^{}) commit of annotated tags.
echo "$out" | awk '/\^\{\}/ {print $1; found=1} END {if (!found) exit 1}' \
  || echo "$out" | awk 'NR==1 {print $1}'
