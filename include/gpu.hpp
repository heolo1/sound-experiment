#pragma once

namespace gpu {

inline constexpr int BLOCK_SIZE    = 0x100;
inline constexpr int BLOCK_SIZE_2D = 0x10;

// wakes up the GPU and times the wakeup duration
void wakeup();

}