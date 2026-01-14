#include "audio/fourier.hpp"

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

std::vector<wave_dataf> naive_fourier_transform_gpu(uint32_t n_samples, const float *input, uint32_t in_spacing, uint32_t samples_per_sec) {
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

}