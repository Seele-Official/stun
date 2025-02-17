#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstring>
#include <string>
#include <system_error>
#include "client.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

class win_client : public client<win_client, ipv4info> {
private:
    friend class client<win_client, ipv4info>;

    SOCKET socketfd;
    timeval timeout;
    uint32_t myIP;
    uint16_t myPort;
    WSADATA wsaData;

    void send(request_t request);
    response_t receive();

    static uint32_t queryMyIP();
    static uint16_t randomPort();

public:
    explicit win_client(uint16_t myport = randomPort());
    ~win_client();

    inline uint32_t getMyIP() const { return myIP; }
    inline uint16_t getMyPort() const { return myPort; }
    inline ipv4info getMyInfo() const { return ipv4info{myIP, myPort}; }
};