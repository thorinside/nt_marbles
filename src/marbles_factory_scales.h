#ifndef NT_MARBLES_FACTORY_SCALES_H_
#define NT_MARBLES_FACTORY_SCALES_H_

#include "marbles/random/quantizer.h"

namespace nt_marbles {

constexpr int kNumFactoryScales = 6;

// Exact factory scale data from the pinned marbles/settings.cc. Keeping the
// immutable data at the NT boundary avoids linking Marbles' STM32 flash-storage
// implementation while still loading the original quantizers verbatim.
const marbles::Scale kFactoryScales[kNumFactoryScales] = {
    {1.0f, 12, {{0.0000f, 255}, {0.0833f, 16}, {0.1667f, 96},
                 {0.2500f, 24}, {0.3333f, 128}, {0.4167f, 64},
                 {0.5000f, 8}, {0.5833f, 192}, {0.6667f, 16},
                 {0.7500f, 96}, {0.8333f, 24}, {0.9167f, 128}}},
    {1.0f, 12, {{0.0000f, 255}, {0.0833f, 16}, {0.1667f, 96},
                 {0.2500f, 128}, {0.3333f, 8}, {0.4167f, 64},
                 {0.5000f, 4}, {0.5833f, 192}, {0.6667f, 96},
                 {0.7500f, 16}, {0.8333f, 128}, {0.9167f, 16}}},
    {1.0f, 12, {{0.0000f, 255}, {0.0833f, 4}, {0.1667f, 96},
                 {0.2500f, 4}, {0.3333f, 4}, {0.4167f, 140},
                 {0.5000f, 4}, {0.5833f, 192}, {0.6667f, 4},
                 {0.7500f, 96}, {0.8333f, 4}, {0.9167f, 4}}},
    {1.0f, 7, {{0.0000f, 255}, {0.1275f, 128}, {0.2625f, 32},
                {0.4600f, 8}, {0.5883f, 192}, {0.7067f, 64},
                {0.8817f, 16}}},
    {1.0f, 12, {{0.0000f, 255}, {0.0752f, 128}, {0.1699f, 4},
                 {0.2630f, 4}, {0.3219f, 128}, {0.4150f, 64},
                 {0.4918f, 4}, {0.5850f, 192}, {0.6601f, 64},
                 {0.7549f, 4}, {0.8479f, 4}, {0.9069f, 64}}},
    {1.0f, 12, {{0.0000f, 255}, {0.0752f, 4}, {0.1699f, 128},
                 {0.2630f, 64}, {0.3219f, 4}, {0.4150f, 128},
                 {0.4918f, 4}, {0.5850f, 192}, {0.6601f, 4},
                 {0.7549f, 64}, {0.8479f, 128}, {0.9069f, 4}}},
};

}  // namespace nt_marbles

#endif  // NT_MARBLES_FACTORY_SCALES_H_
