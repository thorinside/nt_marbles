#!/usr/bin/env python3
"""Static contracts for the host-managed Marbles signal-routing boundary."""
from pathlib import Path

source = Path("src/nt_marbles_plugin.cc").read_text()
docs = Path("docs/control-surface.md").read_text()
makefile = Path("Makefile").read_text()

for macro, name, default in (
    ("NT_PARAMETER_CV_INPUT", "T Clock", "1"),
    ("NT_PARAMETER_CV_INPUT", "X Clock", "1"),
    ("NT_PARAMETER_CV_INPUT", "Reset", "2"),
    ("NT_PARAMETER_CV_INPUT", "X Register CV", "0"),
    ("NT_PARAMETER_CV_OUTPUT", "T1 Output", "kPhysicalOutputBusOffset + 3"),
    ("NT_PARAMETER_CV_OUTPUT", "X1 Output", "kPhysicalOutputBusOffset + 4"),
    ("NT_PARAMETER_CV_OUTPUT", "T2 Output", "kPhysicalOutputBusOffset + 5"),
    ("NT_PARAMETER_CV_OUTPUT", "X2 Output", "kPhysicalOutputBusOffset + 6"),
    ("NT_PARAMETER_CV_OUTPUT", "T3 Output", "kPhysicalOutputBusOffset + 7"),
    ("NT_PARAMETER_CV_OUTPUT", "X3 Output", "kPhysicalOutputBusOffset + 8"),
    ("NT_PARAMETER_CV_OUTPUT", "Y Output", "0"),
):
    assert f'{macro}("{name}", 0, {default})' in source, name

assert "routeValue <= 0 || routeValue > kNT_lastBus" in source
assert "tClock != nullptr" in source
assert "xClock ? marbles::CLOCK_SOURCE_EXTERNAL" in source
for statement in (
    "writeOutput(self, kT1Route, kT1Mode",
    "writeOutput(self, kX1Route, kX1Mode",
    "writeOutput(self, kT2Route, kT2Mode",
    "writeOutput(self, kX2Route, kX2Mode",
    "writeOutput(self, kT3Route, kT3Mode",
    "writeOutput(self, kX3Route, kX3Mode",
    "writeOutput(self, kYRoute, kYMode",
):
    assert statement in source, statement
assert "algorithm->gates, i, 0, 2)" in source
assert "algorithm->gates, i, 1, 2)" in source
assert "registerCv[offset + chunk - 1]" in source
assert "algorithm->controlFrames" in source
assert "if (self->v[mode]) destination[i] = sample" in source
assert "else destination[i] += sample" in source
for section in ("Signal routing and output coverage", "Original output coverage"):
    assert section in docs
for unit in ("marbles/random/output_channel.cc", "marbles/ramp/ramp_extractor.cc", "marbles/resources.cc"):
    assert unit in makefile
assert "engine-matrix" in makefile
print("routing contract checks passed")
