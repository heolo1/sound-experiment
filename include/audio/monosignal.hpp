#pragma once

#include <cstdint>
#include <tuple>
#include <vector>

#include "miniaudio/miniaudio.h"

#include "audio/base.hpp"

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

} // namespace sound