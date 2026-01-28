#include "gpu.hpp"

#include <chrono>
#include <iostream>

namespace gpu {

__global__
static void empty() {

}

int wakeup() {
    std::cerr << "Waking GPU... ";
    auto start = std::chrono::system_clock::now();
    empty<<<1, 1>>>();
    auto end = std::chrono::system_clock::now();
    // TODO: FIND nvcc COMPILER BUG WITH STD::FORMAT!!!!!
    auto err = cudaGetLastError();
    if (err) {
        std::cerr << "Error while waking GPU: " << cudaGetErrorString(err) << '\n';
        std::cerr << "Please make sure the device code required by your GPU is made by the Makefile...\n";
    } else {
        std::cerr << "Awake in " << std::chrono::duration_cast<std::chrono::microseconds>(end - start) << std::endl;
    }
    return err;
}

}