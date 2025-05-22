#include "net_core.h"


#if defined(_WIN32) || defined(_WIN64)
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <system_error>



udpv4::udpv4(){
    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd == INVALID_SOCKET) {
        LOG.async_log("[ERROR] socket() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        std::exit(1);
    }
}


udpv4::~udpv4(){
    if (socketfd != INVALID_SOCKET)
        closesocket(socketfd);
    WSACleanup();
}


bool udpv4::bind(ipv4info info){
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = info.net_port;
    local.sin_addr.s_addr = info.net_address;

    if (::bind(socketfd, (sockaddr*)&local, sizeof(local)) == -1) {
        LOG.async_log("[ERROR] bind() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        return false;
    }
    return true;
}

bool udpv4::set_timeout(uint32_t t){
    DWORD tv = t * 1000;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, 
        reinterpret_cast<const char*>(&tv), sizeof(tv)) == -1) {
        LOG.async_log("[ERROR] setsockopt() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        return false;
    }
    return true;
}
std::expected<size_t, udpv4_error> udpv4::recvfrom(ipv4info& src, void* buffer, size_t buffer_size){
    sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    auto recv_size = ::recvfrom(socketfd, reinterpret_cast<char*>(buffer), buffer_size, 0, reinterpret_cast<sockaddr*>(&src_addr), &src_addr_len);
    if (recv_size == -1){
        LOG.log("[ERROR] recvfrom() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        return std::unexpected{udpv4_error::RECVFROM_ERROR};
    }
    src.net_address = src_addr.sin_addr.s_addr;
    src.net_port = src_addr.sin_port;
    return recv_size;
}

udpv4_error udpv4::sendto(const ipv4info& dest, const void* data, size_t size){
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dest.net_address;
    addr.sin_port = dest.net_port;
    if (::sendto(socketfd, reinterpret_cast<const char*>(data), size, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1){
        LOG.log("[ERROR] sendto() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        return udpv4_error::SENDTO_ERROR;
    }
    return udpv4_error{};
}



uint32_t query_device_ip(uint32_t interface_index){
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);
    pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(outBufLen));
    dwRetVal = GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);

    if (dwRetVal != NO_ERROR) {
        free(pAddresses);
        return 0;
    }


    for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next) {

        if (interface_index == 0) {
            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                 pUnicast; pUnicast = pUnicast->Next) {
                sockaddr_in* sockaddr = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                if (sockaddr->sin_family == AF_INET) {
                    uint32_t res = sockaddr->sin_addr.s_addr;
                    free(pAddresses);
                    return res;
                }
            }
        }


        if (interface_index == pCurr->IfIndex) {

            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                 pUnicast; pUnicast = pUnicast->Next) {
                sockaddr_in* sockaddr = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                if (sockaddr->sin_family == AF_INET) {
                    uint32_t res = sockaddr->sin_addr.s_addr;
                    free(pAddresses);
                    return res;
                }
            }

        }
    }

    free(pAddresses);
    return 0;
}

std::map<uint32_t, std::tuple<std::u8string, uint32_t>> query_all_device_ip(){
    std::map<uint32_t, std::tuple<std::u8string, uint32_t>> res;
    
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // 两次调用，第一次获取所需缓冲区大小
    GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);
    pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(outBufLen));
    dwRetVal = GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);

    if (dwRetVal != NO_ERROR) {
        free(pAddresses);
        return res;
    }

    for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next) {

        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
             pUnicast; pUnicast = pUnicast->Next) {

            
            sockaddr_in* sockaddr = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
            if (sockaddr->sin_family == AF_INET) {
                
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, nullptr, 0, nullptr, nullptr);
                std::u8string u8(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, reinterpret_cast<char*>(u8.data()), size_needed, nullptr, nullptr);
                res[pCurr->IfIndex] = std::make_tuple(
                    u8,
                    sockaddr->sin_addr.s_addr
                );
            }
        }
    }

    free(pAddresses);
    return res;
}






#elif defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>


udpv4::udpv4(){
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1){
        LOG.log("[ERROR] socket() failed: {}\n", strerror(errno));
        std::exit(1);
    }
}

udpv4::udpv4(udpv4&& other){
    socketfd = other.socketfd;
    other.socketfd = -1;
}

udpv4& udpv4::operator=(udpv4&& other){
    if (this != &other){
        if (socketfd != -1)
            close(socketfd);
        socketfd = other.socketfd;
        other.socketfd = -1;
    }
    return *this;
}


udpv4::~udpv4(){
    if (socketfd != -1)
        close(socketfd);
}


bool udpv4::bind(ipv4info info){
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = info.net_address;
    addr.sin_port = info.net_port;
    if (::bind(socketfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1){
        LOG.log("[ERROR] bind() failed: {}\n", strerror(errno));
        return false;
    }
    
    return true;
}
bool udpv4::set_timeout(uint32_t t){
    timeval timeout{t, 0};
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
        LOG.log("[ERROR] setsockopt() failed: {}\n", strerror(errno));
        return false;
    }
    return true;
}
std::expected<size_t, udpv4_error> udpv4::recvfrom(ipv4info& src, void* buffer, size_t buffer_size){
    sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    ssize_t recv_size = ::recvfrom(socketfd, buffer, buffer_size, 0, reinterpret_cast<sockaddr*>(&src_addr), &src_addr_len);
    if (recv_size == -1){
        LOG.log("[ERROR] recvfrom() failed: {}\n", strerror(errno));
        return std::unexpected{udpv4_error::RECVFROM_ERROR};
    }
    src.net_address = src_addr.sin_addr.s_addr;
    src.net_port = src_addr.sin_port;
    return recv_size;
}

udpv4_error udpv4::sendto(const ipv4info& dest, const void* data, size_t size){
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dest.net_address;
    addr.sin_port = dest.net_port;
    if (::sendto(socketfd, data, size, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1){
        LOG.log("[ERROR] sendto() failed: {}\n", strerror(errno));
        return udpv4_error::SENDTO_ERROR;
    }
    return udpv4_error{};
}



uint32_t query_device_ip(uint32_t interface_index){
    ifaddrs *ifAddrStruct = nullptr;
    if (getifaddrs(&ifAddrStruct) == -1) {
        LOG.async_log("[ERROR] getifaddrs() failed: {}\n", strerror(errno));
        return 0;
    }

    for (ifaddrs *it = ifAddrStruct; it != nullptr; it = it->ifa_next) {
        if (it->ifa_addr == nullptr) continue;

        if (it->ifa_addr->sa_family == AF_INET) {
            if (interface_index == 0 && it->ifa_name != std::string_view("lo"))
                return ((sockaddr_in*)it->ifa_addr)->sin_addr.s_addr;
            if (if_nametoindex(it->ifa_name) == interface_index)
                return ((sockaddr_in*)it->ifa_addr)->sin_addr.s_addr;
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return 0;
}

std::map<uint32_t, std::tuple<std::u8string, uint32_t>> query_all_device_ip(){
    std::map<uint32_t, std::tuple<std::u8string, uint32_t>> res;
    ifaddrs *ifAddrStruct = nullptr;
    if (getifaddrs(&ifAddrStruct) == -1) {
        LOG.async_log("[ERROR] getifaddrs() failed: {}\n", strerror(errno));
        return res;
    }

    for (ifaddrs *it = ifAddrStruct; it != nullptr; it = it->ifa_next) {
        if (it->ifa_addr == nullptr) continue;

        if (it->ifa_addr->sa_family == AF_INET) {
            res[if_nametoindex(it->ifa_name)] = std::make_tuple(
                reinterpret_cast<const char8_t*>(it->ifa_name),
                ((sockaddr_in*)it->ifa_addr)->sin_addr.s_addr
            );
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return res;
}



#endif