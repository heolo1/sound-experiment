#pragma once

#include <cstdint>
#include <tuple>
#include <vector>

#include "miniaudio/miniaudio.h"

#include "audio/base.hpp"
#include "audio/wave_data.hpp"

namespace audio {

/**
 * represents a waveform with:
 * 1 channel
 * float32 data, all between -1 and 1
 */
struct monosignal {
    uint32_t samples_per_sec;
    std::vector<float> data;

    audio_buffer make_audio_buffer() const;
};

/**
 * takes in a vector of cosine wave data and converts it to a monosignal 
 * if normalize is true, then all values will be divided by the maximum absolute value in the case that it is greater than 1, or if always_normalize is true.
 */
monosignal generate_monosignal(const std::vector<wave_data> &waves, double time = 1, uint32_t samples_per_sec = 44100,
    bool normalize = false, bool always_normalize = false);

} // namespace sound