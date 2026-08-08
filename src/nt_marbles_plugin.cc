// nt_marbles NT host boundary. Original Marbles generators remain in the
// pinned third_party/eurorack submodule; this file only adapts NT buses/values.

#include <new>
#include "distingnt/api.h"
#include "marbles/note_filter.h"
#include "marbles/random/random_generator.h"
#include "marbles/random/random_stream.h"
#include "marbles/random/t_generator.h"
#include "marbles/random/x_y_generator.h"
#include "marbles_factory_scales.h"
#include "nt_marbles_host_boundary.h"
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
  kX2Route, kT3Route, kX3Route, kYRoute,
  // Appended so existing parameter and route indices remain preset-compatible.
  kXRegisterCvRoute,
  kT1Mode, kX1Mode, kT2Mode, kX2Mode, kT3Mode, kX3Mode, kYMode,
  kParameterCount,
};

const char* const kDejaVuStates[] = {"Off", "Amount", "Super-lock"};
const char* const kTModels[] = {"Complementary", "Clusters", "Drums",
                                "Independent", "Divider", "Three states"};
const char* const kXRelationships[] = {"Identical", "Bump", "Tilt"};
const char* const kTRanges[] = {"1/4x", "1x", "4x"};
const char* const kXYRanges[] = {"Narrow", "Positive", "Full"};
const char* const kOffOn[] = {"Off", "On"};
const char* const kScaleNames[] = {"C major", "C minor", "Pentatonic",
                                   "Pelog", "Raag Bhairav", "Raag Shri"};
const char* const kDejaVuLengthLabels[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "10", "12", "14", "16"};
const int kDejaVuLengths[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16};
const char* const kYDividerLabels[] = {
    "1/64", "1/48", "1/32", "1/24", "1/16", "1/12",
    "1/8", "1/6", "1/4", "1/3", "1/2", "1/1"};
// Copied as data from pinned marbles/marbles.cc, where the physical setting
// selects this exact twelve-entry divider table.
const marbles::Ratio kYDividerRatios[] = {
    {1, 64}, {1, 48}, {1, 32}, {1, 24}, {1, 16}, {1, 12},
    {1, 8}, {1, 6}, {1, 4}, {1, 3}, {1, 2}, {1, 1},
};

const _NT_parameter kParameters[] = {
    // The DSP consumes an exponential -60..+60 coordinate, but that is not a
    // pitch and must not be presented to the user as semitones. Keep the
    // internal/preset-compatible domain and format it as the physical knob's
    // normalized percentage through parameterString().
    {"T Rate", -60, 60, 0, kNT_unitHasStrings, 0, nullptr},
    {"T Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"T Jitter", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"T Range", 0, 2, 1, kNT_unitEnum, 0, kTRanges},
    {"T Gate Length", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"T Gate Randomness", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"T Model", 0, 5, 0, kNT_unitEnum, 0, kTModels},
    {"T Deja Vu", 0, 2, 0, kNT_unitEnum, 0, kDejaVuStates},
    {"X Spread", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"X Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    // The original hardware has no stored power-on value for this physical
    // pot. Start above the unquantized midpoint so the selected factory scale
    // is audible on a new NT instance.
    {"X Steps", 0, 100, 75, kNT_unitPercent, 0, nullptr},
    {"Y Spread", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Y Bias", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Y Steps", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"X Relationship", 0, 2, 0, kNT_unitEnum, 0, kXRelationships},
    {"X Deja Vu", 0, 2, 0, kNT_unitEnum, 0, kDejaVuStates},
    {"Deja Vu Amount", 0, 100, 0, kNT_unitPercent, 0, nullptr},
    {"Deja Vu Length", 0, 11, 0, kNT_unitEnum, 0, kDejaVuLengthLabels},
    {"X Range", 0, 2, 2, kNT_unitEnum, 0, kXYRanges},
    {"X Register", 0, 1, 0, kNT_unitEnum, 0, kOffOn},
    {"X Scale", 0, 5, 0, kNT_unitEnum, 0, kScaleNames},
    {"Y Range", 0, 2, 2, kNT_unitEnum, 0, kXYRanges},
    {"Y Divider", 0, 11, 6, kNT_unitEnum, 0, kYDividerLabels},
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
    NT_PARAMETER_CV_INPUT("X Register CV", 0, 0)
    NT_PARAMETER_OUTPUT_MODE("T1 Output mode")
    NT_PARAMETER_OUTPUT_MODE("X1 Output mode")
    NT_PARAMETER_OUTPUT_MODE("T2 Output mode")
    NT_PARAMETER_OUTPUT_MODE("X2 Output mode")
    NT_PARAMETER_OUTPUT_MODE("T3 Output mode")
    NT_PARAMETER_OUTPUT_MODE("X3 Output mode")
    NT_PARAMETER_OUTPUT_MODE("Y Output mode")
};

const uint8_t kTPage[] = {kTRate, kTBias, kTJitter, kTRange, kTGateLength,
                          kTGateRandomness, kTModel, kTDejaVu};
const uint8_t kXPage[] = {kXSpread, kXBias, kXSteps, kYSpread, kYBias, kYSteps,
                          kXRelationship, kXDejaVu, kXRange, kXRegister,
                          kXScale, kYRange, kYDivider};
const uint8_t kDejaVuPage[] = {kDejaVuAmount, kDejaVuLength};
const uint8_t kRoutingPage[] = {kTClockRoute, kXClockRoute, kResetRoute,
                                kXRegisterCvRoute,
                                kT1Route, kT1Mode, kX1Route, kX1Mode,
                                kT2Route, kT2Mode, kX2Route, kX2Mode,
                                kT3Route, kT3Mode, kX3Route, kX3Mode,
                                kYRoute, kYMode};
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
  marbles::NoteFilter noteFilter;
  float registerValue;
  int maxFrames;
  int controlFrames;
  int potTarget[3];
  float potPosition[3];
  bool potPickupLocked[3];
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

int boundedIndex(int value, int maximum) {
  return value < 0 ? 0 : (value > maximum ? maximum : value);
}

float normalizedParameterValue(const _NT_algorithm* self,
                               Parameter parameter) {
  if (parameter == kTRate) {
    return nt_marbles::PotFromTRate(self->v[parameter]);
  }
  if (parameter == kTRange) {
    return (boundedIndex(self->v[parameter], 2) + 0.5f) / 3.0f;
  }
  return nt_marbles::Clamp(self->v[parameter] / 100.0f, 0.0f, 1.0f);
}

int parameterValueFromPot(Parameter parameter, float position) {
  if (parameter == kTRate) {
    const float coordinate = nt_marbles::TRateFromPot(position);
    return static_cast<int>(
        coordinate + (coordinate < 0.0f ? -0.5f : 0.5f));
  }
  if (parameter == kTRange) {
    return nt_marbles::TRangeFromPot(position);
  }
  return clampPercent(position);
}

bool crossedPickup(float previous, float current, float target) {
  return (previous <= target && current >= target) ||
         (previous >= target && current <= target);
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
  algorithm->randomGenerator.Init(1);
  algorithm->randomStream.Init(&algorithm->randomGenerator);
  algorithm->tGenerator.Init(&algorithm->randomStream, NT_globals.sampleRate);
  algorithm->xyGenerator.Init(&algorithm->randomStream, NT_globals.sampleRate);
  for (int i = 0; i < nt_marbles::kNumFactoryScales; ++i) {
    algorithm->xyGenerator.LoadScale(i, nt_marbles::kFactoryScales[i]);
  }
  algorithm->noteFilter.Init();
  // The physical input exists continuously and idles at the normalized
  // midpoint. Warm the filter to that safe disconnected value.
  for (int i = 0; i < 16; ++i) algorithm->noteFilter.Process(0.5f);
  algorithm->registerValue = 0.5f;
  const Parameter initialPotTargets[] = {kTRate, kTBias, kTJitter};
  const float initialPotPositions[] = {0.5f, 0.5f, 0.0f};
  for (int i = 0; i < 3; ++i) {
    algorithm->potTarget[i] = initialPotTargets[i];
    algorithm->potPosition[i] = initialPotPositions[i];
    algorithm->potPickupLocked[i] = false;
  }
  const uint32_t frames = NT_globals.maxFramesPerStep;
  algorithm->maxFrames = static_cast<int>(frames);
  algorithm->controlFrames = nt_marbles::ControlBlockFrames(
      static_cast<float>(NT_globals.sampleRate));
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
void writeOutput(const _NT_algorithm* self, Parameter route, Parameter mode,
                 float* buses,
                 int frames, const float* source, int sourceStride = 1,
                 int count = -1) {
  const int routeValue = self->v[route];
  if (routeValue <= 0 || routeValue > kNT_lastBus) return;
  float* destination = buses + (routeValue - 1) * frames;
  const int samples = count < 0 ? frames : count;
  for (int i = 0; i < samples; ++i) {
    const float sample = nt_marbles::InterleavedSample(
        source, i, 0, sourceStride);
    if (self->v[mode]) destination[i] = sample;
    else destination[i] += sample;
  }
}

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  const int frames = numFramesBy4 * 4;
  const float* tClock = inputBus(self, kTClockRoute, busFrames, frames);
  const float* xClock = inputBus(self, kXClockRoute, busFrames, frames);
  const float* reset = inputBus(self, kResetRoute, busFrames, frames);
  const float* registerCv = inputBus(
      self, kXRegisterCvRoute, busFrames, frames);
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
  algorithm->tGenerator.set_model(static_cast<marbles::TGeneratorModel>(self->v[kTModel]));
  algorithm->tGenerator.set_range(static_cast<marbles::TGeneratorRange>(self->v[kTRange]));
  algorithm->tGenerator.set_rate(self->v[kTRate]);
  algorithm->tGenerator.set_bias(asUnit(self, kTBias));
  algorithm->tGenerator.set_jitter(asUnit(self, kTJitter));
  algorithm->tGenerator.set_deja_vu(self->v[kTDejaVu] == 2 ? 0.5f :
                                     (self->v[kTDejaVu] ? asUnit(self, kDejaVuAmount) : 0.0f));
  algorithm->tGenerator.set_length(
      kDejaVuLengths[boundedIndex(self->v[kDejaVuLength], 11)]);
  algorithm->tGenerator.set_pulse_width_mean(asUnit(self, kTGateLength));
  algorithm->tGenerator.set_pulse_width_std(asUnit(self, kTGateRandomness));
  marbles::GroupSettings x = {};
  x.control_mode = static_cast<marbles::ControlMode>(self->v[kXRelationship]);
  x.voltage_range = static_cast<marbles::VoltageRange>(self->v[kXRange]);
  x.register_mode = self->v[kXRegister] != 0;
  x.spread = asUnit(self, kXSpread); x.bias = asUnit(self, kXBias); x.steps = asUnit(self, kXSteps);
  x.deja_vu = self->v[kXDejaVu] == 2 ? 0.5f :
              (self->v[kXDejaVu] ? asUnit(self, kDejaVuAmount) : 0.0f);
  x.scale_index = self->v[kXScale];
  x.length = kDejaVuLengths[boundedIndex(self->v[kDejaVuLength], 11)];
  x.ratio = {1, 1};
  marbles::GroupSettings y = x;
  y.control_mode = marbles::CONTROL_MODE_IDENTICAL;
  y.voltage_range = static_cast<marbles::VoltageRange>(self->v[kYRange]);
  y.register_mode = false; y.register_value = 0.0f;
  y.spread = asUnit(self, kYSpread); y.bias = asUnit(self, kYBias); y.steps = asUnit(self, kYSteps);
  y.deja_vu = 0.0f; y.length = 1;
  y.ratio = kYDividerRatios[self->v[kYDivider]];
  const marbles::ClockSource xClockSource = xClock ? marbles::CLOCK_SOURCE_EXTERNAL
                                                    : marbles::CLOCK_SOURCE_INTERNAL_T1_T2_T3;
  const int scratchStride = algorithm->maxFrames;
  marbles::Ramps ramps = {
      algorithm->ramps, algorithm->ramps + scratchStride,
      {algorithm->ramps + 2 * scratchStride,
       algorithm->ramps + 3 * scratchStride}};
  for (int offset = 0; offset < frames; offset += algorithm->controlFrames) {
    const int remaining = frames - offset;
    const int chunk = remaining < algorithm->controlFrames
        ? remaining : algorithm->controlFrames;
    bool didReset = false;
    if (reset) {
      for (int i = 0; i < chunk; ++i) {
        const stmlib::GateFlags flags = stmlib::ExtractGateFlags(
            algorithm->previousReset, reset[offset + i] > 1.0f);
        algorithm->previousReset = flags;
        didReset = didReset || (flags & stmlib::GATE_FLAG_RISING);
      }
    } else {
      algorithm->previousReset = stmlib::GATE_FLAG_LOW;
    }

    const float registerInput = registerCv
        ? nt_marbles::RegisterValueFromVolts(
              registerCv[offset + chunk - 1])
        : 0.5f;
    algorithm->registerValue = algorithm->noteFilter.Process(registerInput);
    x.register_value = registerCv ? algorithm->registerValue : 0.5f;

    algorithm->tGenerator.Process(
        tClock != nullptr, &didReset, algorithm->clocks + offset,
        ramps, algorithm->gates, chunk);
    algorithm->xyGenerator.Process(
        xClockSource, x, y, &didReset,
        algorithm->clocks + frames + offset, ramps,
        algorithm->voltages, chunk);

    float* t1 = algorithm->ramps;
    float* t2 = algorithm->ramps + scratchStride;
    float* t3 = algorithm->ramps + 2 * scratchStride;
    for (int i = 0; i < chunk; ++i) {
      // Read every generator result before reusing the ramp buffers as NT
      // output scratch. In particular, t1 aliases ramps.master.
      const bool t1High = nt_marbles::InterleavedSample(
          algorithm->gates, i, 0, 2);
      const bool t2High = ramps.master[i] < 0.5f;
      const bool t3High = nt_marbles::InterleavedSample(
          algorithm->gates, i, 1, 2);
      t1[i] = t1High ? 5.0f : 0.0f;
      t2[i] = t2High ? 5.0f : 0.0f;
      t3[i] = t3High ? 5.0f : 0.0f;
    }
    writeOutput(self, kT1Route, kT1Mode,
                busFrames + offset, frames, t1, 1, chunk);
    writeOutput(self, kX1Route, kX1Mode, busFrames + offset, frames,
                algorithm->voltages, 4, chunk);
    writeOutput(self, kT2Route, kT2Mode,
                busFrames + offset, frames, t2, 1, chunk);
    writeOutput(self, kX2Route, kX2Mode, busFrames + offset, frames,
                algorithm->voltages + 1, 4, chunk);
    writeOutput(self, kT3Route, kT3Mode,
                busFrames + offset, frames, t3, 1, chunk);
    writeOutput(self, kX3Route, kX3Mode, busFrames + offset, frames,
                algorithm->voltages + 2, 4, chunk);
    writeOutput(self, kYRoute, kYMode, busFrames + offset, frames,
                algorithm->voltages + 3, 4, chunk);
  }
}

uint32_t hasCustomUi(_NT_algorithm* /*self*/) {
  return kNT_button3 | kNT_potL | kNT_potC | kNT_potR | kNT_potButtonL |
         kNT_potButtonC | kNT_potButtonR | kNT_encoderL | kNT_encoderR |
         kNT_encoderButtonL | kNT_encoderButtonR;
}

void customUi(_NT_algorithm* self, const _NT_uiData& data) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  algorithm->help = data.controls & kNT_button3;
  // Encoder deltas are reported in encoders[]. Unlike pots, their bits are
  // used to claim the controls but are not present in data.controls.
  if (data.encoders[0] != 0) {
    algorithm->xWorkspace = data.encoders[0] > 0;
    algorithm->overview = false;
  }
  if ((data.controls & kNT_encoderButtonL) &&
      !(data.lastButtons & kNT_encoderButtonL)) {
    algorithm->overview = !algorithm->overview;
  }
  const bool held[] = {bool(data.controls & kNT_potButtonL),
                       bool(data.controls & kNT_potButtonC),
                       bool(data.controls & kNT_potButtonR)};
  const uint16_t changed[] = {kNT_potL, kNT_potC, kNT_potR};
  const Parameter tPrimary[] = {kTRate, kTBias, kTJitter};
  const Parameter tSecondary[] = {kTRange, kTGateLength,
                                  kTGateRandomness};
  const Parameter xPrimary[] = {kXSpread, kXBias, kXSteps};
  const Parameter xSecondary[] = {kYSpread, kYBias, kYSteps};
  for (int pot = 0; pot < 3; ++pot) {
    const Parameter parameter = algorithm->xWorkspace
        ? (held[pot] ? xSecondary[pot] : xPrimary[pot])
        : (held[pot] ? tSecondary[pot] : tPrimary[pot]);
    const float position = nt_marbles::Clamp(data.pots[pot], 0.0f, 1.0f);
    if (algorithm->potTarget[pot] != parameter) {
      algorithm->potTarget[pot] = parameter;
      algorithm->potPosition[pot] = position;
      const float target = normalizedParameterValue(self, parameter);
      algorithm->potPickupLocked[pot] =
          (position < target - 0.005f || position > target + 0.005f);
    }
    if (!(data.controls & changed[pot])) continue;
    if (algorithm->potPickupLocked[pot]) {
      const float target = normalizedParameterValue(self, parameter);
      const bool pickedUp = crossedPickup(
          algorithm->potPosition[pot], position, target);
      algorithm->potPosition[pot] = position;
      if (!pickedUp) continue;
      algorithm->potPickupLocked[pot] = false;
    } else {
      algorithm->potPosition[pot] = position;
    }
    setFromUi(self, parameter, parameterValueFromPot(parameter, position));
  }
  if (data.encoders[1] != 0) {
    const Parameter parameter = algorithm->xWorkspace
        ? kXRelationship : kTModel;
    const int maximum = algorithm->xWorkspace ? 2 : 5;
    int value = self->v[parameter] + data.encoders[1];
    if (value < 0) value = maximum;
    if (value > maximum) value = 0;
    setFromUi(self, parameter, value);
  }
  if ((data.controls & kNT_encoderButtonR) &&
      !(data.lastButtons & kNT_encoderButtonR)) {
    const Parameter parameter = algorithm->xWorkspace
        ? kXDejaVu : kTDejaVu;
    setFromUi(self, parameter, (self->v[parameter] + 1) % 3);
  }
}

void setupUi(_NT_algorithm* self, _NT_float3& pots) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  const Parameter primary[] = {
      algorithm->xWorkspace ? kXSpread : kTRate,
      algorithm->xWorkspace ? kXBias : kTBias,
      algorithm->xWorkspace ? kXSteps : kTJitter};
  for (int i = 0; i < 3; ++i) {
    pots[i] = normalizedParameterValue(self, primary[i]);
    algorithm->potTarget[i] = primary[i];
    algorithm->potPosition[i] = pots[i];
    algorithm->potPickupLocked[i] = false;
  }
}

int formatInteger(char* text, int value) {
  char reversed[4];
  int count = 0;
  const bool negative = value < 0;
  unsigned int magnitude = static_cast<unsigned int>(
      negative ? -value : value);
  do {
    reversed[count++] = static_cast<char>('0' + magnitude % 10);
    magnitude /= 10;
  } while (magnitude && count < 4);
  int position = 0;
  if (negative) text[position++] = '-';
  while (count) text[position++] = reversed[--count];
  text[position] = '\0';
  return position;
}

int formatTRatePercent(char* text, int value) {
  const int bounded = value < -60 ? -60 : (value > 60 ? 60 : value);
  // Round the internal -60..+60 coordinate onto the physical 0..100% knob.
  const int percent = ((bounded + 60) * 100 + 60) / 120;
  int position = formatInteger(text, percent);
  text[position++] = '%';
  text[position] = '\0';
  return position;
}

int parameterString(_NT_algorithm* /*self*/, int parameter, int value,
                    char* text) {
  return parameter == kTRate ? formatTRatePercent(text, value) : 0;
}

void drawInteger(int x, int y, int value) {
  char text[6];
  formatInteger(text, value);
  NT_drawText(x, y, text);
}

void drawFrame(bool divider) {
  // Mirror the approved 256 x 64 prototype. The dim frame and divider give
  // the cyan-only display a stable visual hierarchy without competing with
  // the full-bright labels.
  NT_drawShapeI(kNT_box, 0, 0, 255, 63, 8);
  if (divider) NT_drawShapeI(kNT_line, 4, 18, 252, 18, 8);
}

bool draw(_NT_algorithm* self) {
  auto* algorithm = static_cast<NtMarblesAlgorithm*>(self);
  if (algorithm->help) {
    drawFrame(false);
    NT_drawText(5, 24, algorithm->xWorkspace
        ? "Pots: Spread / Bias / Steps" : "Pots: Rate / Bias / Jitter");
    NT_drawText(5, 47, algorithm->xWorkspace
        ? "Hold: Y Spread / Bias / Steps" : "Hold: Range / Gate Len / Gate Rand");
    return true;
  }
  if (algorithm->overview) {
    drawFrame(true);
    NT_drawText(5, 13, "MARBLES OVERVIEW");
    NT_drawText(5, 36, "L: T/X  R: model / Deja Vu");
    NT_drawText(5, 55, "B3: help  hold pot: secondary");
    return true;
  }

  drawFrame(true);
  if (algorithm->xWorkspace) {
    NT_drawText(5, 13, "X");
    NT_drawText(18, 13, kXRelationships[
        boundedIndex(self->v[kXRelationship], 2)]);
    NT_drawText(76, 13, "Scale:");
    NT_drawText(112, 13, kScaleNames[
        boundedIndex(self->v[kXScale], 5)]);
    NT_drawText(190, 13, kDejaVuStates[
        boundedIndex(self->v[kXDejaVu], 2)]);
    NT_drawText(5, 36, "Spread");
    drawInteger(48, 36, self->v[kXSpread]);
    NT_drawText(78, 36, "Bias");
    drawInteger(110, 36, self->v[kXBias]);
    NT_drawText(137, 36, "Steps");
    drawInteger(178, 36, self->v[kXSteps]);
    NT_drawText(5, 55, "hold Y");
    NT_drawText(46, 55, "Spr");
    drawInteger(68, 55, self->v[kYSpread]);
    NT_drawText(96, 55, "Bias");
    drawInteger(127, 55, self->v[kYBias]);
    NT_drawText(155, 55, "Steps");
    drawInteger(196, 55, self->v[kYSteps]);
  } else {
    NT_drawText(5, 13, "T");
    NT_drawText(18, 13, kTModels[boundedIndex(self->v[kTModel], 5)]);
    NT_drawText(190, 13, kDejaVuStates[
        boundedIndex(self->v[kTDejaVu], 2)]);
    NT_drawText(5, 36, "Rate");
    char rate[6];
    formatTRatePercent(rate, self->v[kTRate]);
    NT_drawText(40, 36, rate);
    NT_drawText(70, 36, "Bias");
    drawInteger(104, 36, self->v[kTBias]);
    NT_drawText(135, 36, "Jitter");
    drawInteger(181, 36, self->v[kTJitter]);
    NT_drawText(5, 55, "hold");
    NT_drawText(32, 55, "Range");
    NT_drawText(72, 55, kTRanges[boundedIndex(self->v[kTRange], 2)]);
    NT_drawText(108, 55, "Len");
    drawInteger(132, 55, self->v[kTGateLength]);
    NT_drawText(161, 55, "Rand");
    drawInteger(194, 55, self->v[kTGateRandomness]);
  }
  return true;
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
    .step = step,
    .draw = draw,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = hasCustomUi,
    .customUi = customUi,
    .setupUi = setupUi,
    .serialise = nullptr,
    .deserialise = nullptr,
    .midiSysEx = nullptr,
    .parameterUiPrefix = nullptr,
    .parameterString = parameterString};
}  // namespace
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) { switch (selector) { case kNT_selector_version: return kNT_apiVersionCurrent; case kNT_selector_numFactories: return 1; case kNT_selector_factoryInfo: return reinterpret_cast<uintptr_t>(data == 0 ? &kFactory : nullptr); default: return 0; } }
