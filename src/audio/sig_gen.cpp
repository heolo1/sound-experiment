#include "audio/sig_gen.hpp"

#include <cmath>
#include <numbers>
#include <vector>

namespace audio {

monosignal generate_monosignal(const std::vector<sig_freq> &waves, double time, uint32_t samples_per_sec,
    bool normalize, bool always_normalize) {

    monosignal sig{samples_per_sec, std::vector(std::size_t(time * samples_per_sec), 0.f)};

    auto time_step = 1. / samples_per_sec;
    for (std::size_t i = 0; i < sig.data.size(); i++) {
        for (const auto &w : waves) {
            sig.data[i] += w.amplitude * std::cos(2 * std::numbers::pi * w.freq * i * time_step + w.phase);
        }
    }

    if (!normalize) {
        return sig;
    }

    float max = 0;
    for (const auto &sample : sig.data) {
        if (std::abs(sample) > max) {
            max = std::abs(sample);
        }
    }

    if (always_normalize || max > 1) {
        for (auto &sample : sig.data) {
            sample /= max;
        }
    }

    return sig;
}

}