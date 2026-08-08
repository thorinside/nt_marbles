#ifndef NT_MARBLES_HOST_BOUNDARY_H_
#define NT_MARBLES_HOST_BOUNDARY_H_

#include <cstdint>

namespace nt_marbles {

inline float Clamp(float value, float minimum, float maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}

inline float TRateFromPot(float normalized) {
  return Clamp(normalized, 0.0f, 1.0f) * 120.0f - 60.0f;
}

inline float PotFromTRate(float semitones) {
  return (Clamp(semitones, -60.0f, 60.0f) + 60.0f) / 120.0f;
}

inline int TRangeFromPot(float normalized) {
  const float bounded = Clamp(normalized, 0.0f, 1.0f);
  const int range = static_cast<int>(bounded * 3.0f);
  return range > 2 ? 2 : range;
}

inline int NextControlIntervalFrames(int sampleRate, int* remainder) {
  // Marbles runs its five-frame render block at 32 kHz. Bresenham-style
  // intervals preserve that elapsed cadence exactly on average at NT sample
  // rates where the equivalent frame count is fractional (48 kHz: 7, 8...).
  const int64_t total = static_cast<int64_t>(sampleRate) * 5 + *remainder;
  int frames = static_cast<int>(total / 32000);
  *remainder = static_cast<int>(total % 32000);
  if (frames < 1) frames = 1;
  return frames;
}

inline float RegisterValueFromVolts(float volts) {
  return Clamp(volts * 0.1f + 0.5f, 0.0f, 1.0f);
}

template <typename T>
inline T InterleavedSample(const T* values, int frame, int channel,
                           int channels) {
  return values[frame * channels + channel];
}

}  // namespace nt_marbles

#endif  // NT_MARBLES_HOST_BOUNDARY_H_
