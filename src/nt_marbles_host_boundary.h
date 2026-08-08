#ifndef NT_MARBLES_HOST_BOUNDARY_H_
#define NT_MARBLES_HOST_BOUNDARY_H_

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

inline int ControlBlockFrames(float sampleRate) {
  // Marbles runs its five-frame render block at 32 kHz. Preserve that
  // control-call cadence when the NT runs at another sample rate.
  const int frames = static_cast<int>(sampleRate * (5.0f / 32000.0f) + 0.5f);
  return frames < 1 ? 1 : frames;
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
