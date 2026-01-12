#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "miniaudio/miniaudio.h"

constexpr int wav_OK = 0;
constexpr int wav_BAD_ID = 1;
constexpr int wav_UNIMPLEMENTED = 2;

const std::string test_file_name = "data/squeak.wav";

struct wave_file_format {
    uint32_t chunkLength; // Chunk size: 16, 18 or 40
    uint16_t formatTag; // Format code
    uint16_t channels; // Number of interleaved channels
    uint32_t samplesPerSec; // Sampling rate (blocks per second)
    uint32_t avgBytesPerSec; // Data rate
    uint16_t blockAlign; // Data block size (bytes)
    uint16_t bitsPerSample; // Bits per sample
};

struct wave_file {
    uint32_t fileLength;
    wave_file_format fmt;
    std::vector<uint8_t> data;
};

std::ostream &operator<<(std::ostream &os, const wave_file_format &f) {
    return os
        << "chunkLength=" << f.chunkLength << ','
        << "formatTag=" << std::format("0x{:04X}", f.formatTag) << ','
        << "channels=" << f.channels << ','
        << "samplesPerSec=" << f.samplesPerSec << ','
        << "avgBytesPerSec=" << f.avgBytesPerSec << ','
        << "blockAlign=" << f.blockAlign << ','
        << "bitsPerSample=" << f.bitsPerSample;
}

std::ostream &operator<<(std::ostream &os, const wave_file &w) {
    return os << "WAV[" << w.fileLength << ',' << w.data.size() << "]{" << w.fmt << "}";
}

static std::string read_string(std::istream &is, std::size_t n) {
    std::string read(n, '\0');
    is.read(read.data(), std::streamsize(n));
    return read;
}

static bool read_set_string(std::istream &is, const char *str) {
    return read_string(is, std::strlen(str)) == str;
}

template <typename T>
static T &read_num(std::istream &is, T &num) {
    is.read(reinterpret_cast<char *>(&num), sizeof(T));
    return num;
}

template <typename T>
static T read_num(std::istream &is) {
    T num;
    read_num(is, num);
    return num;
}

int read_wav_file_format(std::istream &is, wave_file_format &f, std::uint32_t size) {
    f.chunkLength = size;
    read_num(is, f.formatTag);
    read_num(is, f.channels);
    read_num(is, f.samplesPerSec);
    read_num(is, f.avgBytesPerSec);
    read_num(is, f.blockAlign);
    read_num(is, f.bitsPerSample);

    if (f.chunkLength != 16 || f.formatTag != 0x0001) { // files not in PCM format
        return wav_UNIMPLEMENTED;
    }

    return wav_OK;
}

int read_wav_file(std::istream &is, wave_file &w) {
    w = {};

    // read header

    if (!read_set_string(is, "RIFF")) {
        return wav_BAD_ID;
    }

    read_num(is, w.fileLength);

    if (!read_set_string(is, "WAVE")) {
        return wav_BAD_ID;
    }

    while (is.peek(), !is.eof()) {
        std::string chunk_name = read_string(is, 4);
        uint32_t chunk_size = read_num(is, chunk_size);
        std::cerr << "Found chunk " << chunk_name << " size " << chunk_size << '\n';
        if (chunk_name == "fmt ") {
            if (int ret = read_wav_file_format(is, w.fmt, chunk_size)) {
                return ret;
            }
        } else if (chunk_name == "LIST") {
            // https://www.recordingblogs.com/wiki/list-chunk-of-a-wave-file
            std::string list_type = read_string(is, 4);
            chunk_size -= 4;
            std::cerr << "List type is: " << list_type << '\n';

            if (list_type == "INFO") {
                while (chunk_size >= 8) {
                    std::string tag_name = read_string(is, 4);
                    uint32_t tag_size = read_num(is, tag_size);
                    chunk_size -= 8 + tag_size; // 8 is for tag_name and the storage of tag_size itself

                    std::string tag = read_string(is, tag_size);
                    if (!tag[tag_size - 1]) { // check if it's already formatted as null-terminated
                        tag.resize(tag_size - 1);
                    }

                    // text is required to be word (2 bytes) aligned, so we have to ignore an extra byte
                    // if tag_size is odd
                    if (tag_size % 2) {
                        chunk_size--;
                        is.ignore();
                    }

                    std::cerr << "\tFound tag " << tag_name << ": '" << tag << "'\n";
                }
            } else {
                std::cout << "Unsupported list type\n";
            }
            is.ignore(chunk_size);
        } else if (chunk_name == "data") {
            w.data.resize(chunk_size);
            is.read(reinterpret_cast<char *>(w.data.data()), std::streamsize(chunk_size));
            if (chunk_size % 2) {
                is.ignore(); // ignore padding byte in the case that chunk_size is odd
            }
        } else {
            std::cout << "Unsupported\n";
            is.ignore(chunk_size);
        }
    }

    return wav_OK;
}

ma_format as_ma_format(uint16_t bitsPerSample) {
    switch (bitsPerSample) {
        case 8:  return ma_format_u8;
        case 16: return ma_format_s16;
        case 24: return ma_format_s24;
        case 32: return ma_format_s32;
        default: throw std::runtime_error("Unsupported PCM bit depth.");
    }
}

int main() {
    std::ifstream file(test_file_name, std::ios_base::binary);

    if (!file) {
        std::cerr << "Could not open file '" << test_file_name << "'.\n";
        std::exit(1);
    } else {
        std::cerr << "File '" << test_file_name << "' opened successfully.\n";
    }

    wave_file w;
    if (int res = read_wav_file(file, w)) {
        std::cerr << "Errno " << res << "while reading\n";
        std::exit(res);
    }
    std::cout << w << '\n';

    // audio code (no clue whats happening here)
    ma_engine engine;
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
        std::cerr << "ma_engine_init failure\n";
        std::exit(1);
    }

    ma_audio_buffer_config buffer_config = ma_audio_buffer_config_init(
        as_ma_format(w.fmt.bitsPerSample),
        w.fmt.channels,
        w.data.size() / w.fmt.blockAlign,
        reinterpret_cast<void *>(w.data.data()),
        nullptr
    );

    buffer_config.sampleRate = w.fmt.samplesPerSec * 2;

    ma_audio_buffer audio_buffer;
    if (ma_audio_buffer_init(&buffer_config, &audio_buffer) != MA_SUCCESS) {
        std::cerr << "ma_engine_init failure\n";
        ma_engine_uninit(&engine);
        std::exit(1);
    }

    // audio_buffer.ref.sampleRate = w.fmt.samplesPerSec;

    ma_sound sound;
    if (ma_sound_init_from_data_source(&engine, &audio_buffer, 0, nullptr, &sound) != MA_SUCCESS) {
        std::cerr << "ma_sound_init_from_data_source failure\n";
        ma_audio_buffer_uninit(&audio_buffer);
        ma_engine_uninit(&engine);
        std::exit(1);
    }

    ma_sound_set_looping(&sound, true);
    ma_sound_start(&sound);

    std::cout << "Waiting for input...\n";
    std::getchar();

    ma_sound_uninit(&sound);
    ma_audio_buffer_uninit(&audio_buffer);
    ma_engine_uninit(&engine);

    return 0;
}