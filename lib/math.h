#pragma once
#include <algorithm>
#include <concepts>
#include <random>
#include <cstdint>
#include <array>
#include <expected>
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

    template<std::integral T>
    constexpr std::string itostr(T num) {
        if (num == 0) return "0";
        std::string str;
        bool is_negative = false;
        if (num < 0) {
            is_negative = true;
            num = -num;
        }
        while (num > 0) {
            str.push_back('0' + (num % 10));
            num /= 10;
        }
        if (is_negative) str.push_back('-');
        std::reverse(str.begin(), str.end());
        return str;
    } 
}

