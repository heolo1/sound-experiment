#include <cmath>
#include <iostream>
#include <mutex>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

void callback(ma_device *, void *output, const void *, ma_uint32 frame_count) {
    std::vector<state> states_lock;
    {
        std::lock_guard lock(state_mutex);
        states_lock = states;
    }

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

int main() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate        = sample_rate;
    config.dataCallback      = callback;

    audio::device device(&config);
    ma_device_start(&device);

    {
        std::lock_guard lock(state_mutex);
        states = {
            {500,  0.2 },
            {1000, 0.1 },
            {2000, 0.05},
        };
    }
    
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
                        std::lock_guard lock(state_mutex);
                        states.push_back(state);
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
                } catch (const std::invalid_argument &e) {
                    std::cout << "Err: " << e.what() << '\n';
                }
            }
        } else if (words[0] == "pop") {
            std::lock_guard lock(state_mutex);
            if (!states.empty()) {
                states.pop_back();
            } else {
                std::cout << "Empty.";
            }
        } else if (words[0] == "clear") {
            std::lock_guard lock(state_mutex);
            if (!states.empty()) {
                states.clear();
            } else {
                std::cout << "Empty.";
            }
        } else if (words[0] == "list") {
            std::cout << "Waves:\n";
            std::lock_guard lock(state_mutex);
            for (const auto &state : states) {
                std::cout << state.freq << " Hz @ " << state.amp << '\n';
            }
        } else if (words[0] == "quit") {
            std::cout << "Quitting...\n";
            break;
        } else {
            std::cout << "Unrecognized command: '" << line << "'\n";
        }
    }
}