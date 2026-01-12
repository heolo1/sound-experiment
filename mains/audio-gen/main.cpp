#include <algorithm>
#include <cmath>
#include <iostream>
#include <mutex>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "miniaudio/miniaudio.h"

#include "audio.hpp"

struct state {
    state(double freq, double amp);

    void set_freq(double freq);

    double curr_phase;
    double freq;
    double delta_phase;
    double amp;
};

std::vector<state> states;
std::mutex state_mutex;
const uint32_t sample_rate = 44100;

double comp_dphase(double freq) {
    return 2 * std::numbers::pi * freq / sample_rate;
}

state::state(double freq, double amp) :
        curr_phase{0}, freq{freq}, delta_phase{comp_dphase(freq)}, amp{amp} {}

void state::set_freq(double freq) {
    this->freq = freq;
    this->delta_phase = comp_dphase(freq);
}

std::vector<state> get_states_copy() {
    std::lock_guard lock(state_mutex);
    return states;
}

void callback(ma_device *, void *output, const void *, ma_uint32 frame_count) {
    std::vector<state> states_lock = get_states_copy();
    float *f_output = reinterpret_cast<float *>(output);

    for (auto i = 0u; i < frame_count; i++) {
        for (const auto &state : states_lock) {
            f_output[i] += state.amp * std::cos(state.curr_phase + i * state.delta_phase);
        }
    }

    std::lock_guard lock(state_mutex);
    for (auto &state : states) {
        state.curr_phase += frame_count * state.delta_phase;
        state.curr_phase = std::fmod(state.curr_phase, 2 * std::numbers::pi);
    }
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

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *wave_texture;
constexpr int wt_width = 1600;
constexpr int wt_height = 800;

// s_win_size = window size in seconds, s_sep = time in between each line in sec
void update_texture(double s_win_size, double s_sep) {
    SDL_SetRenderTarget(renderer, wave_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    auto s_px_size = s_win_size / wt_width; // size of pixel in seconds
    auto p_sep_size = s_sep / s_px_size;
    // draw some second dividing lines
    SDL_SetRenderDrawColor(renderer, 15, 60, 128, 255);
    for (int t = 0; t * p_sep_size < wt_width; t++){
        SDL_RenderDrawLineF(renderer, t * p_sep_size, 0, t * p_sep_size, wt_height);
    }

    // draw some 
    std::vector<SDL_FPoint> points(wt_width);
    auto states_lock = get_states_copy();
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (std::size_t i = 0; i < wt_width; i++) {
        points[i].x = i;
        for (const auto &state : states_lock) {
            points[i].y += state.amp * std::cos(2 * std::numbers::pi * state.freq * s_px_size * i + state.curr_phase);
        }
        // remap [-1, 1] to [0, wt_height]
        points[i].y = std::clamp<float>((points[i].y + 1) * wt_height / 2, 0, wt_height - 0.5);
    }
    SDL_RenderDrawLinesF(renderer, points.data(), points.size());

    // now we actually have to draw the window
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 25, 0, 25, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, wave_texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Init(SDL_INIT_EVERYTHING);

    window       = SDL_CreateWindow("Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wt_width, wt_height, 0);
    renderer     = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    wave_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_TARGET, wt_width, wt_height);
    SDL_SetTextureBlendMode(wave_texture, SDL_BLENDMODE_BLEND);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate        = sample_rate;
    config.dataCallback      = callback;

    {
        std::lock_guard lock(state_mutex);
        states = {
            {500,  0.2 },
            {1000, 0.1 },
            {2000, 0.05},
        };
    }
    
    double s_display_size = 0.01; // display size in seconds
    double s_display_sept = 0.001; // display line dt in seconds

    update_texture(s_display_size, s_display_sept);

    audio::device device(&config);
    ma_device_start(&device);

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        auto words = split_string(line);
        if (words.empty()) {
            
        } else if (words[0] == "start") {
            if (ma_result ret = ma_device_start(&device)) {
                std::cout << "Start returned code: " << ret << '\n';
            }
        } else if (words[0] == "stop") {
            if (ma_result ret = ma_device_stop(&device)) {
                std::cout << "Stop returned code: " << ret << '\n';
            }
        } else if (words[0] == "push") {
            if (words.size() != 3) {
                std::cout << "Invalid arguments to push\n";
            } else {
                try {
                    state state(std::stod(words[1]), std::stod(words[2]));
                    if (std::abs(state.amp) > 1) {
                        std::cout << "amp (" << state.amp << ") must be > 0 & <= 1 \n";
                    } else if (std::abs(state.freq) < 1) {
                        std::cout << "freq (" << state.freq << ") must be >= 1\n";
                    } else {
                        {
                            std::lock_guard lock(state_mutex);
                            states.push_back(state);
                        }
                        update_texture(s_display_size, s_display_sept);
                    }
                } catch (const std::invalid_argument &e) {
                    std::cout << "Err: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "pushs") {
            if (words.size() < 4 || words.size() > 6) {
                std::cout << "Invalid arguments to push\n";
            } else {
                try {
                    auto n = std::stoi(words[1]);
                    auto freq = std::stod(words[2]),
                         amp = std::stod(words[3]);
                    double mfreq, mamp;

                    if (words.size() < 5) {
                        mfreq = 2;
                    } else {
                        mfreq = std::stod(words[4]);
                    }

                    if (words.size() < 6) {
                        mamp = 1 / mfreq;
                    } else {
                        mamp = std::stod(words[5]);
                    }

                    if (std::abs(freq) < 1) {
                        std::cout << "freq (" << freq << ") must be >= 1\n";
                        continue;
                    }

                    {
                        std::lock_guard lock(state_mutex);
                        for (int i = 0; i < n; i++) {
                            if (std::abs(amp) > 1) {
                                std::cout << "Skipping " << freq << " Hz @ " << amp << '\n';
                            } else {
                                std::cout << "Adding " << freq << " Hz @ " << amp << '\n';
                                states.emplace_back(freq, amp);
                            }
                            freq *= mfreq;
                            amp *= mamp;
                        }
                    }
                    update_texture(s_display_size, s_display_sept);
                } catch (const std::invalid_argument &e) {
                    std::cout << "Err: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "pop") {
            if (states.empty()) {
                std::cout << "Empty.";
                continue;
            }
            {
                std::lock_guard lock(state_mutex);
                states.pop_back();
            }
            update_texture(s_display_size, s_display_sept);
        } else if (words[0] == "clear") {
            if (states.empty()) {
                std::cout << "Empty.";
                continue;
            }
            {
                std::lock_guard lock(state_mutex);
                states.clear();
            }
            update_texture(s_display_size, s_display_sept);
        } else if (words[0] == "list") {
            std::cout << "Waves:\n";
            auto states_lock = get_states_copy();
            for (const auto &state : states_lock) {
                std::cout << state.freq << " Hz @ " << state.amp << '\n';
            }
        } else if (words[0] == "setsep") {
            if (words.size() != 2) {
                std::cout << "Command requires single argument of time in ms\n";
                continue;
            }
            try {
                auto s_display_sept_pot = std::stod(words[1]);
                if (s_display_sept_pot < 0.001) {
                    std::cout << "Separator too small, must be >= 0.001 ms\n";
                    continue;
                }
                s_display_sept = s_display_sept_pot / 1000;
                update_texture(s_display_size, s_display_sept);
            } catch (const std::invalid_argument &e) {
                std::cout << "Err: " << e.what() << '\n';
            }
        } else if (words[0] == "setsize") {
            if (words.size() != 2) {
                std::cout << "Command requires single argument of time in ms\n";
                continue;
            }
            try {
                auto s_display_size_pot = std::stod(words[1]);
                if (s_display_size_pot < 0.01) {
                    std::cout << "Display too small, must be >= 0.01 ms\n";
                    continue;
                }
                s_display_size = s_display_size_pot / 1000;
                update_texture(s_display_size, s_display_sept);
            } catch (const std::invalid_argument &e) {
                std::cout << "Err: " << e.what() << '\n';
            }
        } else if (words[0] == "curset") {
            std::cout << "Window size: " << s_display_size * 1000 << " ms\n";
            std::cout << "Separator size: " << s_display_sept * 1000 << " ms\n";
        } else if (words[0] == "refresh") {
            update_texture(s_display_size, s_display_sept);
        } else if (words[0] == "resetphase") {
            {
                std::lock_guard lock(state_mutex);
                for (auto &state : states) {
                    state.curr_phase = 0;
                }
            }
            update_texture(s_display_size, s_display_sept);
        } else if (words[0] == "quit") {
            std::cout << "Quitting...\n";
            break;
        } else {
            std::cout << "Unrecognized command: '" << line << "'\n";
        }
    }
}