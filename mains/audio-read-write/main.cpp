#include <filesystem>
#include <iostream>

#include "audio/wav.hpp"
#include "audio/monosignal.hpp"

namespace fs = std::filesystem;

static void play_wav_to_completion(const audio::wav_signal &w) {
    auto ms = w.to_monosignal();
    auto ab = ms.make_audio_buffer();
    audio::sound_obj sound(audio::global_engine, ab);

    ma_sound_start(&sound);

    while (ma_sound_is_playing(&sound));
}

int main() {
    fs::path file("data/squeak.wav");
    fs::path file2("data/gen/squeak2.wav");
    fs::path file3("data/gen/squeak_mono.wav");

    auto wav = audio::read_wav_from_file(file);
    audio::write_to_file(file2, wav);
    audio::write_to_file(file3, audio::wav_signal(wav.to_monosignal()));
    
    std::cout << "read 2 ---------------\n";
    auto wav2 = audio::read_wav_from_file(file2);
    play_wav_to_completion(wav2);

    std::cout << "read 3 ---------------\n";
    auto wav3 = audio::read_wav_from_file(file3);
    play_wav_to_completion(wav3);

    return 0;
}