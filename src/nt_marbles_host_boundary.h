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
