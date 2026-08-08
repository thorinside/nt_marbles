#!/usr/bin/env python3
"""Static contracts for the host-managed Marbles signal-routing boundary."""
from pathlib import Path

source = Path("src/nt_marbles_plugin.cc").read_text()
docs = Path("docs/control-surface.md").read_text()
makefile = Path("Makefile").read_text()

for name, default in (
    ("T Clock", "1"), ("X Clock", "1"), ("Reset", "2"),
    ("T1 Output", "kPhysicalOutputBusOffset + 3"),
    ("X1 Output", "kPhysicalOutputBusOffset + 4"),
    ("T2 Output", "kPhysicalOutputBusOffset + 5"),
    ("X2 Output", "kPhysicalOutputBusOffset + 6"),
    ("T3 Output", "kPhysicalOutputBusOffset + 7"),
    ("X3 Output", "kPhysicalOutputBusOffset + 8"), ("Y Output", "0"),
):
    macro = "NT_PARAMETER_CV_INPUT" if "Clock" in name or name == "Reset" else "NT_PARAMETER_CV_OUTPUT"
    assert f'{macro}("{name}", 0, {default})' in source, name

assert "routeValue <= 0 || routeValue > kNT_lastBus" in source
assert "tClock != nullptr" in source
assert "xClock ? marbles::CLOCK_SOURCE_EXTERNAL" in source
for route, output in (("kT1Route", "t1"), ("kX1Route", "algorithm->voltages"),
                      ("kT2Route", "t2"), ("kX2Route", "algorithm->voltages + frames"),
                      ("kT3Route", "t3"), ("kX3Route", "algorithm->voltages + 2 * frames"),
                      ("kYRoute", "algorithm->voltages + 3 * frames")):
    assert f"writeOutput(self, {route}, busFrames, frames, {output})" in source
for section in ("Signal routing and output coverage", "Original output coverage"):
    assert section in docs
for unit in ("marbles/random/output_channel.cc", "marbles/ramp/ramp_extractor.cc", "marbles/resources.cc"):
    assert unit in makefile
print("routing contract checks passed")
