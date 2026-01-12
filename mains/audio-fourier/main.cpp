#include <algorithm>
#include <iostream>

#include <SDL2/SDL.h>

#include "audio.hpp"

using namespace audio;

std::ostream &operator<<(std::ostream &os, const wave_data &wd) {
    return os << wd.freq << " Hz @ " << wd.amplitude << " + " << wd.phase << " rad"; 
}

void print_wave_data(const std::vector<wave_data> &waves) {
    for (const auto &wd : waves) {
        if (wd.amplitude > 1e-8) {
            std::cout << wd << '\n';
        }
    }
}

void play_monosig(const monosignal &ms) {
    auto ab = ms.make_audio_buffer();
    sound_obj sound(global_engine, ab);

    ma_sound_start(&sound);
    while (ma_sound_is_playing(&sound));
}

int main() {
    std::vector<wave_data> orig_waves{
        {0, 0.1, -0.2}, // the phase should collapse into the amplitude as A cos φ
        {9, 0.3, 0.4},
        {13, 0.4, 1},
        {18, 0.2, -0.5}
    };

    std::cout << "Waves:\n";
    print_wave_data(orig_waves);

    auto signal = generate_monosignal(orig_waves, 1, 60);
    auto fourier_waves = signal.fourier_transform();
    std::cout << "Fourier Waves:\n";
    print_wave_data(fourier_waves);

    auto squeak_wav = read_wav_from_file("data/squeak-clip.wav");
    auto squeak_ms = squeak_wav.to_monosignal();
    std::cout << "Computing squeak fourier..." << std::flush;
    auto squeak_fourier = naive_fourier_transform(squeak_ms.data.size(), squeak_ms.data.data(), 1, squeak_ms.samples_per_sec);
    std::cout << " Done.\n";

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window   *window   = SDL_CreateWindow("Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *freq_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_TARGET, 800, 400);
    SDL_SetTextureBlendMode(freq_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, freq_texture);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    auto rect_w = 800. / squeak_fourier.size();
    for (std::size_t i = 0; i < squeak_fourier.size(); i++) {
        // we want around 0.1 amp to be full height
        int height = std::min(1., squeak_fourier[i].amplitude * 10) * 400;
        SDL_FRect rect{float(i * rect_w), float(400 - height), float(rect_w), float(height)};
        SDL_RenderDrawRectF(renderer, &rect);
    }

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 25, 0, 25, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, freq_texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    std::sort(squeak_fourier.begin(), squeak_fourier.end(),
        [](const wave_data &a, const wave_data &b){
            return a.amplitude > b.amplitude;
        });
    std::cout << "Top 30 waves:\n";
    print_wave_data(std::vector(squeak_fourier.begin(), squeak_fourier.begin() + 30));
    std::cout << "Out of " << squeak_fourier.size() << " waves.\n";

    std::cout << "Building reconstructions..." << std::endl;
    auto squeak_ms_recon = generate_monosignal(squeak_fourier, squeak_ms.duration() * 2);

    std::cout << "Original:\n" << std::flush;
    play_monosig(squeak_ms);

    std::cout << "Reconstructed:\n" << std::flush;
    play_monosig(squeak_ms_recon);

    while (true) {
        std::cout << "Enter sample count, enter nothing to quit: ";
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            break;
        } else {
            try {
                int count = std::stoi(line);
                if (count == -1) {
                    std::cout << "Playing with all..." << std::endl;
                    play_monosig(squeak_ms_recon);
                } else if (count < 1 || std::size_t(count) > squeak_fourier.size()) {
                    std::cout << "Invalid number of waves.\n";
                } else {
                    std::cout << "Playing with " << count << " highest..." << std::endl;
                    auto squeak_ms_reconn = generate_monosignal(std::vector(squeak_fourier.begin(), squeak_fourier.begin() + count), squeak_ms.duration() * 2);
                    play_monosig(squeak_ms_reconn);
                }
            } catch (const std::invalid_argument &e) {
                std::cout << "Err: " << e.what() << '\n';
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}