// nt_marbles NT host boundary. Original Marbles generators remain in the
// pinned third_party/eurorack submodule; this file only adapts NT buses/values.

#include <new>
#include "distingnt/api.h"
#include "marbles/random/random_generator.h"
#include "marbles/random/random_stream.h"
#include "marbles/random/t_generator.h"
#include "marbles/random/x_y_generator.h"
#include "stmlib/utils/gate_flags.h"

namespace {

constexpr uint32_t kNtMarblesGuid = NT_MULTICHAR('T', 'h', 'M', 'B');
constexpr int kPhysicalOutputBusOffset = kNT_numInputBusses;

enum Parameter {
  kTRate, kTBias, kTJitter, kTRange, kTGateLength, kTGateRandomness,
  kTModel, kTDejaVu, kXSpread, kXBias, kXSteps, kYSpread, kYBias, kYSteps,
  kXRelationship, kXDejaVu, kDejaVuAmount, kDejaVuLength, kXRange,
  kXRegister, kXScale, kYRange, kYDivider,
  // Routing remains host-managed state.  A value of zero means no bus is read
  // or written; physical NT output N is bus kPhysicalOutputBusOffset + N.
  kTClockRoute, kXClockRoute, kResetRoute, kT1Route, kX1Route, kT2Route,
  kX2Route, kT3Route, kX3Route, kYRoute, kParameterCount,
};

const char* const kDejaVuStates[] = {"Off", "Amount", "Super-lock"};
const char* const kTModels[] = {"Complementary", "Clusters", "Drums",
                                "Independent", "Divider", "Three states", "Markov"};
const char* const kXRelationships[] = {"Identical", "Bump", "Tilt"};
const char* const kRanges[] = {"Narrow", "Positive", "Full"};
const char* const kOffOn[] = {"Off", "On"};
// Copied as data from pinned marbles/marbles.cc, where the physical setting
// selects this exact twelve-entry divider table.
const marbles::Ratio kYDividerRatios[] = {
    {1, 64}, {1, 48}, {1, 32}, {1, 24}, {1, 16}, {1, 12},
    {1, 8}, {1, 6}, {1, 4}, {1, 3}, {1, 2}, {1, 1},
};

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
    NT_PARAMETER_CV_INPUT("T Clock", 0, 1)
    NT_PARAMETER_CV_INPUT("X Clock", 0, 1)
    NT_PARAMETER_CV_INPUT("Reset", 0, 2)
    NT_PARAMETER_CV_OUTPUT("T1 Output", 0, kPhysicalOutputBusOffset + 3)
    NT_PARAMETER_CV_OUTPUT("X1 Output", 0, kPhysicalOutputBusOffset + 4)
    NT_PARAMETER_CV_OUTPUT("T2 Output", 0, kPhysicalOutputBusOffset + 5)
    NT_PARAMETER_CV_OUTPUT("X2 Output", 0, kPhysicalOutputBusOffset + 6)
    NT_PARAMETER_CV_OUTPUT("T3 Output", 0, kPhysicalOutputBusOffset + 7)
    NT_PARAMETER_CV_OUTPUT("X3 Output", 0, kPhysicalOutputBusOffset + 8)
    NT_PARAMETER_CV_OUTPUT("Y Output", 0, 0)
};

const uint8_t kTPage[] = {kTRate, kTBias, kTJitter, kTRange, kTGateLength,
                          kTGateRandomness, kTModel, kTDejaVu};
const uint8_t kXPage[] = {kXSpread, kXBias, kXSteps, kYSpread, kYBias, kYSteps,
                          kXRelationship, kXDejaVu, kXRange, kXRegister,
                          kXScale, kYRange, kYDivider};
const uint8_t kDejaVuPage[] = {kDejaVuAmount, kDejaVuLength};
const uint8_t kRoutingPage[] = {kTClockRoute, kXClockRoute, kResetRoute,
                                kT1Route, kX1Route, kT2Route, kX2Route,
                                kT3Route, kX3Route, kYRoute};
const _NT_parameterPage kPages[] = {
    {"T rhythm", sizeof(kTPage), 1, {0, 0}, kTPage},
    {"X pitch", sizeof(kXPage), 2, {0, 0}, kXPage},
    {"Deja Vu", sizeof(kDejaVuPage), 3, {0, 0}, kDejaVuPage},
    {"Routing", sizeof(kRoutingPage), 4, {0, 0}, kRoutingPage},
};
const _NT_parameterPages kParameterPages = {sizeof(kPages) / sizeof(kPages[0]), kPages};

struct NtMarblesAlgorithm : _NT_algorithm {
  bool xWorkspace;
  bool overview;
  bool help;
  stmlib::GateFlags previousTClock;
  stmlib::GateFlags previousXClock;
  stmlib::GateFlags previousReset;
  marbles::RandomGenerator randomGenerator;
  marbles::RandomStream randomStream;
  marbles::TGenerator tGenerator;
  marbles::XYGenerator xyGenerator;
  float* ramps;
  float* voltages;
  stmlib::GateFlags* clocks;
  bool* gates;
};

int clampPercent(float value) {
  int result = static_cast<int>(value * 100.0f + 0.5f);
  return result < 0 ? 0 : (result > 100 ? 100 : result);
}
float asUnit(const _NT_algorithm* self, Parameter parameter) {
  return self->v[parameter] / 100.0f;
}

void setFromUi(_NT_algorithm* self, Parameter parameter, int value) {
  NT_setParameterFromUi(NT_algorithmIndex(self), parameter + NT_parameterOffset(), value);
}

void calculateRequirements(_NT_algorithmRequirements& requirements,
                           const int32_t* /*specifications*/) {
  requirements.numParameters = kParameterCount;
  requirements.sram = sizeof(NtMarblesAlgorithm);
  requirements.dram = 0;
  // Four Marbles ramps and four X/Y voltages, then two clock flag and two gate
  // buffers. This is bounded by the host's published maximum step size.
  requirements.dtc = NT_globals.maxFramesPerStep * (8 * sizeof(float) +
                                                     2 * sizeof(stmlib::GateFlags) +
                                                     2 * sizeof(bool));
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
  algorithm->previousTClock = stmlib::GATE_FLAG_LOW;
  algorithm->previousXClock = stmlib::GATE_FLAG_LOW;
  algorithm->previousReset = stmlib::GATE_FLAG_LOW;
  algorithm->randomGenerator.Init(0x4d617262);
  algorithm->randomStream.Init(&algorithm->randomGenerator);
  algorithm->tGenerator.Init(&algorithm->randomStream, NT_globals.sampleRate);
  algorithm->xyGenerator.Init(&algorithm->randomStream, NT_globals.sampleRate);
  const uint32_t frames = NT_globals.maxFramesPerStep;
  algorithm->ramps = reinterpret_cast<float*>(pointers.dtc);
  algorithm->voltages = algorithm->ramps + 4 * frames;
  algorithm->clocks = reinterpret_cast<stmlib::GateFlags*>(algorithm->voltages + 4 * frames);
  algorithm->gates = reinterpret_cast<bool*>(algorithm->clocks + 2 * frames);
  return algorithm;
}

const float* inputBus(const _NT_algorithm* self, Parameter route, float* buses,
                      int frames) {
  const int routeValue = self->v[route];
  return routeValue > 0 && routeValue <= kNT_lastBus
      ? buses + (routeValue - 1) * frames : nullptr;
}
void writeOutput(const _NT_algorithm* self, Parameter route, float* buses,
                 int frames, const float* source) {
  const int routeValue = self->v[route];
  if (routeValue <= 0 || routeValue > kNT_lastBus) return;
  float* destination = buses + (routeValue - 1) * frames;
  for (int i = 0; i < frames; ++i) destination[i] = source[i];
}

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  const int frames = numFramesBy4 * 4;
  const float* tClock = inputBus(self, kTClockRoute, busFrames, frames);
  const float* xClock = inputBus(self, kXClockRoute, busFrames, frames);
  const float* reset = inputBus(self, kResetRoute, busFrames, frames);
  for (int i = 0; i < frames; ++i) {
    algorithm->clocks[i] = tClock
        ? stmlib::ExtractGateFlags(algorithm->previousTClock, tClock[i] > 1.0f)
        : static_cast<stmlib::GateFlags>(stmlib::GATE_FLAG_LOW);
    algorithm->previousTClock = algorithm->clocks[i];
    algorithm->clocks[frames + i] = xClock
        ? stmlib::ExtractGateFlags(algorithm->previousXClock, xClock[i] > 1.0f)
        : static_cast<stmlib::GateFlags>(stmlib::GATE_FLAG_LOW);
    algorithm->previousXClock = algorithm->clocks[frames + i];
  }
  bool didReset = false;
  if (reset) {
    for (int i = 0; i < frames; ++i) {
      const stmlib::GateFlags flags = stmlib::ExtractGateFlags(
          algorithm->previousReset, reset[i] > 1.0f);
      algorithm->previousReset = flags;
      didReset = didReset || (flags & stmlib::GATE_FLAG_RISING);
    }
  } else {
    algorithm->previousReset = stmlib::GATE_FLAG_LOW;
  }

  marbles::Ramps ramps = {algorithm->ramps, algorithm->ramps + frames,
                           {algorithm->ramps + 2 * frames, algorithm->ramps + 3 * frames}};
  algorithm->tGenerator.set_model(static_cast<marbles::TGeneratorModel>(self->v[kTModel]));
  algorithm->tGenerator.set_range(static_cast<marbles::TGeneratorRange>(self->v[kTRange]));
  algorithm->tGenerator.set_rate(asUnit(self, kTRate));
  algorithm->tGenerator.set_bias(asUnit(self, kTBias));
  algorithm->tGenerator.set_jitter(asUnit(self, kTJitter));
  algorithm->tGenerator.set_deja_vu(self->v[kTDejaVu] == 2 ? 0.5f :
                                     (self->v[kTDejaVu] ? asUnit(self, kDejaVuAmount) : 0.0f));
  algorithm->tGenerator.set_length(self->v[kDejaVuLength]);
  algorithm->tGenerator.set_pulse_width_mean(asUnit(self, kTGateLength));
  algorithm->tGenerator.set_pulse_width_std(asUnit(self, kTGateRandomness));
  algorithm->tGenerator.Process(tClock != nullptr, &didReset, algorithm->clocks,
                                ramps, algorithm->gates, frames);

  marbles::GroupSettings x = {};
  x.control_mode = static_cast<marbles::ControlMode>(self->v[kXRelationship]);
  x.voltage_range = static_cast<marbles::VoltageRange>(self->v[kXRange]);
  x.register_mode = self->v[kXRegister] != 0;
  x.register_value = 0.0f;
  x.spread = asUnit(self, kXSpread); x.bias = asUnit(self, kXBias); x.steps = asUnit(self, kXSteps);
  x.deja_vu = self->v[kXDejaVu] == 2 ? 0.5f :
              (self->v[kXDejaVu] ? asUnit(self, kDejaVuAmount) : 0.0f);
  x.scale_index = self->v[kXScale]; x.length = self->v[kDejaVuLength]; x.ratio = {1, 1};
  marbles::GroupSettings y = x;
  y.control_mode = marbles::CONTROL_MODE_IDENTICAL;
  y.voltage_range = static_cast<marbles::VoltageRange>(self->v[kYRange]);
  y.register_mode = false; y.register_value = 0.0f;
  y.spread = asUnit(self, kYSpread); y.bias = asUnit(self, kYBias); y.steps = asUnit(self, kYSteps);
  y.deja_vu = 0.0f; y.length = 1;
  y.ratio = kYDividerRatios[self->v[kYDivider]];
  const marbles::ClockSource xClockSource = xClock ? marbles::CLOCK_SOURCE_EXTERNAL
                                                    : marbles::CLOCK_SOURCE_INTERNAL_T1_T2_T3;
  algorithm->xyGenerator.Process(xClockSource, x, y, &didReset,
                                 algorithm->clocks + frames, ramps,
                                 algorithm->voltages, frames);

  float* t1 = algorithm->ramps; float* t2 = algorithm->ramps + frames;
  float* t3 = algorithm->ramps + 2 * frames;
  for (int i = 0; i < frames; ++i) {
    t1[i] = algorithm->gates[i] ? 5.0f : 0.0f;
    t2[i] = ramps.master[i] < 0.5f ? 5.0f : 0.0f;
    t3[i] = algorithm->gates[frames + i] ? 5.0f : 0.0f;
  }
  writeOutput(self, kT1Route, busFrames, frames, t1);
  writeOutput(self, kX1Route, busFrames, frames, algorithm->voltages);
  writeOutput(self, kT2Route, busFrames, frames, t2);
  writeOutput(self, kX2Route, busFrames, frames, algorithm->voltages + frames);
  writeOutput(self, kT3Route, busFrames, frames, t3);
  writeOutput(self, kX3Route, busFrames, frames, algorithm->voltages + 2 * frames);
  writeOutput(self, kYRoute, busFrames, frames, algorithm->voltages + 3 * frames);
}

uint32_t hasCustomUi(_NT_algorithm* /*self*/) {
  return kNT_button3 | kNT_potL | kNT_potC | kNT_potR | kNT_potButtonL |
         kNT_potButtonC | kNT_potButtonR | kNT_encoderL | kNT_encoderR |
         kNT_encoderButtonL | kNT_encoderButtonR;
}

void customUi(_NT_algorithm* self, const _NT_uiData& data) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  algorithm->help = data.controls & kNT_button3;
  if ((data.controls & kNT_encoderL) && data.encoders[0] != 0) { algorithm->xWorkspace = data.encoders[0] > 0; algorithm->overview = false; }
  if ((data.controls & kNT_encoderButtonL) && !(data.lastButtons & kNT_encoderButtonL)) algorithm->overview = !algorithm->overview;
  const bool held[] = {bool(data.controls & kNT_potButtonL), bool(data.controls & kNT_potButtonC), bool(data.controls & kNT_potButtonR)};
  const uint16_t changed[] = {kNT_potL, kNT_potC, kNT_potR};
  const Parameter tPrimary[] = {kTRate, kTBias, kTJitter}; const Parameter tSecondary[] = {kTRange, kTGateLength, kTGateRandomness};
  const Parameter xPrimary[] = {kXSpread, kXBias, kXSteps}; const Parameter xSecondary[] = {kYSpread, kYBias, kYSteps};
  for (int pot = 0; pot < 3; ++pot) if (data.controls & changed[pot]) { Parameter parameter = algorithm->xWorkspace ? (held[pot] ? xSecondary[pot] : xPrimary[pot]) : (held[pot] ? tSecondary[pot] : tPrimary[pot]); int value = clampPercent(data.pots[pot]); if (parameter == kTRange) value = value * 2 / 100; setFromUi(self, parameter, value); }
  if ((data.controls & kNT_encoderR) && data.encoders[1] != 0) { Parameter parameter = algorithm->xWorkspace ? kXRelationship : kTModel; const int maximum = algorithm->xWorkspace ? 2 : 6; int value = self->v[parameter] + data.encoders[1]; if (value < 0) value = maximum; if (value > maximum) value = 0; setFromUi(self, parameter, value); }
  if ((data.controls & kNT_encoderButtonR) && !(data.lastButtons & kNT_encoderButtonR)) { Parameter parameter = algorithm->xWorkspace ? kXDejaVu : kTDejaVu; setFromUi(self, parameter, (self->v[parameter] + 1) % 3); }
}

void setupUi(_NT_algorithm* self, _NT_float3& pots) { auto* algorithm = static_cast<NtMarblesAlgorithm*>(self); const Parameter primary[] = {algorithm->xWorkspace ? kXSpread : kTRate, algorithm->xWorkspace ? kXBias : kTBias, algorithm->xWorkspace ? kXSteps : kTJitter}; for (int i = 0; i < 3; ++i) pots[i] = self->v[primary[i]] / 100.0f; }
bool draw(_NT_algorithm* self) { auto* algorithm = static_cast<NtMarblesAlgorithm*>(self); if (algorithm->help) { NT_drawText(4, 18, algorithm->xWorkspace ? "Pots: Spread / Bias / Steps" : "Pots: Rate / Bias / Jitter"); NT_drawText(4, 42, algorithm->xWorkspace ? "Hold: Y Spread / Bias / Steps" : "Hold: Range / Gate Len / Gate Rand"); return true; } NT_drawText(4, 10, algorithm->overview ? "MARBLES OVERVIEW" : (algorithm->xWorkspace ? "X PITCH" : "T RHYTHM")); if (algorithm->overview) { NT_drawText(4, 28, "L: T/X  R: model / Deja Vu"); NT_drawText(4, 46, "B3: help  hold pot: secondary"); return true; } if (algorithm->xWorkspace) { NT_drawText(4, 29, "Spread     Bias       Steps"); NT_drawText(4, 47, "hold: Y Spread  Y Bias  Y Steps"); } else { NT_drawText(4, 29, "Rate       Bias       Jitter"); NT_drawText(4, 47, "hold: Range  Gate Len  Gate Rand"); } return true; }

const _NT_factory kFactory = {.guid = kNtMarblesGuid, .name = "Marbles", .description = "Mutable Instruments Marbles port", .numSpecifications = 0, .specifications = nullptr, .calculateStaticRequirements = nullptr, .initialise = nullptr, .calculateRequirements = calculateRequirements, .construct = construct, .parameterChanged = nullptr, .step = step, .draw = draw, .midiRealtime = nullptr, .midiMessage = nullptr, .tags = kNT_tagUtility, .hasCustomUi = hasCustomUi, .customUi = customUi, .setupUi = setupUi, .serialise = nullptr, .deserialise = nullptr, .midiSysEx = nullptr, .parameterUiPrefix = nullptr, .parameterString = nullptr};
}  // namespace
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) { switch (selector) { case kNT_selector_version: return kNT_apiVersionCurrent; case kNT_selector_numFactories: return 1; case kNT_selector_factoryInfo: return reinterpret_cast<uintptr_t>(data == 0 ? &kFactory : nullptr); default: return 0; } }
