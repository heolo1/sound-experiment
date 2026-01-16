#pragma once

#include <cstdint>
#include <tuple>
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
std::vector<wave_data> naive_ft(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec);

/**
 * like naive_ft, but using a hanning window to limit spectral leakage
 */
std::vector<wave_data> naive_ft_hann(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec);

/**
 * like naive_ft, but running calculations on the gpu
 */
std::vector<wave_dataf> naive_ft_gpu(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec);

template <typename T>
struct stft_result {
    std::vector<wave_data_t<T>> waves; // big vector containing all wave data of all regular sized signals
    uint32_t n_freq; // number of frequencies in a standard signal 
    uint32_t n_signals; // number of signals in waves, waves.size() = n_freq * n_signals
    
    T freq_delta; // step in Hz between each wave_data in a signal = 1 / duration
    T freq_range; // the frequency range of waves, i.e. n_freq * freq_delta
    T time_delta; // time step between each signal
    T time_range; // time_delta * n_signals

    // vector containing vectors of wave datas for truncated signals with variable lengths
    // truncated signals are ordered in time
    // in naive_stft calls with truncate = false, this be empty
    std::vector<std::vector<wave_data_t<T>>> trunc_waves;
    uint32_t n_total_signals; // n_signals + trunc_waves.size()
};

/**
 * naive implementation of the short-time fourier transform
 * 
 * @param n_samples is the number of sample points in input
 * @param input pointer to the input samples, pointed at the first sample
 * @param in_spacing offset between each sample in input
 * 
 * @param samples_per_sec samples in a second, used for final frequency calculation
 * 
 * @param n_window_offset is the offset between windows in the short-time fourier transform in samples
 * @param truncate is whether or not to perform the ft on windows with less than the asked number of samples.
 * if truncate is false, then this will skip windows at the end with less than the required number of samples
 * if truncate is true, then this will not skip windows at the end with less than the required number of samples
 * @param n_window_size is the size of the window to perform the fourier transform on.
 * if window_size is given as 0, then it is defaulted to n_samples/n_window_offset
 * 
 * @return a vector containing pairs of window start time and info of the waves the window.
 * (waves are of form with A cos(2πft + φ))
 */
stft_result<double> naive_stft(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec, uint32_t n_window_offset, bool truncate = false, uint32_t n_window_size = 0);

/**
 * like naive_stft, but running calculations on the gpu
 */
stft_result<float> naive_stft_gpu(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec, uint32_t n_window_offset, bool truncate = false, uint32_t n_window_size = 0);

}