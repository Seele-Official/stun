#pragma once
#include <type_traits>


template <typename T>
concept integral = std::is_integral_v<T>;

#if defined(_WIN32) || defined(_WIN64)


#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "Bcrypt.lib")


template <integral T>
T random(T min, T max){
    static BCRYPT_ALG_HANDLE hProvider;
    BCryptOpenAlgorithmProvider(&hProvider, BCRYPT_RNG_ALGORITHM, nullptr, 0);
    T num;
    BCryptGenRandom(hProvider, reinterpret_cast<PUCHAR>(&num), sizeof(T), 0);
    BCryptCloseAlgorithmProvider(hProvider, 0);
    return num % (max - min) + min;
}




#elif defined(__linux__)


#include <sys/random.h>
template <integral T>
T random(T min, T max){
    T num;
    getrandom(&num, sizeof(T), 0);
    return num % (max - min) + min;
}




#endif
