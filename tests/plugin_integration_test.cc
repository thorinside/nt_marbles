#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "distingnt/api.h"
#include "marbles_factory_scales.h"

#ifndef NT_TEST_SAMPLE_RATE
#define NT_TEST_SAMPLE_RATE 48000
#endif

#ifndef NT_TEST_FRAMES
#define NT_TEST_FRAMES 64
#endif

namespace {

const int kTestFrames = NT_TEST_FRAMES;
static_assert(kTestFrames % 4 == 0, "NT step frames must be divisible by four");

_NT_algorithm* gAlgorithm = nullptr;
std::vector<int16_t>* gValues = nullptr;

struct DrawnText {
  int x;
  int y;
  std::string text;
};

std::vector<DrawnText> gDrawnText;

struct DrawnShape {
  _NT_shape shape;
  int x0;
  int y0;
  int x1;
  int y1;
  int colour;
};

std::vector<DrawnShape> gDrawnShapes;

int ParameterIndex(const _NT_algorithm* algorithm, uint32_t count,
                   const char* name) {
  for (uint32_t i = 0; i < count; ++i) {
    if (std::strcmp(algorithm->parameters[i].name, name) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool Near(float actual, float expected) {
  return std::fabs(actual - expected) < 0.00001f;
}

bool TextDrawn(const char* expected) {
  for (const DrawnText& drawn : gDrawnText) {
    if (drawn.text == expected) return true;
  }
  return false;
}

bool TextDrawnAt(int x, int y, const char* expected) {
  for (const DrawnText& drawn : gDrawnText) {
    if (drawn.x == x && drawn.y == y && drawn.text == expected) return true;
  }
  return false;
}

bool ShapeDrawn(_NT_shape shape, int x0, int y0, int x1, int y1,
                int colour) {
  for (const DrawnShape& drawn : gDrawnShapes) {
    if (drawn.shape == shape && drawn.x0 == x0 && drawn.y0 == y0 &&
        drawn.x1 == x1 && drawn.y1 == y1 && drawn.colour == colour) {
      return true;
    }
  }
  return false;
}

void ClearDrawCalls() {
  gDrawnText.clear();
  gDrawnShapes.clear();
}

struct Fixture {
  const _NT_factory* factory;
  _NT_algorithmRequirements requirements;
  std::vector<uint8_t> sram;
  std::vector<uint8_t> dram;
  std::vector<uint8_t> dtc;
  std::vector<uint8_t> itc;
  std::vector<int16_t> values;
  _NT_algorithm* algorithm;

  Fixture()
      : factory(reinterpret_cast<const _NT_factory*>(
            pluginEntry(kNT_selector_factoryInfo, 0))),
        requirements(),
        algorithm(nullptr) {
    assert(pluginEntry(kNT_selector_version, 0) == kNT_apiVersionCurrent);
    assert(pluginEntry(kNT_selector_numFactories, 0) == 1);
    assert(factory != nullptr);
    assert(std::strcmp(factory->name, "Marbles") == 0);
    assert(factory->guid == NT_MULTICHAR('T', 'h', 'M', 'B'));
    factory->calculateRequirements(requirements, nullptr);
    assert(requirements.numParameters > 0);
    assert(requirements.sram >= sizeof(_NT_algorithm));
    assert(requirements.dtc > 0);

    sram.resize(requirements.sram);
    dram.resize(std::max<uint32_t>(requirements.dram, 1));
    dtc.resize(std::max<uint32_t>(requirements.dtc, 1));
    itc.resize(std::max<uint32_t>(requirements.itc, 1));
    _NT_algorithmMemoryPtrs pointers = {
        sram.data(), dram.data(), dtc.data(), itc.data()};
    algorithm = factory->construct(pointers, requirements, nullptr);
    assert(algorithm != nullptr);
    assert(algorithm->parameters != nullptr);
    assert(algorithm->parameterPages != nullptr);

    values.resize(requirements.numParameters);
    for (uint32_t i = 0; i < requirements.numParameters; ++i) {
      values[i] = algorithm->parameters[i].def;
    }
    algorithm->vIncludingCommon = values.data();
    algorithm->v = values.data();
    gAlgorithm = algorithm;
    gValues = &values;
  }

  ~Fixture() {
    gAlgorithm = nullptr;
    gValues = nullptr;
  }

  int parameter(const char* name) const {
    const int result = ParameterIndex(
        algorithm, requirements.numParameters, name);
    assert(result >= 0);
    return result;
  }
};

struct SignalActivity {
  bool low = false;
  bool high = false;

  void Observe(const float* samples, int frames) {
    for (int i = 0; i < frames; ++i) {
      low = low || Near(samples[i], 0.0f);
      high = high || Near(samples[i], 5.0f);
      assert(Near(samples[i], 0.0f) || Near(samples[i], 5.0f));
    }
  }
};

struct RhythmRun {
  std::vector<float> t1;
  std::vector<float> t2;
  std::vector<float> t3;
};

struct RhythmSettings {
  int model = 0;
  int range = 1;
  int rate = 0;
  int bias = 50;
  int jitter = 0;
  int gateLength = 50;
  int gateRandomness = 0;
  int dejaVu = 0;
  int dejaVuAmount = 0;
  int dejaVuLength = 0;
  bool externalClock = false;
  bool resetAtHalf = false;
};

RhythmRun RunRhythm(const RhythmSettings& settings) {
  Fixture fixture;
  const int frames = kTestFrames;
  const int blocks = (NT_TEST_SAMPLE_RATE + frames - 1) / frames;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  fixture.values[fixture.parameter("T Model")] = settings.model;
  fixture.values[fixture.parameter("T Range")] = settings.range;
  fixture.values[fixture.parameter("T Rate")] = settings.rate;
  fixture.values[fixture.parameter("T Bias")] = settings.bias;
  fixture.values[fixture.parameter("T Jitter")] = settings.jitter;
  fixture.values[fixture.parameter("T Gate Length")] = settings.gateLength;
  fixture.values[fixture.parameter("T Gate Randomness")] =
      settings.gateRandomness;
  fixture.values[fixture.parameter("T Deja Vu")] = settings.dejaVu;
  fixture.values[fixture.parameter("Deja Vu Amount")] = settings.dejaVuAmount;
  fixture.values[fixture.parameter("Deja Vu Length")] = settings.dejaVuLength;
  fixture.values[fixture.parameter("T Clock")] = settings.externalClock ? 1 : 0;

  RhythmRun result;
  for (int block = 0; block < blocks; ++block) {
    std::fill(buses.begin(), buses.end(), 0.0f);
    if (settings.externalClock) {
      float* clock = buses.data();
      const int halfPeriod = NT_TEST_SAMPLE_RATE / 200;
      for (int i = 0; i < frames; ++i) {
        const int sample = block * frames + i;
        clock[i] = (sample / halfPeriod) % 2 ? 5.0f : 0.0f;
      }
    }
    if (settings.resetAtHalf && block == blocks / 2) {
      buses[frames] = 5.0f;
    }
    fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
    const int routes[] = {
        fixture.values[fixture.parameter("T1 Output")],
        fixture.values[fixture.parameter("T2 Output")],
        fixture.values[fixture.parameter("T3 Output")]};
    std::vector<float>* outputs[] = {&result.t1, &result.t2, &result.t3};
    for (int channel = 0; channel < 3; ++channel) {
      const float* source = buses.data() + (routes[channel] - 1) * frames;
      outputs[channel]->insert(outputs[channel]->end(), source, source + frames);
    }
  }
  return result;
}

float HighFraction(const std::vector<float>& values) {
  int high = 0;
  for (float value : values) high += value > 2.5f ? 1 : 0;
  return static_cast<float>(high) / static_cast<float>(values.size());
}

void TestFactoryAndTRate() {
  Fixture fixture;
  const int rate = fixture.parameter("T Rate");
  const int model = fixture.parameter("T Model");
  assert(fixture.algorithm->parameters[rate].min == -60);
  assert(fixture.algorithm->parameters[rate].max == 60);
  assert(fixture.algorithm->parameters[rate].def == 0);
  assert(fixture.algorithm->parameters[rate].unit == kNT_unitHasStrings);
  assert(fixture.factory->parameterString != nullptr);
  char display[kNT_parameterStringSize] = {};
  assert(fixture.factory->parameterString(
      fixture.algorithm, rate, -60, display) == 2);
  assert(std::strcmp(display, "0%") == 0);
  assert(fixture.factory->parameterString(
      fixture.algorithm, rate, 0, display) == 3);
  assert(std::strcmp(display, "50%") == 0);
  assert(fixture.factory->parameterString(
      fixture.algorithm, rate, 60, display) == 4);
  assert(std::strcmp(display, "100%") == 0);
  assert(fixture.algorithm->parameters[model].max == 5);

  const _NT_parameter& tRange =
      fixture.algorithm->parameters[fixture.parameter("T Range")];
  assert(std::strcmp(tRange.enumStrings[0], "1/4x") == 0);
  assert(std::strcmp(tRange.enumStrings[1], "1x") == 0);
  assert(std::strcmp(tRange.enumStrings[2], "4x") == 0);

  _NT_uiData ui = {};
  ui.controls = kNT_potL;
  ui.pots[0] = 0.0f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[rate] == -60);
  ui.pots[0] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[rate] == 0);
  ui.pots[0] = 1.0f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[rate] == 60);

  _NT_float3 pots = {};
  fixture.factory->setupUi(fixture.algorithm, pots);
  assert(Near(pots[0], 1.0f));
}

void TestClockedTRhythm() {
  Fixture fixture;
  const int frames = kTestFrames;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  const int tClock = fixture.parameter("T Clock");
  const int reset = fixture.parameter("Reset");
  const int t1Route = fixture.parameter("T1 Output");
  const int t2Route = fixture.parameter("T2 Output");
  const int t3Route = fixture.parameter("T3 Output");
  assert(fixture.values[tClock] == 1);
  assert(fixture.values[reset] == 2);
  assert(fixture.values[t1Route] == 15);
  assert(fixture.values[t2Route] == 17);
  assert(fixture.values[t3Route] == 19);

  SignalActivity t1;
  SignalActivity t2;
  SignalActivity t3;
  for (int block = 0; block < 192; ++block) {
    std::fill(buses.begin(), buses.end(), 0.0f);
    float* clock = buses.data();
    float* resetBus = buses.data() + frames;
    for (int i = 0; i < frames; ++i) {
      clock[i] = ((block * frames + i) / 8) % 2 ? 5.0f : 0.0f;
    }
    if (block == 96) resetBus[0] = 5.0f;
    fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
    t1.Observe(buses.data() + (fixture.values[t1Route] - 1) * frames,
               frames);
    t2.Observe(buses.data() + (fixture.values[t2Route] - 1) * frames,
               frames);
    t3.Observe(buses.data() + (fixture.values[t3Route] - 1) * frames,
               frames);
    for (int i = 0; i < frames * 2; ++i) {
      assert(Near(buses[(12 * frames) + i], 0.0f));
    }
  }
  assert(t1.low && t1.high);
  assert(t2.low && t2.high);
  assert(t3.low && t3.high);

  const int oldRoute = fixture.values[t1Route];
  fixture.values[t1Route] = 0;
  std::fill(buses.begin(), buses.end(), 0.0f);
  std::fill(buses.begin() + (oldRoute - 1) * frames,
            buses.begin() + oldRoute * frames, 7.0f);
  fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
  for (int i = 0; i < frames; ++i) {
    assert(Near(buses[(oldRoute - 1) * frames + i], 7.0f));
  }
}

struct PitchRun {
  std::vector<float> x1;
  std::vector<float> x2;
  std::vector<float> x3;
  std::vector<float> y;
};

struct PitchSettings {
  int scale = 0;
  int xClockRoute = 1;
  bool registerMode = false;
  float registerVolts = 0.0f;
  int relationship = 0;
  int xRange = 2;
  int spread = 80;
  int bias = 37;
  int steps = 75;
  int xDejaVu = 0;
  int dejaVuAmount = 0;
  int dejaVuLength = 0;
  int yRange = 2;
  int ySpread = 50;
  int yBias = 50;
  int ySteps = 0;
  int yDivider = 6;
};

PitchRun RunPitch(const PitchSettings& settings) {
  Fixture fixture;
  const int frames = kTestFrames;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  fixture.values[fixture.parameter("X Scale")] = settings.scale;
  fixture.values[fixture.parameter("X Clock")] = settings.xClockRoute;
  fixture.values[fixture.parameter("X Spread")] = settings.spread;
  fixture.values[fixture.parameter("X Bias")] = settings.bias;
  // Maximum quantization strength retains only the highest-weight root notes,
  // which are shared by every factory scale. A mid setting exercises each
  // scale's distinctive note set.
  fixture.values[fixture.parameter("X Steps")] = settings.steps;
  fixture.values[fixture.parameter("X Relationship")] = settings.relationship;
  fixture.values[fixture.parameter("X Range")] = settings.xRange;
  fixture.values[fixture.parameter("X Deja Vu")] = settings.xDejaVu;
  fixture.values[fixture.parameter("Deja Vu Amount")] = settings.dejaVuAmount;
  fixture.values[fixture.parameter("Deja Vu Length")] = settings.dejaVuLength;
  fixture.values[fixture.parameter("Y Range")] = settings.yRange;
  fixture.values[fixture.parameter("Y Spread")] = settings.ySpread;
  fixture.values[fixture.parameter("Y Bias")] = settings.yBias;
  fixture.values[fixture.parameter("Y Steps")] = settings.ySteps;
  fixture.values[fixture.parameter("Y Divider")] = settings.yDivider;
  fixture.values[fixture.parameter("X Register")] = settings.registerMode ? 1 : 0;
  fixture.values[fixture.parameter("X Register CV")] = settings.registerMode ? 3 : 0;
  fixture.values[fixture.parameter("Y Output")] = 21;

  PitchRun result;
  for (int block = 0; block < 256; ++block) {
    std::fill(buses.begin(), buses.end(), 0.0f);
    float* clock = buses.data();
    float* registerBus = buses.data() + 2 * frames;
    for (int i = 0; i < frames; ++i) {
      clock[i] = ((block * frames + i) / 8) % 2 ? 5.0f : 0.0f;
      registerBus[i] = settings.registerVolts;
    }
    fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
    if (block < 128) continue;
    const int routes[] = {
        fixture.values[fixture.parameter("X1 Output")],
        fixture.values[fixture.parameter("X2 Output")],
        fixture.values[fixture.parameter("X3 Output")],
        fixture.values[fixture.parameter("Y Output")]};
    std::vector<float>* outputs[] = {
        &result.x1, &result.x2, &result.x3, &result.y};
    for (int channel = 0; channel < 4; ++channel) {
      const float* source = buses.data() + (routes[channel] - 1) * frames;
      outputs[channel]->insert(
          outputs[channel]->end(), source, source + frames);
    }
  }
  return result;
}

PitchRun RunPitch(int scale, int xClockRoute, bool registerMode,
                  float registerVolts) {
  PitchSettings settings;
  settings.scale = scale;
  settings.xClockRoute = xClockRoute;
  settings.registerMode = registerMode;
  settings.registerVolts = registerVolts;
  return RunPitch(settings);
}

bool HasRange(const std::vector<float>& values) {
  const auto limits = std::minmax_element(values.begin(), values.end());
  return !values.empty() && *limits.second - *limits.first > 0.01f;
}

bool Different(const std::vector<float>& a, const std::vector<float>& b) {
  assert(a.size() == b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::fabs(a[i] - b[i]) > 0.0001f) return true;
  }
  return false;
}

bool OnScale(const std::vector<float>& values, const marbles::Scale& scale) {
  for (float value : values) {
    const float pitchClass = value - std::floor(value);
    bool matched = false;
    for (int degree = 0; degree < scale.num_degrees; ++degree) {
      const float distance = std::fabs(
          pitchClass - scale.degree[degree].voltage);
      if (distance < 0.0015f || std::fabs(distance - 1.0f) < 0.0015f) {
        matched = true;
        break;
      }
    }
    if (!matched) return false;
  }
  return true;
}

float Mean(const std::vector<float>& values) {
  assert(!values.empty());
  float total = 0.0f;
  for (float value : values) total += value;
  return total / static_cast<float>(values.size());
}

int Transitions(const std::vector<float>& values) {
  int transitions = 0;
  for (size_t i = 1; i < values.size(); ++i) {
    if (std::fabs(values[i] - values[i - 1]) > 0.0001f) ++transitions;
  }
  return transitions;
}

void TestRhythmParameterEffects() {
  RhythmSettings settings;
  settings.rate = 36;
  settings.range = 0;
  const RhythmRun quarter = RunRhythm(settings);
  settings.range = 1;
  const RhythmRun normal = RunRhythm(settings);
  settings.range = 2;
  const RhythmRun quadruple = RunRhythm(settings);
  assert(Transitions(quarter.t2) < Transitions(normal.t2));
  assert(Transitions(normal.t2) < Transitions(quadruple.t2));

  settings = RhythmSettings();
  settings.rate = -12;
  const RhythmRun slow = RunRhythm(settings);
  settings.rate = 12;
  const RhythmRun fast = RunRhythm(settings);
  assert(Transitions(slow.t2) < Transitions(fast.t2));

  settings = RhythmSettings();
  settings.externalClock = true;
  settings.rate = -60;
  const RhythmRun dividedClock = RunRhythm(settings);
  settings.rate = 0;
  const RhythmRun unityClock = RunRhythm(settings);
  settings.rate = 60;
  const RhythmRun multipliedClock = RunRhythm(settings);
  assert(Transitions(dividedClock.t2) < Transitions(unityClock.t2));
  assert(Transitions(unityClock.t2) < Transitions(multipliedClock.t2));

  settings = RhythmSettings();
  settings.externalClock = true;
  settings.range = 0;
  const RhythmRun quarterExternal = RunRhythm(settings);
  settings.range = 2;
  const RhythmRun quadrupleExternal = RunRhythm(settings);
  assert(Transitions(quarterExternal.t2) <
         Transitions(quadrupleExternal.t2));

  settings = RhythmSettings();
  settings.rate = 24;
  settings.gateLength = 20;
  const RhythmRun shortGates = RunRhythm(settings);
  settings.gateLength = 80;
  const RhythmRun longGates = RunRhythm(settings);
  assert(HighFraction(shortGates.t1) < HighFraction(longGates.t1));

  settings = RhythmSettings();
  settings.rate = 24;
  const RhythmRun plain = RunRhythm(settings);
  settings = RhythmSettings();
  settings.rate = 24;
  settings.bias = 10;
  const RhythmRun lowBias = RunRhythm(settings);
  settings.bias = 90;
  const RhythmRun highBias = RunRhythm(settings);
  assert(Different(lowBias.t1, highBias.t1));
  settings = RhythmSettings();
  settings.rate = 24;
  settings.jitter = 100;
  const RhythmRun jitter = RunRhythm(settings);
  assert(Different(plain.t2, jitter.t2));
  settings = RhythmSettings();
  settings.rate = 24;
  settings.gateRandomness = 100;
  const RhythmRun randomLength = RunRhythm(settings);
  assert(Different(plain.t1, randomLength.t1));

  std::vector<RhythmRun> models;
  for (int model = 0; model < 6; ++model) {
    settings = RhythmSettings();
    settings.rate = 24;
    settings.model = model;
    models.push_back(RunRhythm(settings));
  }
  int distinctModels = 1;
  for (int model = 1; model < 6; ++model) {
    if (Different(models[0].t1, models[model].t1) ||
        Different(models[0].t3, models[model].t3)) {
      ++distinctModels;
    }
  }
  assert(distinctModels >= 5);

  settings = RhythmSettings();
  settings.rate = 24;
  settings.dejaVu = 1;
  settings.dejaVuAmount = 100;
  settings.dejaVuLength = 0;
  const RhythmRun loop1 = RunRhythm(settings);
  settings.dejaVuLength = 7;
  const RhythmRun loop8 = RunRhythm(settings);
  assert(Different(loop1.t1, loop8.t1));
  settings.dejaVu = 2;
  const RhythmRun superLock = RunRhythm(settings);
  assert(Different(loop8.t1, superLock.t1) ||
         Different(loop8.t3, superLock.t3));

  settings = RhythmSettings();
  settings.rate = 24;
  const RhythmRun freeRunning = RunRhythm(settings);
  settings.resetAtHalf = true;
  const RhythmRun reset = RunRhythm(settings);
  assert(Different(freeRunning.t1, reset.t1) ||
         Different(freeRunning.t3, reset.t3));
}

void TestPitchDefaultsAndRoutes() {
  Fixture fixture;
  assert(fixture.values[fixture.parameter("X Range")] == 2);
  assert(fixture.values[fixture.parameter("Y Range")] == 2);
  assert(fixture.values[fixture.parameter("Y Steps")] == 0);
  assert(fixture.values[fixture.parameter("Y Divider")] == 6);
  assert(fixture.values[fixture.parameter("X Scale")] == 0);
  assert(fixture.values[fixture.parameter("X Steps")] == 75);
  assert(fixture.values[fixture.parameter("X Register CV")] == 0);
  assert(fixture.values[fixture.parameter("Y Output")] == 0);

  const _NT_parameter& scale =
      fixture.algorithm->parameters[fixture.parameter("X Scale")];
  const char* expectedScales[] = {
      "C major", "C minor", "Pentatonic", "Pelog",
      "Raag Bhairav", "Raag Shri"};
  assert(scale.min == 0 && scale.max == 5);
  for (int i = 0; i < 6; ++i) {
    assert(std::strcmp(scale.enumStrings[i], expectedScales[i]) == 0);
  }

  const _NT_parameter& length =
      fixture.algorithm->parameters[fixture.parameter("Deja Vu Length")];
  const char* expectedLengths[] = {
      "1", "2", "3", "4", "5", "6", "7", "8", "10", "12", "14", "16"};
  assert(length.min == 0 && length.max == 11 && length.def == 0);
  assert(length.unit == kNT_unitEnum);
  for (int i = 0; i < 12; ++i) {
    assert(std::strcmp(length.enumStrings[i], expectedLengths[i]) == 0);
  }

  const _NT_parameter& divider =
      fixture.algorithm->parameters[fixture.parameter("Y Divider")];
  const char* expectedDividers[] = {
      "1/64", "1/48", "1/32", "1/24", "1/16", "1/12",
      "1/8", "1/6", "1/4", "1/3", "1/2", "1/1"};
  assert(divider.min == 0 && divider.max == 11 && divider.def == 6);
  assert(divider.unit == kNT_unitEnum);
  for (int i = 0; i < 12; ++i) {
    assert(std::strcmp(divider.enumStrings[i], expectedDividers[i]) == 0);
  }
  assert(nt_marbles::kNumFactoryScales == 6);
  assert(nt_marbles::kFactoryScales[0].num_degrees == 12);
  assert(nt_marbles::kFactoryScales[3].num_degrees == 7);
  assert(Near(nt_marbles::kFactoryScales[3].degree[1].voltage, 0.1275f));
  assert(nt_marbles::kFactoryScales[5].degree[10].weight == 128);

  for (uint32_t i = 0; i < fixture.requirements.numParameters; ++i) {
    const _NT_parameter& parameter = fixture.algorithm->parameters[i];
    if (parameter.unit == kNT_unitCvInput ||
        parameter.unit == kNT_unitCvOutput) {
      assert(parameter.min == 0);
      assert(parameter.max == kNT_lastBus);
    }
  }
}

void TestPitchProcessing() {
  const PitchRun cMajor = RunPitch(0, 1, false, 0.0f);
  const PitchRun pelog = RunPitch(3, 1, false, 0.0f);
  assert(HasRange(cMajor.x1));
  assert(HasRange(cMajor.x2));
  assert(HasRange(cMajor.x3));
  assert(HasRange(cMajor.y));
  assert(Different(cMajor.x1, cMajor.x2));
  assert(Different(cMajor.x1, cMajor.x3));
  assert(Different(cMajor.x1, cMajor.y));
  assert(Different(cMajor.x1, pelog.x1));
  assert(OnScale(cMajor.x1, nt_marbles::kFactoryScales[0]));
  assert(OnScale(pelog.x1, nt_marbles::kFactoryScales[3]));

  const PitchRun internalClock = RunPitch(0, 0, false, 0.0f);
  assert(HasRange(internalClock.x1));
  assert(HasRange(internalClock.y));

  const PitchRun lowRegister = RunPitch(0, 1, true, -5.0f);
  const PitchRun highRegister = RunPitch(0, 1, true, 5.0f);
  // Register mode intentionally holds the acquired CV rather than generating
  // a range. Verify that the routed input selects materially different held
  // pitches at the two ends of the documented -5 V..+5 V range.
  assert(Different(lowRegister.x1, highRegister.x1));
  assert(Mean(highRegister.x1) - Mean(lowRegister.x1) > 5.0f);
}

void TestPitchParameterEffects() {
  PitchSettings settings;
  const PitchRun identical = RunPitch(settings);
  settings.relationship = 1;
  const PitchRun bump = RunPitch(settings);
  settings.relationship = 2;
  const PitchRun tilt = RunPitch(settings);
  assert(Different(identical.x1, bump.x1));
  assert(Different(bump.x2, tilt.x2) ||
         Different(bump.x3, tilt.x3));

  settings = PitchSettings();
  settings.xRange = 0;
  const PitchRun narrow = RunPitch(settings);
  settings.xRange = 1;
  const PitchRun positive = RunPitch(settings);
  settings.xRange = 2;
  const PitchRun full = RunPitch(settings);
  const auto narrowLimits = std::minmax_element(
      narrow.x1.begin(), narrow.x1.end());
  const auto positiveLimits = std::minmax_element(
      positive.x1.begin(), positive.x1.end());
  const auto fullLimits = std::minmax_element(full.x1.begin(), full.x1.end());
  assert(*narrowLimits.first >= -0.001f && *narrowLimits.second <= 2.001f);
  assert(*positiveLimits.first >= -0.001f && *positiveLimits.second <= 5.001f);
  assert(*fullLimits.first < -0.1f || *fullLimits.second > 5.1f);

  settings = PitchSettings();
  settings.spread = 15;
  const PitchRun lowSpread = RunPitch(settings);
  settings.spread = 90;
  const PitchRun highSpread = RunPitch(settings);
  assert(Different(lowSpread.x1, highSpread.x1));
  settings = PitchSettings();
  settings.bias = 10;
  const PitchRun lowBias = RunPitch(settings);
  settings.bias = 90;
  const PitchRun highBias = RunPitch(settings);
  assert(Mean(highBias.x1) > Mean(lowBias.x1));
  settings = PitchSettings();
  settings.steps = 0;
  const PitchRun smooth = RunPitch(settings);
  settings.steps = 100;
  const PitchRun quantized = RunPitch(settings);
  assert(Different(smooth.x1, quantized.x1));

  settings = PitchSettings();
  // Quantize Y so Transitions() observes note-clock changes instead of the
  // continuously interpolated ramp between notes.
  settings.ySteps = 100;
  settings.yDivider = 0;
  const PitchRun slowY = RunPitch(settings);
  settings.yDivider = 11;
  const PitchRun fastY = RunPitch(settings);
  assert(Transitions(fastY.y) > Transitions(slowY.y));
  settings = PitchSettings();
  settings.yRange = 0;
  settings.ySpread = 10;
  settings.yBias = 10;
  settings.ySteps = 0;
  const PitchRun yLow = RunPitch(settings);
  settings.yRange = 2;
  settings.ySpread = 90;
  settings.yBias = 90;
  settings.ySteps = 100;
  const PitchRun yHigh = RunPitch(settings);
  assert(Different(yLow.y, yHigh.y));

  settings = PitchSettings();
  settings.xDejaVu = 1;
  settings.dejaVuAmount = 100;
  settings.dejaVuLength = 0;
  const PitchRun loop1 = RunPitch(settings);
  settings.dejaVuLength = 7;
  const PitchRun loop8 = RunPitch(settings);
  assert(Different(loop1.x1, loop8.x1));
  settings.xDejaVu = 2;
  const PitchRun superLock = RunPitch(settings);
  assert(Different(loop8.x1, superLock.x1) ||
         Different(loop8.x3, superLock.x3));
}

void TestOutputModes() {
  Fixture fixture;
  const char* modeNames[] = {
      "T1 Output mode", "X1 Output mode", "T2 Output mode",
      "X2 Output mode", "T3 Output mode", "X3 Output mode",
      "Y Output mode"};
  for (const char* name : modeNames) {
    const _NT_parameter& mode =
        fixture.algorithm->parameters[fixture.parameter(name)];
    assert(mode.unit == kNT_unitOutputMode);
    assert(mode.min == 0 && mode.max == 1 && mode.def == 0);
  }

  const int frames = kTestFrames;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  const int route = fixture.values[fixture.parameter("T1 Output")];
  float* destination = buses.data() + (route - 1) * frames;
  std::fill(destination, destination + frames, 1.0f);
  float* clock = buses.data();
  for (int i = 0; i < frames; ++i) {
    clock[i] = (i / 8) % 2 ? 5.0f : 0.0f;
  }
  fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
  bool addedGate = false;
  for (int i = 0; i < frames; ++i) {
    assert(Near(destination[i], 1.0f) || Near(destination[i], 6.0f));
    addedGate = addedGate || Near(destination[i], 6.0f);
  }
  assert(addedGate);

  fixture.values[fixture.parameter("T1 Output mode")] = 1;
  std::fill(buses.begin(), buses.end(), 0.0f);
  destination = buses.data() + (route - 1) * frames;
  std::fill(destination, destination + frames, 1.0f);
  clock = buses.data();
  for (int i = 0; i < frames; ++i) {
    clock[i] = ((frames + i) / 8) % 2 ? 5.0f : 0.0f;
  }
  fixture.factory->step(fixture.algorithm, buses.data(), frames / 4);
  for (int i = 0; i < frames; ++i) {
    assert(Near(destination[i], 0.0f) || Near(destination[i], 5.0f));
  }
}

void TestControlSurface() {
  Fixture fixture;
  const uint32_t expectedControls =
      kNT_button3 | kNT_potL | kNT_potC | kNT_potR | kNT_potButtonL |
      kNT_potButtonC | kNT_potButtonR | kNT_encoderL | kNT_encoderR |
      kNT_encoderButtonL | kNT_encoderButtonR;
  assert(fixture.factory->hasCustomUi(fixture.algorithm) == expectedControls);
  assert((expectedControls &
          (kNT_button1 | kNT_button2 | kNT_button4)) == 0);

  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("T"));
  assert(TextDrawn("Complementary"));
  assert(TextDrawn("Off"));
  assert(TextDrawn("Rate"));
  assert(TextDrawn("Jitter"));
  assert(TextDrawn("Range"));
  assert(TextDrawnAt(40, 36, "50%"));
  assert(TextDrawnAt(104, 36, "50"));
  assert(TextDrawnAt(181, 36, "0"));
  assert(ShapeDrawn(kNT_box, 0, 0, 255, 63, 8));
  assert(ShapeDrawn(kNT_line, 4, 18, 252, 18, 8));

  _NT_uiData ui = {};
  ui.controls = kNT_potL;
  ui.pots[0] = 0.25f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Rate")] == -30);
  ui = {};
  ui.controls = kNT_potC;
  ui.pots[1] = 0.61f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Bias")] == 61);
  ui = {};
  ui.controls = kNT_potR;
  ui.pots[2] = 0.72f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Jitter")] == 72);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawnAt(40, 36, "25%"));
  assert(TextDrawnAt(104, 36, "61"));
  assert(TextDrawnAt(181, 36, "72"));

  ui = {};
  ui.controls = kNT_potButtonL;
  ui.pots[0] = 0.25f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonL | kNT_potL;
  ui.pots[0] = 0.32f;
  fixture.factory->customUi(fixture.algorithm, ui);
  // The secondary target remains unchanged until the pot crosses its own
  // pickup point, rather than jumping to the physical primary position.
  assert(fixture.values[fixture.parameter("T Range")] == 1);
  ui.pots[0] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[0] = 0.32f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Range")] == 0);
  ui.pots[0] = 0.34f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Range")] == 1);
  ui.pots[0] = 0.67f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Range")] == 2);
  ui = {};
  ui.controls = kNT_potButtonC;
  ui.pots[1] = 0.61f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonC | kNT_potC;
  ui.pots[1] = 0.55f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Length")] == 50);
  ui.pots[1] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[1] = 0.34f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Length")] == 34);
  ui = {};
  ui.controls = kNT_potButtonR;
  ui.pots[2] = 0.72f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonR | kNT_potR;
  ui.pots[2] = 0.45f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Randomness")] == 0);
  ui.pots[2] = 0.0f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[2] = 0.45f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Randomness")] == 45);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawnAt(72, 55, "4x"));
  assert(TextDrawnAt(132, 55, "34"));
  assert(TextDrawnAt(194, 55, "45"));

  const std::vector<int16_t> beforePlainPresses = fixture.values;
  ui = {};
  ui.controls = kNT_potButtonL | kNT_potButtonC | kNT_potButtonR |
                kNT_button1 | kNT_button2 | kNT_button4;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values == beforePlainPresses);

  ui = {};
  ui.encoders[1] = 1;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Model")] == 1);
  ui = {};
  ui.controls = kNT_encoderButtonR;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Deja Vu")] == 1);
  ui.lastButtons = kNT_encoderButtonR;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Deja Vu")] == 1);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("Clusters"));
  assert(TextDrawn("Amount"));

  ui = {};
  ui.encoders[0] = 1;
  ui.pots[0] = 0.67f;
  ui.pots[1] = 0.34f;
  ui.pots[2] = 0.45f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("X"));
  assert(TextDrawn("Identical"));
  assert(TextDrawn("Scale:"));
  assert(TextDrawn("C major"));
  assert(TextDrawn("Off"));
  assert(TextDrawnAt(178, 36, "75"));

  ui = {};
  ui.controls = kNT_potL;
  ui.pots[0] = 0.6f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Spread")] == 50);
  ui.pots[0] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[0] = 0.11f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Spread")] == 11);
  ui = {};
  ui.controls = kNT_potC;
  ui.pots[1] = 0.22f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Bias")] == 50);
  ui.pots[1] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[1] = 0.22f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Bias")] == 22);
  ui = {};
  ui.controls = kNT_potR;
  ui.pots[2] = 0.33f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Steps")] == 75);
  ui.pots[2] = 0.75f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[2] = 0.33f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Steps")] == 33);
  ui = {};
  ui.controls = kNT_potButtonL;
  ui.pots[0] = 0.11f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonL | kNT_potL;
  ui.pots[0] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[0] = 0.44f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Spread")] == 44);
  ui = {};
  ui.controls = kNT_potButtonC;
  ui.pots[1] = 0.22f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonC | kNT_potC;
  ui.pots[1] = 0.5f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[1] = 0.55f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Bias")] == 55);
  ui = {};
  ui.controls = kNT_potButtonR;
  ui.pots[2] = 0.33f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.controls = kNT_potButtonR | kNT_potR;
  ui.pots[2] = 0.0f;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui.pots[2] = 0.66f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Steps")] == 66);
  fixture.values[fixture.parameter("X Scale")] = 3;
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("Pelog"));
  assert(TextDrawnAt(48, 36, "11"));
  assert(TextDrawnAt(110, 36, "22"));
  assert(TextDrawnAt(178, 36, "33"));
  assert(TextDrawnAt(68, 55, "44"));
  assert(TextDrawnAt(127, 55, "55"));
  assert(TextDrawnAt(196, 55, "66"));

  ui = {};
  ui.encoders[1] = 1;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Relationship")] == 1);
  ui = {};
  ui.controls = kNT_encoderButtonR;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Deja Vu")] == 1);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("Bump"));
  assert(TextDrawn("Amount"));

  _NT_float3 pots = {};
  fixture.factory->setupUi(fixture.algorithm, pots);
  assert(Near(pots[0], 0.11f));
  assert(Near(pots[1], 0.22f));
  assert(Near(pots[2], 0.33f));

  ui = {};
  ui.controls = kNT_encoderButtonL;
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("MARBLES OVERVIEW"));
  assert(TextDrawn("L: T/X  R: model / Deja Vu"));
  assert(TextDrawn("B3: help  hold pot: secondary"));

  ui = {};
  ui.controls = kNT_button3;
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("Pots: Spread / Bias / Steps"));
  assert(TextDrawn("Hold: Y Spread / Bias / Steps"));
  assert(ShapeDrawn(kNT_box, 0, 0, 255, 63, 8));
  assert(!ShapeDrawn(kNT_line, 4, 18, 252, 18, 8));

  ui = {};
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("MARBLES OVERVIEW"));

  ui = {};
  ui.encoders[0] = -1;
  fixture.factory->customUi(fixture.algorithm, ui);
  ui = {};
  ui.controls = kNT_button3;
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("Pots: Rate / Bias / Jitter"));
  assert(TextDrawn("Hold: Range / Gate Len / Gate Rand"));
}

void TestHostManagedPresetSurface() {
  Fixture source;
  assert(source.requirements.numParameters == 41);
  assert(source.factory->serialise == nullptr);
  assert(source.factory->deserialise == nullptr);

  for (uint32_t i = 0; i < source.requirements.numParameters; ++i) {
    const _NT_parameter& parameter = source.algorithm->parameters[i];
    source.values[i] = (i % 2 == 0) ? parameter.min : parameter.max;
  }
  const std::vector<int16_t> hostPreset = source.values;

  Fixture restored;
  assert(restored.values.size() == hostPreset.size());
  std::copy(hostPreset.begin(), hostPreset.end(), restored.values.begin());
  assert(restored.values == hostPreset);

  // The processing callback consumes host-owned values but never rewrites
  // them, including disconnected and fully routed selector values.
  const int frames = kTestFrames;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  restored.factory->step(restored.algorithm, buses.data(), frames / 4);
  assert(restored.values == hostPreset);
}

}  // namespace

uint8_t NT_screen[128 * 64];

const _NT_globals NT_globals = {
    NT_TEST_SAMPLE_RATE, NT_TEST_FRAMES, nullptr, 0, 0, 0};

extern "C" int32_t NT_algorithmIndex(const _NT_algorithm* algorithm) {
  return algorithm == gAlgorithm ? 0 : -1;
}

extern "C" uint32_t NT_parameterOffset(void) {
  return 0;
}

extern "C" void NT_setParameterFromUi(uint32_t algorithmIndex,
                                        uint32_t parameter,
                                        int16_t value) {
  assert(algorithmIndex == 0);
  assert(gValues != nullptr);
  assert(parameter < gValues->size());
  (*gValues)[parameter] = value;
}

extern "C" void NT_drawText(int x, int y, const char* text, int,
                             _NT_textAlignment, _NT_textSize) {
  assert(x >= 0 && x < 256);
  assert(y >= 0 && y < 64);
  gDrawnText.push_back({x, y, text ? text : ""});
}

extern "C" void NT_drawShapeI(_NT_shape shape, int x0, int y0, int x1,
                                int y1, int colour) {
  assert(x0 >= 0 && x0 < 256);
  assert(x1 >= 0 && x1 < 256);
  assert(y0 >= 0 && y0 < 64);
  assert(y1 >= 0 && y1 < 64);
  gDrawnShapes.push_back({shape, x0, y0, x1, y1, colour});
}

int main() {
  TestFactoryAndTRate();
  TestClockedTRhythm();
  TestPitchDefaultsAndRoutes();
  TestPitchProcessing();
  TestRhythmParameterEffects();
  TestPitchParameterEffects();
  TestOutputModes();
  TestControlSurface();
  TestHostManagedPresetSurface();
}
