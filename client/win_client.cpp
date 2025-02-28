#include "win_client.h"

void win_client::send(request_t request) {
    auto [ip, size, data] = request;
    LOG.async_log("sending to {}:{}\n", my_inet_ntoa(ip.net_address), my_ntohs(ip.net_port));
    log_stunMessage(stunMessage_view{data});
    sockaddr_in remoteAddr{};
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = ip.net_port;
    remoteAddr.sin_addr.s_addr = ip.net_address;

    if (sendto(socketfd, reinterpret_cast<const char*>(data), size, 0,
        (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        LOG.async_log("[ERROR] sendto() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
    }
}

win_client::response_t win_client::receive() {
    static constexpr size_t buffer_size = 1024;
    static uint8_t buffer[buffer_size];

    sockaddr_in from{};
    int fromlen = sizeof(from);
    auto recvlen = recvfrom(socketfd, reinterpret_cast<char*>(buffer), buffer_size, 0,
        (sockaddr*)&from, &fromlen);

    if (recvlen == SOCKET_ERROR) {
        LOG.async_log("[ERROR] recvfrom() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        return response_t{ipv4info{}, 0, nullptr};
    }

    LOG.async_log("received from {}:{}\n", my_inet_ntoa(from.sin_addr.s_addr), my_ntohs(from.sin_port));
    log_stunMessage(stunMessage_view{buffer});
    return response_t{
        ipv4info{
            from.sin_addr.s_addr,
            from.sin_port
        },
        static_cast<size_t>(recvlen),
        buffer
    };
}
uint32_t win_client::query_device_ip(uint32_t interface_index) {
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

uint16_t win_client::randomPort() {
    return htons(random<uint16_t>(49152, 65535));
}
std::map<uint32_t, std::tuple<std::wstring, uint32_t>> win_client::query_all_device_ip(){
    std::map<uint32_t, std::tuple<std::wstring, uint32_t>> res;

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
                res[pCurr->IfIndex] = std::make_tuple(
                    std::wstring(pCurr->FriendlyName),
                    sockaddr->sin_addr.s_addr
                );
            }
        }
    }

    free(pAddresses);
    return res;
}


win_client::win_client(uint32_t myIP, uint16_t myPort) : myIP(myIP), myPort(myPort) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG.async_log("[ERROR] WSAStartup() failed\n");
        std::exit(1);
    }

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd == INVALID_SOCKET) {
        LOG.async_log("[ERROR] socket() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        std::exit(1);
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = myPort;
    local.sin_addr.s_addr = myIP;

    if (bind(socketfd, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        LOG.async_log("[ERROR] bind() failed: {}\n", std::system_error(WSAGetLastError(), std::system_category()).what());
        std::exit(1);
    }

    DWORD tv = 1000;
    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, 
        reinterpret_cast<const char*>(&tv), sizeof(tv));

    this->start_listener();
}

win_client::~win_client() {
    closesocket(socketfd);
    WSACleanup();
}

std::wstring string_to_wstring(const std::string& str, uint32_t codePage) {
    int len = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    std::wstring res(len, 0);
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, res.data(), len);
    return res;
}