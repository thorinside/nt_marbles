Place ordered, numbered compatibility patches here (for example,
`001-nt-compatibility.patch`). They are applied to the clean pinned
`third_party/eurorack` checkout by `scripts/apply-eurorack-patches.sh` and
reversed by `scripts/remove-eurorack-patches.sh`.

This directory is intentionally empty in the foundation slice: the selected
upstream generator units compile without compatibility edits.
