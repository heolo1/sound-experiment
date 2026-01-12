#pragma once

#include "miniaudio.h"

namespace audio {

struct engine {
    engine(const ma_engine_config * = nullptr);
    ~engine();
    engine(engine &&) = default;
    engine &operator=(engine &&) = default;
    engine(const engine &) = delete;
    engine &operator=(const engine &) = delete;

    ma_engine *operator&() {
        return &engine_;
    }

    const ma_engine *operator&() const {
        return &engine_;
    }

    bool ok() const {
        return init_result_ == MA_SUCCESS;
    }

    operator bool() const {
        return ok();
    }

    bool operator!() const{
        return !ok();
    }

    ma_result init_result() const {
        return init_result_;
    }

private:
    ma_engine engine_;
    ma_result init_result_;
};

extern engine global_engine;

struct audio_buffer {
    audio_buffer(const ma_audio_buffer_config *);
    ~audio_buffer();
    audio_buffer(audio_buffer &&) = default;
    audio_buffer &operator=(audio_buffer &&) = default;
    audio_buffer(const audio_buffer &) = delete;
    audio_buffer &operator=(const audio_buffer &) = delete;

    ma_audio_buffer *operator&() {
        return &audio_buffer_;
    }

    const ma_audio_buffer *operator&() const {
        return &audio_buffer_;
    }

    bool ok() const {
        return init_result_ == MA_SUCCESS;
    }

    operator bool() const {
        return ok();
    }

    bool operator!() const{
        return !ok();
    }

    ma_result init_result() const {
        return init_result_;
    }

private:
    ma_audio_buffer audio_buffer_;
    ma_result init_result_;
};

struct sound_obj {
    sound_obj(engine &, audio_buffer &);
    ~sound_obj();
    sound_obj(sound_obj &&) = default;
    sound_obj &operator=(sound_obj &&) = default;
    sound_obj(const sound_obj &) = delete;
    sound_obj &operator=(const sound_obj &) = delete;

    ma_sound *operator&() {
        return &sound_;
    }

    const ma_sound *operator&() const {
        return &sound_;
    }

    bool ok() const {
        return init_result_ == MA_SUCCESS;
    }

    operator bool() const {
        return ok();
    }

    bool operator!() const{
        return !ok();
    }

    ma_result init_result() const {
        return init_result_;
    }

private:
    ma_sound sound_;
    ma_result init_result_;
};

struct device {
    device(const ma_device_config *);
    ~device();
    device(device &&) = default;
    device &operator=(device &&) = default;
    device(const device &) = delete;
    device &operator=(const device &) = delete;

    ma_device *operator&() {
        return &device_;
    }

    const ma_device *operator&() const {
        return &device_;
    }

    bool ok() const {
        return init_result_ == MA_SUCCESS;
    }

    operator bool() const {
        return ok();
    }

    bool operator!() const{
        return !ok();
    }

    ma_result init_result() const {
        return init_result_;
    }

private:
    ma_device device_;
    ma_result init_result_;
};

}