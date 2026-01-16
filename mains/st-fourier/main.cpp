#include <algorithm>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <SDL2/SDL.h>

#include "audio.hpp"

int width = 1600;
int height = 900;

template <typename T>
void display_stft(const audio::stft_result<T> &result, SDL_Renderer *renderer, SDL_Texture *texture,
    auto min_freq, auto max_freq, auto start_dur, auto end_dur) {

    auto view_time_range = end_dur - start_dur;
    auto view_freq_range = max_freq - min_freq;
    auto data_w = static_cast<T>(width) * result.time_delta / view_time_range;
    auto data_h = static_cast<T>(height) * result.freq_delta / view_freq_range;

    SDL_SetRenderTarget(renderer, texture);

    for (uint32_t s = start_dur / result.time_delta; s < std::ceil(end_dur / result.time_delta); s++) {
        if (s * result.n_freq >= result.waves.size()) { // quick fix for something wrong with my math
            break;
        }
        for (uint32_t f = min_freq / result.freq_delta; f < std::ceil(max_freq / result.freq_delta); f++) {
            const auto &wave = result.waves[s * result.n_freq + f];
            // generally expect max amplitudes of 0.16 for regular signals (number i made up)
            int col = std::min<T>(wave.amplitude * 6 * 255, 255);
            SDL_SetRenderDrawColor(renderer, col, col, col, 255);
            SDL_FRect rect{
                float((s - start_dur / result.time_delta) * data_w),
                float(height - (f - min_freq / result.freq_delta + 1) * data_h),
                float(data_w), float(data_h)};
            SDL_RenderFillRectF(renderer, &rect);
        }
    }

    std::cout << "Renderered STFT result w/ " << min_freq << " - " << max_freq << " Hz freq range & "
        << start_dur << " - " << end_dur << " s time range\n"; 

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 25, 0, 25, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

std::vector<std::string> split_string(const std::string &str) {
    std::vector<std::string> out;
    std::stringstream stream(str);
    std::string word;
    while (stream >> word) {
        out.push_back(word);
    }
    return out;
}

int main() {
    using namespace audio;

    SDL_Init(SDL_INIT_EVERYTHING);

    auto wav = read_wav_from_file("data/piano.wav");
    auto ms = wav.to_monosignal();

    std::cout << "Computing STFT...\n";
    uint32_t window_offset = std::sqrt(ms.data.size()); // arbitrary size
    uint32_t window_size = ms.samples_per_sec;

    ms.data.insert(ms.data.end(), window_size, 0); // pad the back so we don't accidentally chop off a bit

    auto start = std::chrono::steady_clock::now();
    auto stft = naive_stft_gpu(ms.data.size(), ms.data.data(), 1, ms.samples_per_sec,
        window_offset, false, window_size);
    auto end = std::chrono::steady_clock::now();
    
    std::cout << "STFT computed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << '\n';

    SDL_Window   *window   = SDL_CreateWindow("STFT Results", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET, width, height);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    double min_freq, max_freq, start_dur, end_dur;
    
    std::cout << "Displaying STFT result...\n";
    display_stft(stft, renderer, texture,
        min_freq = 0, max_freq = stft.freq_range,
        start_dur = 0, end_dur = stft.time_range);

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        auto words = split_string(line);
        if (words.empty()) {
            
        } else if (words[0] == "curr") {
            std::cout << "Freq Range: " << min_freq << " Hz - " << max_freq << " Hz\n";
            std::cout << "Time Range: " << start_dur << " s - " << end_dur << " s\n";
            std::cout << "Original Freq Range: " << 0 << " Hz - " << stft.freq_range << " Hz\n";
            std::cout << "Original Time Range: " << 0 << " s - " << stft.time_range << " s\n";
        } else if (words[0] == "freq") {
            if (words.size() != 3) {
                std::cout << "Invalid arguments to freq\n";
            } else {
                try {
                    auto new_min_freq = std::stod(words[1]);
                    auto new_max_freq = std::stod(words[2]);

                    if (new_min_freq == -1) {
                        new_min_freq = 0;
                    } else if (new_min_freq == -2) {
                        new_min_freq = min_freq;
                    }

                    if (new_max_freq == -1) {
                        new_max_freq = stft.freq_range;
                    } else if (new_max_freq == -2) {
                        new_max_freq = max_freq;
                    }

                    if (new_min_freq < 0 || new_max_freq > stft.freq_range || new_min_freq >= new_max_freq) {
                        std::cout << "Invalid range, attempted to set to " << new_min_freq << " Hz - " << new_max_freq << " Hz\n";
                    } else {
                        display_stft(stft, renderer, texture,
                            min_freq = new_min_freq, max_freq = new_max_freq,
                            start_dur, end_dur);
                    }
                } catch (const std::invalid_argument &e) {
                    std::cout << "Err: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "time") {
            if (words.size() != 3) {
                std::cout << "Invalid arguments to time\n";
            } else {
                try {
                    auto new_min_dur = std::stod(words[1]);
                    auto new_max_dur = std::stod(words[2]);

                    if (new_min_dur == -1) {
                        new_min_dur = 0;
                    } else if (new_min_dur == -2) {
                        new_min_dur = start_dur;
                    }

                    if (new_max_dur == -1) {
                        new_max_dur = stft.time_range;
                    } else if (new_max_dur == -2) {
                        new_max_dur = end_dur;
                    }

                    if (new_min_dur < 0 || new_max_dur > stft.time_range || new_min_dur >= new_max_dur) {
                        std::cout << "Invalid range, attempted to set to " << new_min_dur << " s - " << new_max_dur << " s\n";
                    } else {
                        display_stft(stft, renderer, texture,
                            min_freq, max_freq,
                            start_dur = new_min_dur, end_dur = new_max_dur);
                    }
                } catch (const std::invalid_argument &e) {
                    std::cout << "Err: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "load") {
            if (words.size() != 2) {
                std::cout << "Invalid arguments to load\n";
            } else {
                std::filesystem::path path(words[1]);
                if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
                    std::cout << "Invalid file\n";
                    continue;
                }
                try {
                    auto wav = read_wav_from_file(path);
                    ms = wav.to_monosignal();
                    std::cout << "Computing STFT...\n";

                    ms.data.insert(ms.data.end(), window_size, 0); // pad the back so we don't accidentally chop off a bit

                    auto start = std::chrono::steady_clock::now();
                    stft = naive_stft_gpu(ms.data.size(), ms.data.data(), 1, ms.samples_per_sec,
                        window_offset, false, window_size);
                    auto end = std::chrono::steady_clock::now();
                    
                    std::cout << "STFT computed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << '\n';
                    display_stft(stft, renderer, texture,
                        min_freq = 0, max_freq = stft.freq_range,
                        start_dur = 0, end_dur = stft.time_range);
                } catch (const std::runtime_error &e) {
                    std::cout << "Err while reading file: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "refresh") {
            display_stft(stft, renderer, texture,
                min_freq, max_freq,
                start_dur, end_dur);
        } else if (words[0] == "quit") {
            std::cout << "Quitting...\n";
            break;
        } else {
            std::cout << "Unrecognized command: '" << line << "'\n";
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}