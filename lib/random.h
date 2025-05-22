#pragma once
#include <concepts>
#include <random>

template <std::integral T>
T random(T min, T max) {
    static std::random_device rd; 
    std::uniform_int_distribution<T> dist(min, max - 1); 
    return dist(rd);
}
