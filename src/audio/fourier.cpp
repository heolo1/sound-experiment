#include "audio/fourier.hpp"

#include <complex>
#include <cstdint>
#include <numbers>
#include <vector>

#include "audio.hpp"

namespace audio {

std::vector<wave_data> naive_fourier_transform(std::size_t n_samples, const float *input, std::size_t in_spacing, std::size_t samples_per_sec) {
    // ala nyquist-shannon sampling thm., e.g. a 2 Hz wave requires at least 5 samples to always be represented
    std::size_t max_freq = (n_samples - 1) / 2;
    std::vector<wave_data> waves;
    waves.reserve(max_freq + 1);

    double inv_n_samples = 1. / n_samples;
    double inv_duration = samples_per_sec * inv_n_samples;

    // will need to divide freq by duration at the end to account for assumption of duration = 1s
    for (std::size_t freq = 0; freq <= max_freq; freq++) {
        std::complex<double> sum; // will need to divide by n_samples at the end to get correct amplitude
        for (std::size_t i = 0; i < n_samples; i++) {
            sum += static_cast<double>(input[i * in_spacing]) * std::polar(1., -2 * std::numbers::pi * freq * i / n_samples);
        }
        
        waves.emplace_back(freq * inv_duration, std::abs(sum) * inv_n_samples * 2, std::arg(sum));
    }

    // recall that transform for freq and -freq collapse to double the transform of 0 Hz
    // in the usual case, we have to double sum to get the correct amplitude of the wave
    // but, since we doubled the transform of everything, we have to divide the amplitude of 0 Hz by 2 to get the correct value
    waves[0].amplitude /= 2;
    return waves;
}

std::vector<wave_data> naive_fourier_transform_hann(std::size_t n_samples, const float *input, std::size_t in_spacing, std::size_t samples_per_sec) {
    // ala nyquist-shannon sampling thm., e.g. a 2 Hz wave requires at least 5 samples to always be represented
    std::size_t max_freq = (n_samples - 1) / 2;
    std::vector<wave_data> waves;
    waves.reserve(max_freq + 1);

    double inv_n_samples = 1. / n_samples;
    double inv_duration = samples_per_sec * inv_n_samples;

    // will need to divide freq by duration at the end to account for assumption of duration = 1s
    for (std::size_t freq = 0; freq <= max_freq; freq++) {
        std::complex<double> sum; // will need to divide by n_samples at the end to get correct amplitude
        for (std::size_t i = 0; i < n_samples; i++) {
            auto t = 2 * std::numbers::pi * i / n_samples;
            sum += static_cast<double>(input[i * in_spacing])
                * std::polar(1., -t * freq)
                * (0.5 - std::cos(t) / 2);
        }
        
        // multiply by 4 instead of 2 to account for hann window
        waves.emplace_back(freq * inv_duration, std::abs(sum) * inv_n_samples * 4, std::arg(sum));
    }

    // recall that transform for freq and -freq collapse to double the transform of 0 Hz
    // in the usual case, we have to double sum to get the correct amplitude of the wave
    // but, since we doubled the transform of everything, we have to divide the amplitude of 0 Hz by 2 to get the correct value
    waves[0].amplitude /= 2;
    return waves;
}

}