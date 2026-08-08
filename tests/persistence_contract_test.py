#!/usr/bin/env python3
"""Static contracts for host-managed nt_marbles preset state."""
from pathlib import Path

source = Path("src/nt_marbles_plugin.cc").read_text()
docs = Path("docs/control-surface.md").read_text()
api = Path("third_party/distingNT_API/include/distingnt/api.h").read_text()

# Every value in the plugin's parameter enum is backed by one normal NT
# declaration. The NT owns self->v, so these values (including I/O routes) are
# the sole preset surface rather than a plug-in-maintained shadow copy.
parameter_block = source.split("const _NT_parameter kParameters[] = {", 1)[1].split("};", 1)[0]
assert parameter_block.count('{"') == 23
assert parameter_block.count("NT_PARAMETER_CV_INPUT") == 4
assert parameter_block.count("NT_PARAMETER_CV_OUTPUT") == 7
assert parameter_block.count("NT_PARAMETER_OUTPUT_MODE") == 7
assert "kT1Mode, kX1Mode, kT2Mode, kX2Mode, kT3Mode, kX3Mode, kYMode" in source
assert "requirements.numParameters = kParameterCount" in source
assert "const int16_t*\t\t\t\tv;\t\t\t\t\t// Pointer to the algorithm's parameter values. Managed by the system." in api

# Custom JSON is reserved for demonstrated non-parameter continuity needs.
# It must not duplicate values that the NT host owns in self->v.
assert ".serialise = nullptr" in source
assert ".deserialise = nullptr" in source
assert "NT_setParameterFromUi" in source
assert "self->v[route]" in source
assert "self->v[route] =" not in source
assert "self->v[mode]" in source

# Saved routes are used as-is when valid, otherwise safely disconnected; no
# fallback route can silently replace a saved selection.
assert "routeValue > 0 && routeValue <= kNT_lastBus" in source
assert "routeValue <= 0 || routeValue > kNT_lastBus" in source
assert "Host-native preset restoration" in docs
assert "does not substitute a different route" in docs
assert "fresh, valid Marbles random" in docs
print("persistence contract checks passed")
