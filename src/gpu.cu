#include "gpu.hpp"

#include <chrono>
#include <iostream>

namespace gpu {

__global__
static void empty() {

}

void wakeup() {
    std::cerr << "Waking GPU... ";
    auto start = std::chrono::system_clock::now();
    empty<<<1, 1>>>();
    auto end = std::chrono::system_clock::now();
    // TODO: FIND COMPILER BUG WITH STD::FORMAT!!!!!
    std::cerr << "Awake in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start) << std::endl;
}

}