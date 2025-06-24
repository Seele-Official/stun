#pragma once
#include <map>
#include "net/ipv4.h"

namespace seele::net{

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

}