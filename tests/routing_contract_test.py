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
    "writeOutput(self, kT1Route, busFrames, frames, t1)",
    "writeOutput(self, kX1Route, busFrames, frames, algorithm->voltages, 4)",
    "writeOutput(self, kT2Route, busFrames, frames, t2)",
    "writeOutput(self, kX2Route, busFrames, frames, algorithm->voltages + 1, 4)",
    "writeOutput(self, kT3Route, busFrames, frames, t3)",
    "writeOutput(self, kX3Route, busFrames, frames, algorithm->voltages + 2, 4)",
    "writeOutput(self, kYRoute, busFrames, frames, algorithm->voltages + 3, 4)",
):
    assert statement in source, statement
assert "InterleavedSample(\n        algorithm->gates, i, 0, 2)" in source
assert "InterleavedSample(\n        algorithm->gates, i, 1, 2)" in source
assert "RegisterValueFromVolts(registerCv[frames - 1])" in source
for section in ("Signal routing and output coverage", "Original output coverage"):
    assert section in docs
for unit in ("marbles/random/output_channel.cc", "marbles/ramp/ramp_extractor.cc", "marbles/resources.cc"):
    assert unit in makefile
print("routing contract checks passed")
