#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <format>
#include <expected>
#include <map>
#include "random.h"
#include "log.h"
constexpr std::expected<uint32_t, char> my_stoi(std::string_view str){
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
    return tohex(&struct_t, sizeof(T));
}


#if MY_BYTE_ORDER == MY_BIG_ENDIAN
constexpr uint32_t my_htonl(uint32_t hostlong) {
    return hostlong;
}

constexpr uint16_t my_htons(uint16_t hostshort) {
    return hostshort;
}

constexpr uint32_t my_ntohl(uint32_t netlong) {
    return my_htonl(netlong);
}

constexpr uint16_t my_ntohs(uint16_t netshort) {
    return my_htons(netshort);
}



#elif MY_BYTE_ORDER == MY_LITTLE_ENDIAN

constexpr uint32_t my_htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

constexpr uint16_t my_htons(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

constexpr uint32_t my_ntohl(uint32_t netlong) {
    return my_htonl(netlong);
}

constexpr uint16_t my_ntohs(uint16_t netshort) {
    return my_htons(netshort);
}

#endif

constexpr std::expected<uint32_t, std::string> my_inet_addr(std::string_view ip) {
    uint32_t addr = 0;
    for (size_t i = 0; i < 3; i++) {
        auto pos = ip.find('.');
        if (pos == std::string_view::npos) {
            return std::unexpected{"missing '.'"};
        }
        auto num = my_stoi(ip.substr(0, pos));
        if (!num.has_value()) {
            return std::unexpected{std::format("unexpected char: {}", tohex(num.error()))};
        }
        addr = (addr << 8) | num.value();
        ip.remove_prefix(pos + 1);
    }
    auto num = my_stoi(ip);
    if (!num.has_value()) {
        return std::unexpected{std::format("unexpected char: {}", tohex(num.error()))};
    }
    addr = (addr << 8) | num.value();

    return my_htonl(addr);
}

constexpr std::string my_inet_ntoa(uint32_t addr) {
    addr = my_ntohl(addr);
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
        return std::format("{}:{}", my_inet_ntoa(net_address), my_ntohs(net_port));
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
#endif


class udpv4{
private:
    int socketfd;
public:
    explicit udpv4();
    ~udpv4();

    bool bind(ipv4info info);
    bool set_timeout(uint32_t timeout);

    std::expected<size_t, udpv4_error> recvfrom(ipv4info& src, void* buffer, size_t buffer_size);
    udpv4_error sendto(const ipv4info& dest, const void* data, size_t size);

};






uint32_t query_device_ip(uint32_t interface_index);

std::map<uint32_t, std::tuple<std::u8string, uint32_t>> query_all_device_ip();

inline uint16_t randomPort() {
    return my_htons(random<uint16_t>(49152, 65535));
}