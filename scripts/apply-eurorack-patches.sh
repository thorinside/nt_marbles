#!/usr/bin/env sh
# Apply the ordered, removable compatibility overlay without committing into
# the pinned upstream checkout.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
upstream="$root/third_party/eurorack"
patches="$root/patches/eurorack"

git -C "$upstream" rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "Missing eurorack submodule; run git submodule update --init --recursive." >&2; exit 1; }
[ -z "$(git -C "$upstream" status --porcelain)" ] || { echo "eurorack checkout is not clean; refusing to overlay patches." >&2; exit 1; }

set -- "$patches"/*.patch
[ -e "$1" ] || { echo "No eurorack compatibility patches to apply."; exit 0; }
git -C "$upstream" apply --unidiff-zero --whitespace=error "$@"
printf 'Applied %s compatibility patch(es).\n' "$#"
