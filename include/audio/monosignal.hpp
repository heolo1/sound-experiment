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
    std::vector<wave_data> fourier_transform() const;
    double duration() const;
    void play() const;
};

/**
 * takes in a vector of cosine wave data and converts it to a monosignal 
 * if normalize is true, then all values will be divided by the maximum absolute value in the case that it is greater than 1, or if always_normalize is true.
 */
template <typename T>
monosignal generate_monosignal(const std::vector<wave_data_t<T>> &waves, double time = 1, uint32_t samples_per_sec = 44100,
    bool normalize = false, bool always_normalize = false);

extern template monosignal generate_monosignal<double>(const std::vector<wave_data> &, double, uint32_t, bool, bool);
extern template monosignal generate_monosignal<float>(const std::vector<wave_dataf> &, double, uint32_t, bool, bool);

/**
 * like generate_monosignal, but running calculations on the gpu
 * also, no options for normalize (why did i even include that in the first place? what's the point of that?)
 */
monosignal generate_monosignal_gpu(const std::vector<wave_dataf> &waves, double time = 1, uint32_t samples_per_sec = 44100);

} // namespace sound