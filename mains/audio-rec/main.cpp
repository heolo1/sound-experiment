#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>

#include <SDL2/SDL.h>

#include "miniaudio/miniaudio.h"

#include "audio.hpp"

#include "markable_interval.hpp"

std::vector<float> samples;
std::mutex sample_mutex;
const uint32_t sample_rate = 44100;
const uint32_t sample_buf_size = sample_rate * 2;
uint32_t sample_pos = 0;

void callback(ma_device *, void *, const void *input, ma_uint32 frame_count) {
    const float *finput = static_cast<const float *>(input);
    std::lock_guard guard(sample_mutex);
    if (sample_pos + frame_count >= sample_buf_size) {
        std::copy(finput, finput + (sample_buf_size - sample_pos), &samples[sample_pos]);
        sample_pos += frame_count;
        sample_pos -= sample_buf_size;
        std::copy(finput + (sample_buf_size - sample_pos), finput + frame_count, samples.begin());
    } else {
        std::copy(finput, finput + frame_count, &samples[sample_pos]);
        sample_pos += frame_count;
    }
}

int main() {
    samples.resize(sample_buf_size);

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate        = sample_rate;
    config.dataCallback      = callback;

    audio::device device(&config);
    if (auto err = ma_device_start(&device)) {
        std::cerr << "Err code " << err << '\n';
        return 1;
    }

    std::cout << "Reading, press enter to quit...";
    std::getchar();
}