#include <cuda/std/cmath>
#include <cstdint>
#include <tuple>
#include <vector>

#include "audio.hpp"
#include "gpu.hpp"

namespace audio {

__global__
void compute_gm(uint32_t n_waves, const wave_dataf *input, uint32_t samples_per_sec,
    uint32_t n_samples, float *out) {
    uint32_t sample = threadIdx.x + blockIdx.x * blockDim.x;
    if (sample >= n_samples) {
        return;
    }

    auto time_step = 1.f / samples_per_sec;
    for (uint32_t i = 0; i < n_waves; i++) {
        out[sample] += input[i].amplitude * cuda::std::cos(2 * std::numbers::pi_v<float> * time_step * input[i].freq * sample + input[i].phase);
    }
}

monosignal generate_monosignal_gpu(const std::vector<wave_dataf> &waves, double time, uint32_t samples_per_sec) {
    wave_dataf *g_waves;
    cudaMalloc(&g_waves, sizeof(wave_dataf) * waves.size());
    cudaMemcpy(g_waves, waves.data(), sizeof(wave_dataf) * waves.size(), cudaMemcpyHostToDevice);

    uint32_t n_samples = time * samples_per_sec;
    float *g_samples;
    cudaMalloc(&g_samples, sizeof(float) * n_samples);

    int grid_size = n_samples / gpu::BLOCK_SIZE + 1;
    compute_gm<<<grid_size, gpu::BLOCK_SIZE>>>(waves.size(), g_waves, samples_per_sec, n_samples, g_samples);
    cudaDeviceSynchronize();

    monosignal sig{samples_per_sec, std::vector(n_samples, 0.f)};
    cudaMemcpy(sig.data.data(), g_samples, sizeof(float) * n_samples, cudaMemcpyDeviceToHost);

    cudaFree(g_samples);
    cudaFree(g_waves);

    return sig;
}

}