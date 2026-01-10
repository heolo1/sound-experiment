#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "audio/monosignal.hpp"

namespace audio {

// assumes chunk size of 16
struct wav_signal_fmt {
    static constexpr uint16_t FORMAT_PCM        = 0x0001;
    static constexpr uint16_t FORMAT_IEEE_FLOAT = 0x0003;
    static constexpr uint16_t FORMAT_ALAW       = 0x0006;
    static constexpr uint16_t FORMAT_MULAW      = 0x0007;
    static constexpr uint16_t FORMAT_EXTENSIBLE = 0xFFFE;

    uint16_t format_tag; // Format code
    uint16_t channels; // Number of interleaved channels
    uint32_t samples_per_sec; // Sampling rate (blocks per second)
    uint32_t avg_bytes_per_sec; // Data rate
    uint16_t block_align; // Data block size (bytes)
    uint16_t bits_per_sample; // Bits per sample
};

struct wav_signal {
    wav_signal();
    explicit wav_signal(const monosignal &);

    monosignal to_monosignal() const;

    wav_signal_fmt fmt;
    std::vector<unsigned char> data;
};

std::istream &read_from_stream(std::istream &, wav_signal &);
wav_signal read_wav_from_stream(std::istream &);
std::istream &operator>>(std::istream &, wav_signal &);

void read_from_file(const std::filesystem::path &, wav_signal &);
wav_signal read_wav_from_file(const std::filesystem::path &);

std::ostream &write_to_stream(std::ostream &, const wav_signal &);
std::ostream &operator<<(std::ostream &, const wav_signal &);

void write_to_file(const std::filesystem::path &, const wav_signal &);

}