#include <filesystem>
#include <iostream>

#include "audio/wav.hpp"
#include "audio/monosignal.hpp"

const std::string test_file_name = "data/squeak.wav";


namespace fs = std::filesystem;

int main() {
    fs::path test_file(test_file_name);

    if (fs::exists(test_file) && fs::is_regular_file(test_file)) {
        std::cerr << "File '" << test_file_name << "' opened successfully.\n";
    } else {
        std::cerr << "Could not open file '" << test_file_name << "'.\n";
        std::exit(1);
    }

    auto wav = audio::read_wav_from_file(test_file);
    auto ms = wav.to_monosignal();
    auto audio_buffer = ms.make_audio_buffer();
    audio::sound_obj sound(audio::global_engine, audio_buffer);

    ma_sound_set_looping(&sound, true);
    ma_sound_start(&sound);

    std::cout << "Waiting for input...\n";
    std::getchar();

    return 0;
}