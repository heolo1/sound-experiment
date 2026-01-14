#pragma once

namespace gpu {

inline constexpr int BLOCK_SIZE = 0x100;

// wakes up the GPU and times the wakeup duration
void wakeup();

}