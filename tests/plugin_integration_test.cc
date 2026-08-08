#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "distingnt/api.h"
#include "marbles_factory_scales.h"

namespace {

_NT_algorithm* gAlgorithm = nullptr;
std::vector<int16_t>* gValues = nullptr;
std::vector<std::string> gDrawnText;

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
  return std::find(gDrawnText.begin(), gDrawnText.end(), expected) !=
      gDrawnText.end();
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

void TestFactoryAndTRate() {
  Fixture fixture;
  const int rate = fixture.parameter("T Rate");
  const int model = fixture.parameter("T Model");
  assert(fixture.algorithm->parameters[rate].min == -60);
  assert(fixture.algorithm->parameters[rate].max == 60);
  assert(fixture.algorithm->parameters[rate].def == 0);
  assert(fixture.algorithm->parameters[model].max == 5);

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
  const int frames = 64;
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

PitchRun RunPitch(int scale, int xClockRoute, bool registerMode,
                  float registerVolts) {
  Fixture fixture;
  const int frames = 64;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  fixture.values[fixture.parameter("X Scale")] = scale;
  fixture.values[fixture.parameter("X Clock")] = xClockRoute;
  fixture.values[fixture.parameter("X Spread")] = 80;
  fixture.values[fixture.parameter("X Bias")] = 37;
  // Maximum quantization strength retains only the highest-weight root notes,
  // which are shared by every factory scale. A mid setting exercises each
  // scale's distinctive note set.
  fixture.values[fixture.parameter("X Steps")] = 75;
  fixture.values[fixture.parameter("X Register")] = registerMode ? 1 : 0;
  fixture.values[fixture.parameter("X Register CV")] = registerMode ? 3 : 0;
  fixture.values[fixture.parameter("Y Output")] = 21;

  PitchRun result;
  for (int block = 0; block < 256; ++block) {
    std::fill(buses.begin(), buses.end(), 0.0f);
    float* clock = buses.data();
    float* registerBus = buses.data() + 2 * frames;
    for (int i = 0; i < frames; ++i) {
      clock[i] = ((block * frames + i) / 8) % 2 ? 5.0f : 0.0f;
      registerBus[i] = registerVolts;
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

float Mean(const std::vector<float>& values) {
  assert(!values.empty());
  float total = 0.0f;
  for (float value : values) total += value;
  return total / static_cast<float>(values.size());
}

void TestPitchDefaultsAndRoutes() {
  Fixture fixture;
  assert(fixture.values[fixture.parameter("X Range")] == 2);
  assert(fixture.values[fixture.parameter("Y Range")] == 2);
  assert(fixture.values[fixture.parameter("Y Steps")] == 0);
  assert(fixture.values[fixture.parameter("Y Divider")] == 6);
  assert(fixture.values[fixture.parameter("X Scale")] == 0);
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
  assert(TextDrawn("T RHYTHM"));
  assert(TextDrawn("Complementary"));
  assert(TextDrawn("Off"));
  assert(TextDrawn("Rate       Bias       Jitter"));
  assert(TextDrawn("hold: Range  Gate Len  Gate Rand"));
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

  ui = {};
  ui.controls = kNT_potButtonL | kNT_potL;
  ui.pots[0] = 1.0f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Range")] == 2);
  ui = {};
  ui.controls = kNT_potButtonC | kNT_potC;
  ui.pots[1] = 0.34f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Length")] == 34);
  ui = {};
  ui.controls = kNT_potButtonR | kNT_potR;
  ui.pots[2] = 0.45f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("T Gate Randomness")] == 45);

  const std::vector<int16_t> beforePlainPresses = fixture.values;
  ui = {};
  ui.controls = kNT_potButtonL | kNT_potButtonC | kNT_potButtonR |
                kNT_button1 | kNT_button2 | kNT_button4;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values == beforePlainPresses);

  ui = {};
  ui.controls = kNT_encoderR;
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

  ui = {};
  ui.controls = kNT_encoderL;
  ui.encoders[0] = 1;
  fixture.factory->customUi(fixture.algorithm, ui);
  ClearDrawCalls();
  assert(fixture.factory->draw(fixture.algorithm));
  assert(TextDrawn("X PITCH"));
  assert(TextDrawn("Identical"));
  assert(TextDrawn("Off"));
  assert(TextDrawn("Spread     Bias       Steps"));
  assert(TextDrawn("hold: Y Spread  Y Bias  Y Steps"));

  ui = {};
  ui.controls = kNT_potL;
  ui.pots[0] = 0.11f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Spread")] == 11);
  ui = {};
  ui.controls = kNT_potC;
  ui.pots[1] = 0.22f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Bias")] == 22);
  ui = {};
  ui.controls = kNT_potR;
  ui.pots[2] = 0.33f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("X Steps")] == 33);
  ui = {};
  ui.controls = kNT_potButtonL | kNT_potL;
  ui.pots[0] = 0.44f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Spread")] == 44);
  ui = {};
  ui.controls = kNT_potButtonC | kNT_potC;
  ui.pots[1] = 0.55f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Bias")] == 55);
  ui = {};
  ui.controls = kNT_potButtonR | kNT_potR;
  ui.pots[2] = 0.66f;
  fixture.factory->customUi(fixture.algorithm, ui);
  assert(fixture.values[fixture.parameter("Y Steps")] == 66);

  ui = {};
  ui.controls = kNT_encoderR;
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
  ui.controls = kNT_encoderL;
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
  assert(source.requirements.numParameters == 34);
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
  const int frames = 64;
  std::vector<float> buses(kNT_lastBus * frames, 0.0f);
  restored.factory->step(restored.algorithm, buses.data(), frames / 4);
  assert(restored.values == hostPreset);
}

}  // namespace

uint8_t NT_screen[128 * 64];

const _NT_globals NT_globals = {
    48000, 256, nullptr, 0, 0, 0};

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

extern "C" void NT_drawText(int, int, const char* text, int,
                             _NT_textAlignment, _NT_textSize) {
  gDrawnText.push_back(text ? text : "");
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
  TestControlSurface();
  TestHostManagedPresetSurface();
}
