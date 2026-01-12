#pragma once

#include <vector>

namespace audio {

template <typename T>
struct wave_data_t {
    double freq; // in rev/sec
    double amplitude;
    double phase; // in radians
};

using wave_data = wave_data_t<double>;
using wave_dataf = wave_data_t<float>;

}