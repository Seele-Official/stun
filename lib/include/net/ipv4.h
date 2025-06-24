#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <format>
#include <expected>
#include "math.h"

namespace seele::net{

    std::expected<uint32_t, std::string> inet_addr(std::string_view ip);

    std::string inet_ntoa(uint32_t addr);

    

    struct ipv4{
        uint32_t net_address;
        uint16_t net_port;
        explicit ipv4() : net_address{}, net_port{} {}
        explicit ipv4(uint32_t net_address, uint16_t net_port) : net_address{net_address}, net_port{net_port} {}
        auto operator<=>(const ipv4&) const = default;

        auto toString() const {
            return std::format("{}:{}", inet_ntoa(net_address), math::ntoh(net_port));
        }

    };
    
    std::expected<ipv4, std::string> parse_addr(std::string_view addr);

    inline uint16_t random_pri_iana_net_port() {
        return math::hton(math::random<uint16_t>(32768, 65535));
    }
}