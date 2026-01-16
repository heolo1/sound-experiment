#include "audio/fourier.hpp"

#include <complex>
#include <cstdint>
#include <numbers>
#include <vector>

#include "audio.hpp"

namespace audio {

std::vector<wave_data> naive_ft(uint32_t n_samples, const float *input, uint32_t in_spacing, uint32_t samples_per_sec) {
    // ala nyquist-shannon sampling thm., e.g. a 2 Hz wave requires at least 5 samples to always be represented
    uint32_t max_freq = (n_samples - 1) / 2;
    std::vector<wave_data> waves;
    waves.reserve(max_freq + 1);

    double inv_n_samples = 1. / n_samples;
    double inv_duration = samples_per_sec * inv_n_samples;

    // will need to divide freq by duration at the end to account for assumption of duration = 1s
    for (uint32_t freq = 0; freq <= max_freq; freq++) {
        std::complex<double> sum; // will need to divide by n_samples at the end to get correct amplitude
        for (uint32_t i = 0; i < n_samples; i++) {
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

std::vector<wave_data> naive_ft_hann(uint32_t n_samples, const float *input, uint32_t in_spacing, uint32_t samples_per_sec) {
    // ala nyquist-shannon sampling thm., e.g. a 2 Hz wave requires at least 5 samples to always be represented
    uint32_t max_freq = (n_samples - 1) / 2;
    std::vector<wave_data> waves;
    waves.reserve(max_freq + 1);

    double inv_n_samples = 1. / n_samples;
    double inv_duration = samples_per_sec * inv_n_samples;

    // will need to divide freq by duration at the end to account for assumption of duration = 1s
    for (uint32_t freq = 0; freq <= max_freq; freq++) {
        std::complex<double> sum; // will need to divide by n_samples at the end to get correct amplitude
        for (uint32_t i = 0; i < n_samples; i++) {
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

stft_result<double> naive_stft(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec, uint32_t n_window_offset, bool truncate, uint32_t n_window_size) {
    if (n_window_size == 0) {
        n_window_size = n_samples / n_window_offset;
    }

    if (n_window_size > n_samples) {
        return {};
    }

    std::vector<wave_data> waves;
    uint32_t n_freq = (n_window_size - 1) / 2;
    uint32_t n_signals = (n_samples - n_window_size) / n_window_offset + 1;

    uint32_t curr_off = 0;
    for (; curr_off + n_window_size <= n_samples; curr_off += n_window_offset) {
        auto signal = naive_ft(n_window_size, input + curr_off * in_spacing, in_spacing, samples_per_sec);
        waves.insert(waves.end(), signal.begin(), signal.end());
    }

    std::vector<std::vector<wave_data>> trunc_waves;

    // curr_off should now be at first offset where the window doesn't fully fit
    if (truncate) {
        for (; curr_off < n_samples; curr_off += n_window_offset) {
            auto n_window_size = n_samples - curr_off;
            trunc_waves.push_back(naive_ft(n_window_size, input + curr_off * in_spacing, in_spacing, samples_per_sec));
        }
    }

    return {
        waves,
        n_freq,
        n_signals,
        static_cast<double>(samples_per_sec) / n_window_size, // freq_delta = 1 / window_duration
        static_cast<double>(samples_per_sec) / n_window_size * n_freq, // freq_range = n_freq / window_duration
        static_cast<double>(n_window_offset) / samples_per_sec, // time_delta = window_duration
        static_cast<double>(n_window_offset) / samples_per_sec * n_signals, // time_range = n_signals * window_duration
        trunc_waves,
        uint32_t(n_signals + trunc_waves.size()) // n_total_signals
    };
}

}