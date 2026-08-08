#!/usr/bin/env sh
# Remove exactly the ordered overlay applied by apply-eurorack-patches.sh.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
upstream="$root/third_party/eurorack"
patches="$root/patches/eurorack"

git -C "$upstream" rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "Missing eurorack submodule; run git submodule update --init --recursive." >&2; exit 1; }
set -- "$patches"/*.patch
[ -e "$1" ] || { echo "No eurorack compatibility patches to remove."; exit 0; }
git -C "$upstream" apply --reverse --whitespace=error "$@"
[ -z "$(git -C "$upstream" status --porcelain)" ] || { echo "Overlay removal did not restore a clean checkout." >&2; exit 1; }
printf 'Removed %s compatibility patch(es).\n' "$#"
