#pragma once
#include <concepts>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <format>
#include <expected>
#include <map>
#include <bit>
#include "math.h"
#include "log.h"



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
    return tohex(&struct_t, sizeof(T));
}


constexpr std::expected<uint32_t, std::string> my_inet_addr(std::string_view ip) {
    uint32_t addr = 0;
    for (size_t i = 0; i < 3; i++) {
        auto pos = ip.find('.');
        if (pos == std::string_view::npos) {
            return std::unexpected{"missing '.'"};
        }
        auto num = math::stoi(ip.substr(0, pos));
        if (!num.has_value()) {
            return std::unexpected{std::format("unexpected char: {}", tohex(num.error()))};
        }
        addr = (addr << 8) | num.value();
        ip.remove_prefix(pos + 1);
    }
    auto num = math::stoi(ip);
    if (!num.has_value()) {
        return std::unexpected{std::format("unexpected char: {}", tohex(num.error()))};
    }
    addr = (addr << 8) | num.value();

    return math::hton(addr);
}

constexpr std::string my_inet_ntoa(uint32_t addr) {
    addr = math::ntoh(addr);
    std::string ip;
    for (size_t i = 0; i < 4; i++) {
        ip.insert(0, std::to_string(addr & 0xFF));
        if (i != 3) ip.insert(0, ".");
        addr >>= 8;
    }
    return ip;
}

struct ipv4info{
    uint32_t net_address;
    uint16_t net_port;
    explicit ipv4info() : net_address{}, net_port{} {}
    explicit ipv4info(uint32_t net_address, uint16_t net_port) : net_address{net_address}, net_port{net_port} {}
    auto operator<=>(const ipv4info&) const = default;

    auto toString() const {
        return std::format("{}:{}", my_inet_ntoa(net_address), math::ntoh(net_port));
    }

};

enum class udpv4_error{
    BIND_ERROR,
    TIMEOUT_ERROR,
    RECVFROM_ERROR,
    SENDTO_ERROR
};

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
class wsainiter{
public:
    WSADATA wsaData;
    explicit wsainiter(){
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            LOG.async_log("[ERROR] WSAStartup() failed\n");
            std::exit(1);
        }
    }
    ~wsainiter(){
        WSACleanup();
    }
};
inline static wsainiter wsa{};
using socket_t = SOCKET;
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

    bool bind(ipv4info info);
    bool set_timeout(uint32_t timeout);

    std::expected<size_t, udpv4_error> recvfrom(ipv4info& src, void* buffer, size_t buffer_size);
    udpv4_error sendto(const ipv4info& dest, const void* data, size_t size);

};






uint32_t query_device_ip(uint32_t interface_index);

std::map<uint32_t, std::tuple<std::u8string, uint32_t>> query_all_device_ip();

inline uint16_t random_pri_iana_net_port() {
    return math::hton(math::random<uint16_t>(32768, 65535));
}