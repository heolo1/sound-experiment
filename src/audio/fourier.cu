#include "audio/fourier.hpp"

#include <stdio.h>
#include <cstdint>
#include <cuda/std/complex>
#include <numbers>
#include <vector>

#include "audio.hpp"
#include "gpu.hpp"

namespace audio {

__global__
static void compute_nft(uint32_t n_samples, const float *input, uint32_t in_spacing, uint32_t samples_per_sec,
    uint32_t n_freq, wave_dataf *out) {
    uint32_t freq = threadIdx.x + blockIdx.x * blockDim.x;
    if (freq >= n_freq) {
        return;
    }

    float inv_n_samples = 1.f / n_samples;
    float freq_div_n_samples = freq * inv_n_samples;
    float inv_duration = samples_per_sec * inv_n_samples;
    
    cuda::std::complex<float> sum;
    for (uint32_t i = 0; i < n_samples; i++) {
        sum += input[i * in_spacing] * cuda::std::polar<float>(1, -2 * std::numbers::pi_v<float> * freq_div_n_samples * i);
    }
    
    out[freq].freq = freq * inv_duration;
    out[freq].amplitude = cuda::std::abs(sum) * inv_n_samples * 2;
    out[freq].phase = cuda::std::arg(sum);
}

std::vector<wave_dataf> naive_ft_gpu(uint32_t n_samples, const float *input, uint32_t in_spacing, uint32_t samples_per_sec) {
    float *g_input;
    cudaMalloc(&g_input, sizeof(float) * n_samples);
    cudaMemcpy(g_input, input, sizeof(float) * n_samples, cudaMemcpyHostToDevice);
    
    uint32_t n_freq = (n_samples - 1) / 2 + 1;
    wave_dataf *g_waves;
    cudaMalloc(&g_waves, sizeof(wave_dataf) * n_freq);

    int grid_size = n_freq / gpu::BLOCK_SIZE + 1;
    compute_nft<<<grid_size, gpu::BLOCK_SIZE>>>(n_samples, g_input, in_spacing, samples_per_sec, n_freq, g_waves);
    cudaDeviceSynchronize();

    std::vector<wave_dataf> waves{n_freq};
    cudaMemcpy(waves.data(), g_waves, sizeof(wave_dataf) * n_freq, cudaMemcpyDeviceToHost);
    
    cudaFree(g_input);
    cudaFree(g_waves);
    
    return waves;
}

__global__
static void compute_nstft(const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec, uint32_t n_window_offset, uint32_t n_window_size,
    uint32_t n_freq, uint32_t n_signal, wave_dataf *out) {
    uint32_t freq = threadIdx.x + blockIdx.x * blockDim.x;
    
    if (freq >= n_freq) {
        return;
    }

    uint32_t signal = threadIdx.y + blockIdx.y * blockDim.y;
    if (signal >= n_signal) {
        return;
    }

    uint32_t wave = signal * n_freq + freq;
    
    float inv_n_window_size = 1.f / n_window_size;
    float freq_div_n_samples = freq * inv_n_window_size;
    float inv_duration = samples_per_sec * inv_n_window_size;
    
    input += signal * n_window_offset * in_spacing;
    cuda::std::complex<float> sum;
    for (uint32_t i = 0; i < n_window_size; i++) {
        sum += input[i * in_spacing] * cuda::std::polar<float>(1, -2 * std::numbers::pi_v<float> * freq_div_n_samples * i);
    }
    
    out[wave].freq = freq * inv_duration;
    out[wave].amplitude = cuda::std::abs(sum) * inv_n_window_size * 2;
    out[wave].phase = cuda::std::arg(sum);
}

stft_result<float> naive_stft_gpu(uint32_t n_samples, const float *input, uint32_t in_spacing,
    uint32_t samples_per_sec, uint32_t n_window_offset, bool truncate, uint32_t n_window_size) {
    if (n_window_size == 0) {
        n_window_size = n_samples / n_window_offset;
    }

    uint32_t n_freq = (n_window_size - 1) / 2 + 1;

    if (n_window_size > n_samples) {
        return {
            {},
            n_freq,
            0,
            static_cast<float>(samples_per_sec) / n_window_size, // freq_delta = 1 / window_duration
            static_cast<float>(samples_per_sec) / n_window_size * n_freq, // freq_range = n_freq / window_duration
            static_cast<float>(n_window_offset) / samples_per_sec, // time_delta = window_duration
            0, // time_range = n_signals * window_duration
            {},
            0 // n_total_signals
        };
    }

    uint32_t n_signal = (n_samples - n_window_size) / n_window_offset + 1;
    uint32_t n_waves = n_freq * n_signal;

    float *g_input;
    cudaMalloc(&g_input, sizeof(float) * n_samples);
    cudaMemcpy(g_input, input, sizeof(float) * n_samples, cudaMemcpyHostToDevice);

    wave_dataf *g_waves;
    cudaMalloc(&g_waves, sizeof(wave_dataf) * n_waves);

    dim3 grid_size(n_freq / gpu::BLOCK_SIZE_2D + 1, n_signal / gpu::BLOCK_SIZE_2D + 1);
    compute_nstft<<<grid_size, dim3(gpu::BLOCK_SIZE_2D, gpu::BLOCK_SIZE_2D)>>>(
        g_input, in_spacing,
        samples_per_sec, n_window_offset, n_window_size,
        n_freq, n_signal, g_waves);
    cudaDeviceSynchronize();
    std::cout << std::endl;

    std::vector<wave_dataf> waves{n_waves};
    cudaMemcpy(waves.data(), g_waves, sizeof(wave_dataf) * n_waves, cudaMemcpyDeviceToHost);

    cudaFree(g_waves);
    cudaFree(g_input);

    return {
        waves,
        n_freq,
        n_signal,
        static_cast<float>(samples_per_sec) / n_window_size, // freq_delta = 1 / window_duration
        static_cast<float>(samples_per_sec) / n_window_size * n_freq, // freq_range = n_freq / window_duration
        static_cast<float>(n_window_offset) / samples_per_sec, // time_delta = window_duration
        static_cast<float>(n_window_offset) / samples_per_sec * n_signal, // time_range = n_signal * window_duration
        {}, // unimplemented
        uint32_t(n_signal + 0 /* trunc_waves.size() */) // n_total_signals
    };
}

}