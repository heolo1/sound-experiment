#include "audio/monosignal.hpp"

#include <iostream>

#include "miniaudio.h"

#include "audio/base.hpp"

namespace audio {

audio_buffer monosignal::make_audio_buffer() const {
    ma_audio_buffer_config buffer_config = ma_audio_buffer_config_init(
        ma_format_f32,
        1,
        data.size(),
        reinterpret_cast<const void *>(data.data()),
        nullptr
    );

    // need to do this manually for some funny reason, apparently will be fixed in miniaudio.h v0.12
    buffer_config.sampleRate = samples_per_sec;

    return audio_buffer(&buffer_config);
}

}