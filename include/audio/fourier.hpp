#pragma once

#include <cstdint>
#include <vector>

#include "audio/wave_data.hpp"

namespace audio {

/**
 * naive implementation of the fourier transform
 * 
 * @param n_samples is the number of sample points in input
 * @param input pointer to the input samples, pointed at the first sample
 * @param in_spacing offset between each sample in input
 * 
 * @param samples_per_sec samples in a second, used for final frequency calculation
 * 
 * @return a vector containing wave_data representing all of the waves in the signal
 * (waves are of form with A cos(2πft + φ))
 */
std::vector<wave_data> naive_fourier_transform(std::size_t n_samples, const float *input, std::size_t in_spacing,
    std::size_t samples_per_sec);

/**
 * like naive_fourier_transform, but using a hanning window to limit spectral leakage
 */
std::vector<wave_data> naive_fourier_transform_hann(std::size_t n_samples, const float *input, std::size_t in_spacing,
    std::size_t samples_per_sec);

}