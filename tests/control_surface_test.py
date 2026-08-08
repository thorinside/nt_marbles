#!/usr/bin/env python3
"""Static contract checks for the reviewed Marbles NT control surface."""
from pathlib import Path

source = Path("src/nt_marbles_plugin.cc").read_text()
docs = Path("docs/control-surface.md").read_text()

for name in ("T Rate", "T Bias", "T Jitter", "T Range", "T Gate Length",
             "T Gate Randomness", "X Spread", "X Bias", "X Steps", "Y Spread",
             "Y Bias", "Y Steps", "T Model", "X Relationship", "T Deja Vu",
             "X Deja Vu", "Deja Vu Amount", "Deja Vu Length"):
    assert f'"{name}"' in source, name

assert 'return kNT_button3 |' in source
assert 'kNT_button1' not in source and 'kNT_button2' not in source and 'kNT_button4' not in source
assert 'algorithm->help = data.controls & kNT_button3;' in source
assert 'held[pot] ? tSecondary[pot] : tPrimary[pot]' in source
assert 'held[pot] ? xSecondary[pot] : xPrimary[pot]' in source
assert '"Off", "Amount", "Super-lock"' in source
assert '{"T Rate", -60, 60, 0, kNT_unitHasStrings' in source
assert 'const char* const kTRanges[] = {"1/4x", "1x", "4x"};' in source
assert 'const char* const kDejaVuLengthLabels[]' in source
assert 'const char* const kYDividerLabels[]' in source
assert 'TRangeFromPot(data.pots[pot])' not in source
assert 'parameterValueFromPot(parameter, position)' in source
assert 'crossedPickup(' in source
assert '.parameterString = parameterString' in source
assert '{"T Model", 0, 5, 0, kNT_unitEnum' in source
assert '{"X Steps", 0, 100, 75, kNT_unitPercent' in source
assert 'const int maximum = algorithm->xWorkspace ? 2 : 5;' in source
assert 'if (data.encoders[0] != 0)' in source
assert 'if (data.encoders[1] != 0)' in source
assert 'data.controls & kNT_encoderL' not in source
assert 'data.controls & kNT_encoderR' not in source
assert 'algorithm->xyGenerator.LoadScale(i, nt_marbles::kFactoryScales[i]);' in source
assert 'NT_drawText(18, 13, kTModels[' in source
assert 'NT_drawText(18, 13, kXRelationships[' in source
assert 'NT_drawText(112, 13, kScaleNames[' in source
assert 'drawInteger(178, 36, self->v[kXSteps]);' in source
assert 'NT_drawShapeI(kNT_box, 0, 0, 255, 63, 8);' in source
assert 'NT_drawShapeI(kNT_line, 4, 18, 252, 18, 8);' in source
for upstream_value in ("ADC_CHANNEL_T_RATE", "ADC_CHANNEL_X_STEPS", "State::t_model",
                       "State::x_control_mode", "State::y_range"):
    assert upstream_value in docs, upstream_value
print("control-surface contract checks passed")
