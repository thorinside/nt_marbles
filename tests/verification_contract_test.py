#!/usr/bin/env python3
"""Auditable source-fidelity and coverage evidence for the integrated port."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / "src/nt_marbles_plugin.cc").read_text()
docs = (root / "docs/control-surface.md").read_text()
departures = (root / "docs/departures.md").read_text()
makefile = (root / "Makefile").read_text()
factory_scales = (root / "src/marbles_factory_scales.h").read_text()
workflow = (root / ".github/workflows/build.yml").read_text()
notices = (root / "THIRD_PARTY_NOTICES.md").read_text()
evidence = (root / "docs/acceptance-evidence.md").read_text()


def initializer_numbers(text: str, marker: str) -> list[str]:
    """Return numeric tokens from one balanced C/C++ initializer."""
    start = text.index("{", text.index(marker))
    depth = 0
    for end in range(start, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if depth == 0:
                body = re.sub(r"//.*", "", text[start:end + 1])
                return re.findall(r"(?<![A-Za-z_])\d+(?:\.\d+)?f?", body)
    raise AssertionError(f"unterminated initializer: {marker}")

# Provenance is both documented and pinned in the checked-out submodule.
revision = "08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4"
assert revision in (root / "README.md").read_text()
assert subprocess.check_output(
    ["git", "-C", str(root / "third_party/eurorack"), "rev-parse", "HEAD"], text=True
).strip() == revision

# The algorithmic core is compiled directly from upstream rather than copied.
for unit in (
    "marbles/random/t_generator.cc", "marbles/random/x_y_generator.cc",
    "marbles/random/output_channel.cc", "marbles/random/quantizer.cc",
    "marbles/random/discrete_distribution_quantizer.cc",
    "marbles/random/lag_processor.cc", "marbles/ramp/ramp_extractor.cc",
    "marbles/resources.cc", "stmlib/dsp/units.cc",
):
    assert f"$(UPSTREAM)/{unit}" in makefile, unit
assert "third_party/eurorack" in source
assert "#include \"marbles/random/t_generator.h\"" in source
assert "#include \"marbles/random/x_y_generator.h\"" in source

# The NT boundary loads the exact six immutable factory scales without linking
# the STM32 flash-storage implementation in settings.cc.
settings = (root / "third_party/eurorack/marbles/settings.cc").read_text()
assert initializer_numbers(settings, "const Scale preset_scales[6]") == \
       initializer_numbers(factory_scales, "kFactoryScales[kNumFactoryScales]")
assert "xyGenerator.LoadScale(i, nt_marbles::kFactoryScales[i])" in source

# Shipping metadata retains upstream/API attribution and identifies both the
# object and package. CI runs behavior checks before producing that artifact.
for holder in ("Copyright 2015 Emilie Gillet", "Copyright 2012 Emilie Gillet",
               "Copyright 2025 Expert Sleepers Ltd"):
    assert holder in notices, holder
assert "make test verify-overlay" in workflow
assert "make ARM_CXX=arm-none-eabi-g++ package" in workflow
assert "BUILD_COMMIT.txt" in makefile
assert "PLUGIN_SHA256.txt" in makefile
for criterion in range(1, 22):
    assert f"AC-{criterion:03d}" in evidence

# Persisted musical defaults match Settings::Init at the pinned revision.
for declaration in (
    '{"T Rate", -60, 60, 0, kNT_unitSemitones',
    '{"X Steps", 0, 100, 75, kNT_unitPercent',
    '{"X Range", 0, 2, 2, kNT_unitEnum',
    '{"Y Steps", 0, 100, 0, kNT_unitPercent',
    '{"Y Range", 0, 2, 2, kNT_unitEnum',
    '{"Y Divider", 0, 11, 6, kNT_unitNone',
):
    assert declaration in source, declaration

# Every upstream musical value/state has a documented NT representation or a
# documented non-musical hardware/UI exception.
for upstream_value in (
    "ADC_CHANNEL_DEJA_VU_AMOUNT", "ADC_CHANNEL_DEJA_VU_LENGTH",
    "ADC_CHANNEL_T_RATE", "ADC_CHANNEL_T_BIAS", "ADC_CHANNEL_T_JITTER",
    "ADC_CHANNEL_X_SPREAD", "ADC_CHANNEL_X_BIAS", "ADC_CHANNEL_X_STEPS",
    "State::t_deja_vu", "State::t_model", "State::t_range",
    "State::t_pulse_width_mean", "State::t_pulse_width_std",
    "State::x_deja_vu", "State::x_control_mode", "State::x_register_mode",
    "State::x_range", "State::x_scale", "State::y_spread", "State::y_bias",
    "State::y_steps", "State::y_divider", "State::y_range",
):
    assert upstream_value in docs, upstream_value
for hardware_exception in ("State::color_blind", "explicit_reset"):
    assert hardware_exception in docs, hardware_exception

# Every observed host-orchestration departure is retained with reason and an
# executable or inspectable verification method.
for departure in ("D-NT-01", "D-NT-02", "D-NT-03", "D-NT-04", "D-NT-05", "D-NT-06", "D-NT-07"):
    assert departure in departures, departure
for required_column in ("Original behavior", "NT adaptation and reason", "Verification method"):
    assert required_column in departures, required_column
assert "No compatibility patch is currently required" in departures
print("verification contract checks passed")
