#pragma once
#include <concepts>
#include <random>
#include <cstdint>
#include <array>
#include <expected>
#include <format>
namespace math {
    template <std::integral T>
    T random(T min, T max) {
        static std::random_device rd; 
        std::uniform_int_distribution<T> dist(min, max - 1); 
        return dist(rd);
    }

    template <std::integral T>
    constexpr T ntoh(T value) {
        if constexpr (std::endian::native == std::endian::big) {
            return value; 
        } else {
            return std::byteswap(value);
        }
    }
    template <std::integral T>
    constexpr T hton(T value) {
        if constexpr (std::endian::native == std::endian::big) {
            return value; 
        } else {
            return std::byteswap(value);
        }
    }

    constexpr std::array<uint32_t, 256> crc32_table() {
        constexpr uint32_t poly = 0xEDB88320; // 反射多项式
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
            }
            table[i] = crc;
        }
        return table;
    }

    constexpr uint32_t crc32_bitwise(const uint8_t* data, size_t len) {

        static constexpr auto CRC32_TABLE = crc32_table();

        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; ++i) {
            crc = (crc >> 8) ^ CRC32_TABLE[(crc ^ data[i]) & 0xFF];
        }
        return crc ^ 0xFFFFFFFF;
    }

    constexpr std::expected<uint32_t, char> stoi(std::string_view str){
        if (str.empty()) return std::unexpected{'\0'};
        uint32_t num = 0;
        for (auto c : str){
            if (c < '0' || c > '9') return std::unexpected{c};
            num = num * 10 + c - '0';
        }
        return num;
    }
    constexpr auto tohex(void* ptr, size_t size){
        std::string str{"0x"};
        for (size_t i = 0; i < size; i++){
            constexpr char e[] = "0123456789ABCDEF";
            str += std::format("{}{}", e[((uint8_t*)ptr)[i] >> 4], e[((uint8_t*)ptr)[i] & 0xF]);
        }
        return str;
    }

    template <typename T>
    constexpr auto tohex(T struct_t){
        return math::tohex(&struct_t, sizeof(T));
    }
}

