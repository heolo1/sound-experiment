#include "audio/base.hpp"

#include <stdexcept>

#include "miniaudio.h"

namespace audio {

engine::engine(const ma_engine_config *engine_config) : init_result_{MA_ERROR} {
    if ((init_result_ = ma_engine_init(engine_config, &engine_))) {
        throw std::runtime_error("engine init failure");
    }
}

engine::~engine() {
    if (ok()) {
        ma_engine_uninit(&engine_);
    }
}

engine global_engine(nullptr);

audio_buffer::audio_buffer(const ma_audio_buffer_config *buffer_config) : init_result_{MA_ERROR} {
    if ((init_result_ = ma_audio_buffer_init(buffer_config, &audio_buffer_))) {
        throw std::runtime_error("audio_buffer init failure");
    }
}

audio_buffer::~audio_buffer() {
    if (ok()) {
        ma_audio_buffer_uninit(&audio_buffer_);
    }
}

sound_obj::sound_obj(engine &engine, audio_buffer &audio_buffer) : init_result_{MA_ERROR} {
    if ((init_result_ = ma_sound_init_from_data_source(&engine, &audio_buffer, 0, nullptr, &sound_))) {
        throw std::runtime_error("sound init failure");
    }
}

sound_obj::~sound_obj() {
    if (ok()) {
        ma_sound_uninit(&sound_);
    }
}

device::device(const ma_device_config *device_config) : init_result_{MA_ERROR} {
    if ((init_result_ = ma_device_init(nullptr, device_config, &device_))) {
        throw std::runtime_error("device init failure");
    }
    ma_device_start(&device_);
}

device::~device() {
    if (ok()) {
        if (ma_device_is_started(&device_)) {
            ma_device_stop(&device_);
        }
        ma_device_uninit(&device_);
    }
}

}