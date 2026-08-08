#!/usr/bin/env sh
# Build a patched staging copy while leaving the pinned upstream checkout clean.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
upstream="$root/third_party/eurorack"
patches="$root/patches/eurorack"
overlay="$root/build/eurorack-overlay"

git -C "$upstream" rev-parse --is-inside-work-tree >/dev/null 2>&1 || {
  echo "Missing eurorack submodule; run git submodule update --init --recursive." >&2
  exit 1
}
[ -z "$(git -C "$upstream" status --porcelain)" ] || {
  echo "eurorack checkout is not clean; refusing to prepare overlay." >&2
  exit 1
}

mkdir -p "$root/build"
staging=$(mktemp -d "$root/build/.eurorack-overlay.XXXXXX")
trap 'rm -rf "$staging"' EXIT HUP INT TERM
cp -R "$upstream/marbles" "$staging/"
cp -R "$upstream/stmlib" "$staging/"

set -- "$patches"/*.patch
if [ -e "$1" ]; then
  for patch_file do
    patch -s -p1 -d "$staging" < "$patch_file"
  done
fi

rm -rf "$overlay"
mv "$staging" "$overlay"
trap - EXIT HUP INT TERM
printf 'Prepared clean compatibility overlay at %s.\n' "$overlay"
