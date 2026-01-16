#include <iostream>

#include "miniaudio/miniaudio.h"
#include <SDL2/SDL.h>

#include "audio.hpp"

int main() {
    // we hate error handling here
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window     = SDL_CreateWindow("Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 400, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    auto wav_sig = audio::read_wav_from_file("data/squeak.wav");
    auto monosig = wav_sig.to_monosignal();

    // actually drawing the wavform
    int width = 32768;
    SDL_Texture *wave_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_TARGET, width, 256);
    SDL_SetTextureBlendMode(wave_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, wave_texture);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (std::size_t i = 0; i < monosig.data.size(); i++) {
        int height = std::abs(monosig.data[i]) * 128;
        auto x = i / (float)monosig.data.size() * width;
        SDL_RenderDrawLineF(renderer, x, 128 - height, x, 127 + height);
    }
    
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect dest_rect{0, 72, 800, 256};
    SDL_RenderCopy(renderer, wave_texture, nullptr, &dest_rect);
    SDL_RenderPresent(renderer);

    monosig.play();
    std::cout << "Press enter to continue...";
    std::getchar();

    SDL_DestroyTexture(wave_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}