#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <format>
#include <expected>
#include <map>
#include "math.h"

namespace seele::net{



    constexpr std::expected<uint32_t, std::string> inet_addr(std::string_view ip) {
        uint32_t addr = 0;
        for (size_t i = 0; i < 3; i++) {
            auto pos = ip.find('.');
            if (pos == std::string_view::npos) {
                return std::unexpected{"missing '.'"};
            }
            auto num = math::stoi(ip.substr(0, pos));
            if (!num.has_value()) {
                return std::unexpected{std::format("unexpected char: {}", math::tohex(num.error()))};
            }
            addr = (addr << 8) | num.value();
            ip.remove_prefix(pos + 1);
        }
        auto num = math::stoi(ip);
        if (!num.has_value()) {
            return std::unexpected{std::format("unexpected char: {}", math::tohex(num.error()))};
        }
        addr = (addr << 8) | num.value();

        return math::hton(addr);
    }

    constexpr std::string inet_ntoa(uint32_t addr) {
        addr = math::ntoh(addr);
        std::string ip;
        for (size_t i = 0; i < 4; i++) {
            ip.insert(0, std::to_string(addr & 0xFF));
            if (i != 3) ip.insert(0, ".");
            addr >>= 8;
        }
        return ip;
    }

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

    enum class udpv4_error{
        BIND_ERROR,
        TIMEOUT_ERROR,
        RECVFROM_ERROR,
        SENDTO_ERROR
    };

    #if defined(_WIN32) || defined(_WIN64)
    using socket_t = uint64_t;
    #elif defined(__linux__)
    using socket_t = int;
    #endif


    class udpv4{
    private:
        socket_t socketfd;
    public:
        explicit udpv4();
        udpv4(const udpv4&) = delete;
        udpv4(udpv4&&);

        udpv4& operator=(const udpv4&) = delete;
        udpv4& operator=(udpv4&&);

        ~udpv4();

        bool bind(ipv4 info);
        bool set_timeout(uint32_t timeout);

        std::expected<size_t, udpv4_error> recvfrom(ipv4& src, void* buffer, size_t buffer_size);
        udpv4_error sendto(const ipv4& dest, const void* data, size_t size);

    };






    uint32_t query_device_ip(uint32_t interface_index);

    std::map<uint32_t, std::tuple<std::u8string, uint32_t>> query_all_device_ip();

    inline uint16_t random_pri_iana_net_port() {
        return math::hton(math::random<uint16_t>(32768, 65535));
    }
}