#include <filesystem>
#include <iostream>

#include "audio.hpp"

namespace fs = std::filesystem;

int main() {
    fs::path file("data/squeak.wav");
    fs::path file2("data/gen/squeak2.wav");
    fs::path file3("data/gen/squeak_mono.wav");

    auto wav = audio::read_wav_from_file(file);
    audio::write_to_file(file2, wav);
    audio::write_to_file(file3, audio::wav_signal(wav.to_monosignal()));
    
    std::cout << "read 2 ---------------\n";
    auto wav2 = audio::read_wav_from_file(file2);
    wav2.to_monosignal().play();

    std::cout << "read 3 ---------------\n";
    auto wav3 = audio::read_wav_from_file(file3);
    wav3.to_monosignal().play();

    return 0;
}