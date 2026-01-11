#pragma once

#include <vector>

#include "monosignal.hpp"

namespace audio {

struct sig_freq {
    double freq; // in rev/sec
    double amplitude;
    double phase; // in radians
};

/**
 * takes in a vector of cosine wave data and converts it to a monosignal 
 * if normalize is true, then all values will be divided by the maximum absolute value in the case that it is greater than 1, or if always_normalize is true.
 */
monosignal generate_monosignal(const std::vector<sig_freq> &waves, double time = 1, uint32_t samples_per_sec = 44100,
    bool normalize = false, bool always_normalize = false);

}