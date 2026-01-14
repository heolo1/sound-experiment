#include <algorithm>
#include <chrono>
#include <iostream>
#include <tuple>

#include <SDL2/SDL.h>

#include "audio.hpp"
#include "gpu.hpp"

void play_monosig(const audio::monosignal &ms) {
    auto ab = ms.make_audio_buffer();
    audio::sound_obj sound(audio::global_engine, ab);

    ma_sound_start(&sound);
    while (ma_sound_is_playing(&sound));
}

auto time(auto lambda, const char *str) {
    namespace chr = std::chrono;
    auto start = chr::system_clock::now();
    auto ret = lambda();
    auto dur = chr::system_clock::now() - start;
    std::cout << str << " computed in " << chr::duration_cast<chr::milliseconds>(dur) << std::endl;
    return ret;
}

template <typename T>
std::tuple<SDL_Window *, SDL_Renderer *, SDL_Texture *>
display_fourier(const std::vector<audio::wave_data_t<T>> &v, const char *name) {
    SDL_Window   *window   = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_TARGET, 800, 400);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    auto rect_w = 800. / v.size();
    for (std::size_t i = 0; i < v.size(); i++) {
        // we want around 0.1 amp to be full height
        int height = std::min<T>(1., v[i].amplitude * 10) * 400;
        SDL_FRect rect{float(i * rect_w), float(400 - height), float(rect_w), float(height)};
        SDL_RenderDrawRectF(renderer, &rect);
    }

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 25, 0, 25, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    return { window, renderer, texture };
}

void destroy(std::tuple<SDL_Window *, SDL_Renderer *, SDL_Texture *> &t) {
    SDL_DestroyTexture(get<2>(t));
    SDL_DestroyRenderer(get<1>(t));
    SDL_DestroyWindow(get<0>(t));
}

int main() {
    using namespace audio;

    SDL_Init(SDL_INIT_EVERYTHING);

    gpu::wakeup();

    auto squeak_wav = read_wav_from_file("data/squeak-clip.wav");
    auto squeak_ms = squeak_wav.to_monosignal();
    
    auto cpu_fourier = time([&]{
        return naive_fourier_transform(squeak_ms.data.size(), squeak_ms.data.data(), 1, squeak_ms.samples_per_sec);
    }, "CPU fourier");

    auto cpu_fourier_display = display_fourier(cpu_fourier, "CPU fourier");

    auto gpu_fourier = time([&]{
        return naive_fourier_transform_gpu(squeak_ms.data.size(), squeak_ms.data.data(), 1, squeak_ms.samples_per_sec);
    }, "GPU fourier");

    auto gpu_fourier_display = display_fourier(gpu_fourier, "GPU fourier");

    std::cout << "Building reconstructions..." << std::endl;
    auto cc_recon = time([&]{
        return generate_monosignal(cpu_fourier, squeak_ms.duration() * 2);
    }, "CPU reconstruction of CPU fourier");
    auto cg_recon = time([&]{
        return generate_monosignal(gpu_fourier, squeak_ms.duration() * 2);
    }, "CPU reconstruction of GPU fourier");
    auto gg_recon = time([&]{
        return generate_monosignal_gpu(gpu_fourier, squeak_ms.duration() * 2);
    }, "GPU reconstruction of GPU fourier");

    std::cout << "Original:" << std::endl;
    play_monosig(squeak_ms);
    std::cout << "CPU reconstructed of CPU fourier:" << std::endl;
    play_monosig(cc_recon);
    std::cout << "CPU reconstructed of GPU fourier:" << std::endl;
    play_monosig(cg_recon);
    std::cout << "GPU reconstructed of GPU fourier:" << std::endl;
    play_monosig(gg_recon);

    std::cout << "Waiting for input...";
    std::getchar();

    destroy(cpu_fourier_display);
    destroy(gpu_fourier_display);
    SDL_Quit();
}