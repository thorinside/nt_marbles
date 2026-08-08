Patches in this directory are ordered and numbered. The apply/remove scripts
verify that they round-trip against the exact clean pinned revision.

`0001-sample-rate-aware-ramp-extractor.patch` converts the pinned extractor's
32 kHz time constants into sample-rate-aware values and threads the active
sample rate through its initialization. The host adapter separately preserves
the original control-call cadence.

`scripts/prepare-eurorack-overlay.sh` copies the required upstream trees under
`build/eurorack-overlay/` and applies this ordered patch set there. Normal
builds compile that staging copy, so `third_party/eurorack` remains read-only
and clean.
