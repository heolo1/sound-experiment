#include <iostream>
#include <vector>

#include "audio/base.hpp"
#include "audio/sig_gen.hpp"

int main() {
    std::vector<audio::sig_freq> freqs{{750, 0.115, 0}, {937, 0.09, 0}};
    auto ms = audio::generate_monosignal(freqs);
    auto ab = ms.make_audio_buffer();
    audio::sound_obj sound(audio::global_engine, ab);

    ma_sound_set_looping(&sound, true);
    ma_sound_start(&sound);

    std::cout << "Press enter to stop...";
    std::getchar();
    return 0;
}