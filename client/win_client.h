#pragma once

#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstring>
#include <string>
#include <cstring>
#include <system_error>
#include "client.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

std::wstring string_to_wstring(const std::string& str, uint32_t codePage = CP_UTF8);

class win_client : public client<win_client, ipv4info> {
private:
    friend class client<win_client, ipv4info>;

    SOCKET socketfd;
    uint32_t myIP;
    uint16_t myPort;
    WSADATA wsaData;

    void send(request_t request);
    response_t receive();



public:
    explicit win_client(uint32_t myIP, uint16_t myPort = randomPort());
    ~win_client();

    static uint32_t query_device_ip(uint32_t interface_index);
    static std::map<uint32_t, std::tuple<std::wstring, uint32_t>> query_all_device_ip();
    static uint16_t randomPort();

    inline uint32_t getMyIP() const { return myIP; }
    inline uint16_t getMyPort() const { return myPort; }
    inline ipv4info getMyInfo() const { return ipv4info{myIP, myPort}; }
};