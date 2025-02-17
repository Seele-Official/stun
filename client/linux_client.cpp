#include "linux_client.h"

void linux_client::send(request_t request) {
    auto [ip, size, data] = request;
    LOG.log("sending to {}:{}\n", my_inet_ntoa(ip.net_address), my_ntohs(ip.net_port));
    trval_stunMessage(stunMessage_view{data});
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = ip.net_port;
    remoteAddr.sin_addr.s_addr = ip.net_address;

    if (sendto(socketfd, data, size, 0, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == -1) {
        LOG.log("[ERROR] sendto() failed: {}\n", strerror(errno));
    }
}

linux_client::response_t linux_client::receive() {
    static constexpr size_t buffer_size = 1024;
    static uint8_t buffer[buffer_size];

    sockaddr_in from{};
    socklen_t fromlen = sizeof(from);
    auto recvlen = recvfrom(socketfd, buffer, buffer_size, 0, (sockaddr*)&from, &fromlen);

    if (recvlen == -1) {
        LOG.log("[ERROR] recvfrom() failed: {}\n", strerror(errno));
        return response_t{ipv4info{}, 0, nullptr};
    }

    LOG.log("received from {}:{}\n", my_inet_ntoa(from.sin_addr.s_addr), my_ntohs(from.sin_port));
    trval_stunMessage(stunMessage_view{buffer});

    return response_t{
        ipv4info{
            from.sin_addr.s_addr, 
            from.sin_port
        },
        recvlen, buffer
    };
}

uint32_t linux_client::queryMyIP() {
    ifaddrs *ifAddrStruct = nullptr;
    if (getifaddrs(&ifAddrStruct) == -1) {
        LOG.log("[ERROR] getifaddrs() failed: {}\n", strerror(errno));
        return -1;
    }

    for (ifaddrs *it = ifAddrStruct; it != nullptr; it = it->ifa_next) {
        if (it->ifa_addr == nullptr) continue;

        if (it->ifa_addr->sa_family == AF_INET) {
            if (strcmp(it->ifa_name, "lo") == 0) continue;
            std::cout << "choosen interface: " << it->ifa_name << std::endl;
            return ((sockaddr_in*)it->ifa_addr)->sin_addr.s_addr;
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return -1;
}

uint16_t linux_client::randomPort() {
    return htons(random<uint16_t>(49152, 65535));
}

linux_client::linux_client(uint16_t myport) : myIP{queryMyIP()}, myPort{myport} {
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        LOG.log("[ERROR] socket() failed: {}\n", strerror(errno));
        std::exit(1);
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = myPort;
    local.sin_addr.s_addr = myIP;

    if (myIP == -1 || bind(socketfd, (sockaddr*)&local, sizeof(local)) == -1) {
        LOG.log("[ERROR] bind() failed: {}\n", strerror(errno));
        std::exit(1);
    }

    timeout.tv_sec = 1;  
    timeout.tv_usec = 0;

    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    this->start_listener();
}

linux_client::~linux_client() {
    close(socketfd);
}
