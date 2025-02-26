#pragma once

#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include "client.h" 

class linux_client : public client<linux_client, ipv4info> {
private:
    friend class client<linux_client, ipv4info>;

    int socketfd;
    timeval timeout;
    uint32_t myIP; 
    uint16_t myPort;

    void send(request_t request);
    response_t receive();

    
public:
    explicit linux_client(uint32_t myIP, uint16_t myPort = randomPort());
        
    inline ~linux_client() {
        close(socketfd);
    }


    static uint32_t query_device_ip(std::string_view interface);
    static std::map<std::string, uint32_t> query_all_device_ip();
    static uint16_t randomPort();

    
    inline uint32_t getMyIP() const { return myIP; }
    inline uint16_t getMyPort() const { return myPort; }
    inline ipv4info getMyInfo() const { return ipv4info{myIP, myPort}; }
};


