#include <cassert>
#include <cmath>

#include "src/nt_marbles_host_boundary.h"

namespace {

bool Near(float actual, float expected) {
  return std::fabs(actual - expected) < 0.00001f;
}

}  // namespace

int main() {
  const float voltages[] = {1.0f, 2.0f, 3.0f, 4.0f,
                            11.0f, 12.0f, 13.0f, 14.0f};
  for (int channel = 0; channel < 4; ++channel) {
    assert(Near(nt_marbles::InterleavedSample(voltages, 0, channel, 4),
                static_cast<float>(channel + 1)));
    assert(Near(nt_marbles::InterleavedSample(voltages, 1, channel, 4),
                static_cast<float>(channel + 11)));
  }

  const bool gates[] = {true, false, false, true};
  assert(nt_marbles::InterleavedSample(gates, 0, 0, 2));
  assert(!nt_marbles::InterleavedSample(gates, 0, 1, 2));
  assert(!nt_marbles::InterleavedSample(gates, 1, 0, 2));
  assert(nt_marbles::InterleavedSample(gates, 1, 1, 2));

  assert(Near(nt_marbles::TRateFromPot(0.0f), -60.0f));
  assert(Near(nt_marbles::TRateFromPot(0.5f), 0.0f));
  assert(Near(nt_marbles::TRateFromPot(1.0f), 60.0f));
  assert(Near(nt_marbles::PotFromTRate(-60.0f), 0.0f));
  assert(Near(nt_marbles::PotFromTRate(0.0f), 0.5f));
  assert(Near(nt_marbles::PotFromTRate(60.0f), 1.0f));

  assert(nt_marbles::TRangeFromPot(-1.0f) == 0);
  assert(nt_marbles::TRangeFromPot(0.32f) == 0);
  assert(nt_marbles::TRangeFromPot(0.34f) == 1);
  assert(nt_marbles::TRangeFromPot(0.66f) == 1);
  assert(nt_marbles::TRangeFromPot(0.67f) == 2);
  assert(nt_marbles::TRangeFromPot(1.0f) == 2);
  assert(nt_marbles::ControlBlockFrames(32000.0f) == 5);
  assert(nt_marbles::ControlBlockFrames(48000.0f) == 8);
  assert(nt_marbles::ControlBlockFrames(96000.0f) == 15);

  assert(Near(nt_marbles::RegisterValueFromVolts(-8.0f), 0.0f));
  assert(Near(nt_marbles::RegisterValueFromVolts(-5.0f), 0.0f));
  assert(Near(nt_marbles::RegisterValueFromVolts(0.0f), 0.5f));
  assert(Near(nt_marbles::RegisterValueFromVolts(5.0f), 1.0f));
  assert(Near(nt_marbles::RegisterValueFromVolts(8.0f), 1.0f));
}
