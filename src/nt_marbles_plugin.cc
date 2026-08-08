// nt_marbles NT host boundary.
//
// This file contains the disting NT control surface only. Original Marbles DSP
// remains in third_party/eurorack; the forthcoming adapter belongs in
// src/marbles_dsp/ and consumes these host-managed parameter values.

#include <new>
#include "distingnt/api.h"

namespace {

constexpr uint32_t kNtMarblesGuid = NT_MULTICHAR('T', 'h', 'M', 'B');

enum Parameter {
  kTRate, kTBias, kTJitter, kTRange, kTGateLength, kTGateRandomness,
  kTModel, kTDejaVu, kXSpread, kXBias, kXSteps, kYSpread, kYBias, kYSteps,
  kXRelationship, kXDejaVu, kDejaVuAmount, kDejaVuLength, kXRange,
  kXRegister, kXScale, kYRange, kYDivider, kParameterCount,
};

const char* const kDejaVuStates[] = {"Off", "Amount", "Super-lock"};
const char* const kTModels[] = {"Complementary", "Clusters", "Drums",
                                "Independent", "Divider", "Three states", "Markov"};
const char* const kXRelationships[] = {"Identical", "Bump", "Tilt"};
const char* const kRanges[] = {"Narrow", "Positive", "Full"};
const char* const kOffOn[] = {"Off", "On"};

const _NT_parameter kParameters[] = {
    {"T Rate", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"T Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"T Jitter", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"T Range", 0, 2, 1, kNT_unitEnum, 0, kRanges},
    {"T Gate Length", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"T Gate Randomness", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"T Model", 0, 6, 0, kNT_unitEnum, 0, kTModels},
    {"T Deja Vu", 0, 2, 0, kNT_unitEnum, 0, kDejaVuStates},
    {"X Spread", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"X Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"X Steps", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Y Spread", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Y Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Y Steps", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"X Relationship", 0, 2, 0, kNT_unitEnum, 0, kXRelationships},
    {"X Deja Vu", 0, 2, 0, kNT_unitEnum, 0, kDejaVuStates},
    {"Deja Vu Amount", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"Deja Vu Length", 1, 16, 1, kNT_unitNone, 0, nullptr},
    {"X Range", 0, 2, 1, kNT_unitEnum, 0, kRanges},
    {"X Register", 0, 1, 0, kNT_unitEnum, 0, kOffOn},
    {"X Scale", 0, 5, 0, kNT_unitNone, 0, nullptr},
    {"Y Range", 0, 2, 1, kNT_unitEnum, 0, kRanges},
    {"Y Divider", 0, 11, 5, kNT_unitNone, 0, nullptr},
};

const uint8_t kTPage[] = {kTRate, kTBias, kTJitter, kTRange, kTGateLength,
                          kTGateRandomness, kTModel, kTDejaVu};
const uint8_t kXPage[] = {kXSpread, kXBias, kXSteps, kYSpread, kYBias, kYSteps,
                          kXRelationship, kXDejaVu, kXRange, kXRegister,
                          kXScale, kYRange, kYDivider};
const uint8_t kDejaVuPage[] = {kDejaVuAmount, kDejaVuLength};
const _NT_parameterPage kPages[] = {
    {"T rhythm", sizeof(kTPage), 1, {0, 0}, kTPage},
    {"X pitch", sizeof(kXPage), 2, {0, 0}, kXPage},
    {"Deja Vu", sizeof(kDejaVuPage), 3, {0, 0}, kDejaVuPage},
};
const _NT_parameterPages kParameterPages = {sizeof(kPages) / sizeof(kPages[0]), kPages};

struct NtMarblesAlgorithm : _NT_algorithm {
  bool xWorkspace;
  bool overview;
  bool help;
};

int clampPercent(float value) {
  int result = static_cast<int>(value * 100.0f + 0.5f);
  return result < 0 ? 0 : (result > 100 ? 100 : result);
}

void setFromUi(_NT_algorithm* self, Parameter parameter, int value) {
  NT_setParameterFromUi(NT_algorithmIndex(self), parameter + NT_parameterOffset(), value);
}

void calculateRequirements(_NT_algorithmRequirements& requirements,
                           const int32_t* /*specifications*/) {
  requirements.numParameters = kParameterCount;
  requirements.sram = sizeof(NtMarblesAlgorithm);
  requirements.dram = 0;
  requirements.dtc = 0;
  requirements.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& pointers,
                         const _NT_algorithmRequirements& /*requirements*/,
                         const int32_t* /*specifications*/) {
  auto* algorithm = new (pointers.sram) NtMarblesAlgorithm();
  algorithm->parameters = kParameters;
  algorithm->parameterPages = &kParameterPages;
  algorithm->xWorkspace = false;
  algorithm->overview = false;
  algorithm->help = false;
  return algorithm;
}

uint32_t hasCustomUi(_NT_algorithm* /*self*/) {
  // Button 3 is help. Buttons 1, 2, and 4 deliberately remain host navigation.
  return kNT_button3 | kNT_potL | kNT_potC | kNT_potR | kNT_potButtonL |
         kNT_potButtonC | kNT_potButtonR | kNT_encoderL | kNT_encoderR |
         kNT_encoderButtonL | kNT_encoderButtonR;
}

void customUi(_NT_algorithm* self, const _NT_uiData& data) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  algorithm->help = data.controls & kNT_button3;
  if ((data.controls & kNT_encoderL) && data.encoders[0] != 0) {
    algorithm->xWorkspace = data.encoders[0] > 0;
    algorithm->overview = false;
  }
  if ((data.controls & kNT_encoderButtonL) && !(data.lastButtons & kNT_encoderButtonL))
    algorithm->overview = !algorithm->overview;

  const bool held[] = {bool(data.controls & kNT_potButtonL),
                       bool(data.controls & kNT_potButtonC),
                       bool(data.controls & kNT_potButtonR)};
  const uint16_t changed[] = {kNT_potL, kNT_potC, kNT_potR};
  const Parameter tPrimary[] = {kTRate, kTBias, kTJitter};
  const Parameter tSecondary[] = {kTRange, kTGateLength, kTGateRandomness};
  const Parameter xPrimary[] = {kXSpread, kXBias, kXSteps};
  const Parameter xSecondary[] = {kYSpread, kYBias, kYSteps};
  for (int pot = 0; pot < 3; ++pot) {
    if (!(data.controls & changed[pot])) continue;
    Parameter parameter = algorithm->xWorkspace
        ? (held[pot] ? xSecondary[pot] : xPrimary[pot])
        : (held[pot] ? tSecondary[pot] : tPrimary[pot]);
    int value = clampPercent(data.pots[pot]);
    if (parameter == kTRange) value = value * 2 / 100;
    setFromUi(self, parameter, value);
  }

  if ((data.controls & kNT_encoderR) && data.encoders[1] != 0) {
    Parameter parameter = algorithm->xWorkspace ? kXRelationship : kTModel;
    const int maximum = algorithm->xWorkspace ? 2 : 6;
    int value = self->v[parameter] + data.encoders[1];
    if (value < 0) value = maximum;
    if (value > maximum) value = 0;
    setFromUi(self, parameter, value);
  }
  if ((data.controls & kNT_encoderButtonR) && !(data.lastButtons & kNT_encoderButtonR)) {
    Parameter parameter = algorithm->xWorkspace ? kXDejaVu : kTDejaVu;
    setFromUi(self, parameter, (self->v[parameter] + 1) % 3);
  }
}

void setupUi(_NT_algorithm* self, _NT_float3& pots) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  const Parameter primary[] = {algorithm->xWorkspace ? kXSpread : kTRate,
                               algorithm->xWorkspace ? kXBias : kTBias,
                               algorithm->xWorkspace ? kXSteps : kTJitter};
  for (int i = 0; i < 3; ++i) pots[i] = self->v[primary[i]] / 100.0f;
}

bool draw(_NT_algorithm* self) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  if (algorithm->help) {
    NT_drawText(4, 18, algorithm->xWorkspace ? "Pots: Spread / Bias / Steps" : "Pots: Rate / Bias / Jitter");
    NT_drawText(4, 42, algorithm->xWorkspace ? "Hold: Y Spread / Bias / Steps" : "Hold: Range / Gate Len / Gate Rand");
    return true;
  }
  NT_drawText(4, 10, algorithm->overview ? "MARBLES OVERVIEW" :
              (algorithm->xWorkspace ? "X PITCH" : "T RHYTHM"));
  if (algorithm->overview) {
    NT_drawText(4, 28, "L: T/X  R: model / Deja Vu");
    NT_drawText(4, 46, "B3: help  hold pot: secondary");
    return true;
  }
  if (algorithm->xWorkspace) {
    NT_drawText(4, 29, "Spread     Bias       Steps");
    NT_drawText(4, 47, "hold: Y Spread  Y Bias  Y Steps");
  } else {
    NT_drawText(4, 29, "Rate       Bias       Jitter");
    NT_drawText(4, 47, "hold: Range  Gate Len  Gate Rand");
  }
  return true;
}

const _NT_factory kFactory = {
    .guid = kNtMarblesGuid, .name = "Marbles", .description = "Mutable Instruments Marbles port",
    .numSpecifications = 0, .specifications = nullptr, .calculateStaticRequirements = nullptr,
    .initialise = nullptr, .calculateRequirements = calculateRequirements, .construct = construct,
    .parameterChanged = nullptr, .step = nullptr, .draw = draw, .midiRealtime = nullptr,
    .midiMessage = nullptr, .tags = kNT_tagUtility, .hasCustomUi = hasCustomUi,
    .customUi = customUi, .setupUi = setupUi, .serialise = nullptr, .deserialise = nullptr,
    .midiSysEx = nullptr, .parameterUiPrefix = nullptr, .parameterString = nullptr,
};

}  // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
  switch (selector) {
    case kNT_selector_version: return kNT_apiVersionCurrent;
    case kNT_selector_numFactories: return 1;
    case kNT_selector_factoryInfo: return reinterpret_cast<uintptr_t>(data == 0 ? &kFactory : nullptr);
    default: return 0;
  }
}
