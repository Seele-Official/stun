#include "win_client.h"
#include <cstdint>

void win_client::send(request_t request) {
    auto [ip, size, data] = request;
    LOG.log("sending to {}:{}\n", my_inet_ntoa(ip.net_address), my_ntohs(ip.net_port));
    trval_stunMessage(stunMessage_view{data});
    sockaddr_in remoteAddr{};
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = ip.net_port;
    remoteAddr.sin_addr.s_addr = ip.net_address;

    if (sendto(socketfd, reinterpret_cast<const char*>(data), size, 0,
        (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        LOG.log("[ERROR] sendto() failed: {}\n", 
            std::system_error(WSAGetLastError(), std::system_category()).what());
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
        auto error = WSAGetLastError();
        if (error != WSAETIMEDOUT) {
            LOG.log("[ERROR] recvfrom() failed: {}\n",
                std::system_error(error, std::system_category()).what());
        }
        return response_t{ipv4info{}, 0, nullptr};
    }

    LOG.log("received from {}:{}\n", my_inet_ntoa(from.sin_addr.s_addr), my_ntohs(from.sin_port));
    trval_stunMessage(stunMessage_view{buffer});
    return response_t{
        ipv4info{
            from.sin_addr.s_addr,
            from.sin_port
        },
        static_cast<size_t>(recvlen),
        buffer
    };
}
uint32_t win_client::queryMyIP() {
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // 两次调用，第一次获取所需缓冲区大小
    GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);
    pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(outBufLen));
    dwRetVal = GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &outBufLen);

    if (dwRetVal != NO_ERROR) {
        free(pAddresses);
        return INADDR_NONE;
    }

    for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr; pCurr = pCurr->Next) {
        // 1. 过滤非活动接口
        if (pCurr->OperStatus != IfOperStatusUp) continue;

        // 2. 过滤虚拟适配器（关键步骤）
        const std::wstring friendlyName(pCurr->FriendlyName);
        if (friendlyName.find(L"vEthernet") != std::wstring::npos ||   // Hyper-V 虚拟适配器
            friendlyName.find(L"Virtual") != std::wstring::npos ||     // 通用虚拟适配器
            friendlyName.find(L"VPN") != std::wstring::npos ||        // VPN 适配器
            friendlyName.find(L"VMware") != std::wstring::npos)        // VMware 虚拟适配器
            {          
            continue;
        }

        // 3. 过滤回环和隧道接口
        if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||  // 回环接口
            pCurr->IfType == IF_TYPE_TUNNEL) {             // 隧道接口
            continue;
        }

        // 4. 只处理物理以太网和无线接口
        if (pCurr->IfType != IF_TYPE_ETHERNET_CSMACD &&    // 有线以太网
            pCurr->IfType != IF_TYPE_IEEE80211) {          // 无线 Wi-Fi
            continue;
        }

        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
             pUnicast; pUnicast = pUnicast->Next) {
            sockaddr_in* sockaddr = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
            if (sockaddr->sin_family == AF_INET) {
                uint32_t ip = sockaddr->sin_addr.s_addr;
                free(pAddresses);
                return ip;
            }
        }
    }

    free(pAddresses);
    return INADDR_NONE;
}

uint16_t win_client::randomPort() {
    return htons(random<uint16_t>(49152, 65535));
}



win_client::win_client(uint16_t myport) : myIP(queryMyIP()), myPort(myport) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG.log("[ERROR] WSAStartup() failed\n");
        std::exit(1);
    }

    socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd == INVALID_SOCKET) {
        LOG.log("[ERROR] socket() failed: {}\n",
            std::system_error(WSAGetLastError(), std::system_category()).what());
        WSACleanup();
        std::exit(1);
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = myPort;
    local.sin_addr.s_addr = myIP;

    if (bind(socketfd, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        LOG.log("[ERROR] bind() failed: {}\n",
            std::system_error(WSAGetLastError(), std::system_category()).what());
        closesocket(socketfd);
        WSACleanup();
        std::exit(1);
    }

    DWORD tv = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, 
        reinterpret_cast<const char*>(&tv), sizeof(tv));

    this->start_listener();
}

win_client::~win_client() {
    closesocket(socketfd);
    WSACleanup();
}
