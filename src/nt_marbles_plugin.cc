// nt_marbles NT host boundary.
//
// This file intentionally contains only disting NT integration.  Original
// Marbles DSP is compiled directly from third_party/eurorack; the adapter that
// wires host signals and parameters to that DSP belongs in src/marbles_dsp/.

#include <new>
#include "distingnt/api.h"

namespace {

// `Th` identifies the thorinside namespace, as used by the owner's NT ports.
constexpr uint32_t kNtMarblesGuid = NT_MULTICHAR('T', 'h', 'M', 'B');

struct NtMarblesAlgorithm : _NT_algorithm {};

void calculateRequirements(_NT_algorithmRequirements& requirements,
                           const int32_t* /*specifications*/) {
  requirements.numParameters = 0;
  requirements.sram = sizeof(NtMarblesAlgorithm);
  requirements.dram = 0;
  requirements.dtc = 0;
  requirements.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& pointers,
                         const _NT_algorithmRequirements& /*requirements*/,
                         const int32_t* /*specifications*/) {
  auto* algorithm = new (pointers.sram) NtMarblesAlgorithm();
  algorithm->parameters = nullptr;
  algorithm->parameterPages = nullptr;
  return algorithm;
}

const _NT_factory kFactory = {
    .guid = kNtMarblesGuid,
    .name = "Marbles",
    .description = "Mutable Instruments Marbles port",
    .numSpecifications = 0,
    .specifications = nullptr,
    .calculateStaticRequirements = nullptr,
    .initialise = nullptr,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = nullptr,
    .step = nullptr,
    .draw = nullptr,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = nullptr,
    .customUi = nullptr,
    .setupUi = nullptr,
    .serialise = nullptr,
    .deserialise = nullptr,
    .midiSysEx = nullptr,
    .parameterUiPrefix = nullptr,
    .parameterString = nullptr,
};

}  // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
  switch (selector) {
    case kNT_selector_version:
      return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
      return 1;
    case kNT_selector_factoryInfo:
      return reinterpret_cast<uintptr_t>(data == 0 ? &kFactory : nullptr);
    default:
      return 0;
  }
}
