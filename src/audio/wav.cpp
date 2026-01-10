#include "audio/wav.hpp"

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/monosignal.hpp"

namespace audio {

wav_signal::wav_signal() {}

wav_signal::wav_signal(const monosignal &ms) : fmt{
        wav_signal_fmt::FORMAT_IEEE_FLOAT,  // format code
        1,                                  // channels (1 for single channel signal)
        ms.samples_per_sec,                 // samples/s
        ms.samples_per_sec * uint32_t(sizeof(float)), // avg bytes/sec = samples/s * sizeof(sample)
        sizeof(float),                      // sample block size = sizeof(sample) * channels
        sizeof(float) * 8                   // bit per sample = sizeof(sample) in bytes * 8 bits/byte
    }, data(reinterpret_cast<const unsigned char *>(ms.data.begin().base()),
            reinterpret_cast<const unsigned char *>(ms.data.end()  .base())) {}

static float get_next_from_pcm_ptr(const unsigned char *&ptr, std::size_t bits) {
    if (bits == 8) { // 8 bit 
        return (int(*ptr++) - 128) / 128.f;
    } else if (bits == 16) {
        const int16_t *ptr16 = reinterpret_cast<const int16_t *>(ptr);
        ptr += 2;
        return *ptr16 / float(0x7FFF); // max val of a signed 16 bit integer
    } else if (bits == 24) {
        // (ptr[2] >> 7) * 0xFF is a trick to maintain signedness
        int32_t num = (ptr[2] >> 7) * 0xFF000000 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
        ptr += 3;
        return num / float(0x7FFFFF); // max val of a signed 24 bit integer
    } else if (bits == 32) {
        const int32_t *ptr32 = reinterpret_cast<const int32_t *>(ptr);
        ptr += 4;
        return *ptr32 / float(0x7FFFFFFF); // max val of a signed 32 bit integer
    } else {
        return 0;
    }
}

monosignal wav_signal::to_monosignal() const {
    std::vector<float> monosignal_data(data.size() / fmt.block_align);

    if (fmt.format_tag == wav_signal_fmt::FORMAT_IEEE_FLOAT) {
        // just copy directly
        for (auto data_it = reinterpret_cast<const float *>(data.begin().base());
             auto &ms_data : monosignal_data) {
            for (std::size_t i = 0; i < fmt.channels; i++, data_it++) {
                ms_data += *data_it;
            }
            if (fmt.channels != 1) {
                ms_data /= fmt.channels;
            }
        }
    } else if (fmt.format_tag == wav_signal_fmt::FORMAT_PCM) {
        if (fmt.bits_per_sample == 8  ||
            fmt.bits_per_sample == 16 ||
            fmt.bits_per_sample == 24 ||
            fmt.bits_per_sample == 32) {
            for (auto data_it = data.begin().base();
                 auto &ms_data : monosignal_data) {
                for (std::size_t i = 0; i < fmt.channels; i++) {
                    ms_data += get_next_from_pcm_ptr(data_it, fmt.bits_per_sample);
                }
                if (fmt.channels != 1) {
                    ms_data /= fmt.channels;
                }
            }
        } else {
            throw std::runtime_error(std::format("unsupported bits/sample {}", fmt.bits_per_sample));
        }
    } else { // need to support ALAW, MULAW, EXTENSIBLE
        throw std::runtime_error(std::format("unsupported format tag 0x{:04X}", fmt.format_tag));
    }

    return {
        fmt.samples_per_sec,    // samples_per_sec
        monosignal_data
    };
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

static std::ostream &write_num_unformatted(std::ostream &os, const auto &num) {
    os.write(reinterpret_cast<const char *>(&num), sizeof(num));
    return os;
}

// helper struct for writing unformatted data
template <typename T>
struct UF {
    const T &t;
};

template <typename T>
static std::ostream &operator<<(std::ostream &os, UF<T> &&uf) {
    return os.write(reinterpret_cast<const char *>(&uf.t), sizeof(T));
}

std::istream &read_from_stream(std::istream &is, wav_signal &w) {
    w = {};

    if (!read_set_string(is, "RIFF")) {
        throw std::runtime_error("did not find expected byte signature RIFF");
    }

    read_num<uint32_t>(is); // file_length

    if (!read_set_string(is, "WAVE")) {
        throw std::runtime_error("did not find expected byte signature WAVE");
    }

    while (is.peek(), !is.eof()) {
        std::string chunk_name = read_string(is, 4);
        uint32_t chunk_size = read_num(is, chunk_size);
        std::cerr << "Found chunk " << chunk_name << " size " << chunk_size << '\n';
        if (chunk_name == "fmt ") {
            read_num(is, w.fmt.format_tag);
            read_num(is, w.fmt.channels);
            read_num(is, w.fmt.samples_per_sec);
            read_num(is, w.fmt.avg_bytes_per_sec);
            read_num(is, w.fmt.block_align);
            read_num(is, w.fmt.bits_per_sample);

            if (chunk_size != 16 || (w.fmt.format_tag != 0x0001 && w.fmt.format_tag != 0x0003)) {
                // files not in PCM format or IEEE format
                throw std::runtime_error(std::format("cannot support format tag 0x{:04X}, unimplemented", w.fmt.format_tag));
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
        } else {
            std::cout << "Unsupported\n";
            is.ignore(chunk_size);
        }
    }

    return is;
}

wav_signal read_wav_from_stream(std::istream &is) {
    wav_signal w;
    read_from_stream(is, w);
    return w;
}

std::istream &operator>>(std::istream &is, wav_signal &w) {
    return read_from_stream(is, w);
}

void read_from_file(const std::filesystem::path &path, wav_signal &w) {
    std::ifstream(path, std::ios::binary) >> w;
}

wav_signal read_wav_from_file(const std::filesystem::path &path) {
    wav_signal w;
    read_from_file(path, w);
    return w;
}

std::ostream &write_to_stream(std::ostream &os, const wav_signal &w) {
    uint32_t fmt_chunk_size = sizeof(wav_signal_fmt);
    uint32_t data_chunk_size = w.data.size();
    uint32_t file_size = 4 /* for WAVE */
        + (8 + fmt_chunk_size) // 8 is for the chunk "header"
        + (8 + data_chunk_size + data_chunk_size % 2); // we add a padding byte if data_chunk is odd

    os // header
        << "RIFF" 
        << UF{file_size}
        << "WAVE";
    
    os // format chunk, basically anyways
        << "fmt "
        << UF{fmt_chunk_size}
        << UF{w.fmt}; // this splats everything in order
    
    os // data chunk
        << "data"
        << UF{data_chunk_size};
    os.write(reinterpret_cast<const char *>(w.data.data()), w.data.size());
    if (w.data.size() % 2) {
        os.put(0); // padding byte should be null
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const wav_signal &w) {
    return write_to_stream(os, w);
}

void write_to_file(const std::filesystem::path &path, const wav_signal &w) {
    std::ofstream(path, std::ios::binary) << w;
}

}