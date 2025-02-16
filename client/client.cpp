#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "client.h"
#include "stun.h"


struct ipv4info{
    uint32_t address;
    uint16_t port;

    explicit ipv4info(uint32_t address, uint16_t port) : address{address}, port{port} {}
};


class clientImpl : public client<clientImpl>{
private:
    friend class client<clientImpl>;

    int socketfd;
    sockaddr_in remoteAddr;

    uint32_t myIP; 
    uint16_t myPort;
    // void send(stunMessage_view msg){
    //     sendto(socketfd, msg.data(), msg.size(), 0, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
    // }


    // stunMessage receive(){
    //     static constexpr size_t buffer_size = 1024;
    //     static uint8_t buffer[buffer_size];

    //     sockaddr_in from{};
    //     socklen_t fromlen = sizeof(from);
    //     auto recvlen = recvfrom(socketfd, buffer, buffer_size, 0, (sockaddr*)&from, &fromlen);

    //     if (recvlen == -1){
    //         return stunMessage{};
    //     }

    //     return stunMessage{buffer, static_cast<size_t>(recvlen)};

    // }

    static uint32_t queryMyIP(){
        ifaddrs *ifAddrStruct = nullptr;
        if (getifaddrs(&ifAddrStruct) == -1) {
            perror("getifaddrs");
            return -1;
        }

        for (ifaddrs *it = ifAddrStruct; it != nullptr; it = it->ifa_next) {
            if (it->ifa_addr == nullptr) continue;

            if (it->ifa_addr->sa_family == AF_INET && strcmp(it->ifa_name, "eth2") == 0) {
                return ((sockaddr_in*)it->ifa_addr)->sin_addr.s_addr;
            }
        }
        if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
        return -1;
    }
    static uint16_t randomPort(){
        return htons(49152 + rand() % 16383);
    }

public:
    explicit clientImpl(std::string_view ip, uint16_t port) : remoteAddr{}, myIP{queryMyIP()}, myPort{randomPort()} {


        socketfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socketfd == -1){
            //
        }
        
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_port = myPort;
        local.sin_addr.s_addr = myIP;

        if (bind(socketfd, (sockaddr*)&local, sizeof(local)) == -1){
            //
        }
        
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(port);
        remoteAddr.sin_addr.s_addr = inet_addr(ip.data());
    }

    ~clientImpl(){
        close(socketfd);
    }


    uint32_t getMyIP() const {
        return myIP;
    }

    uint16_t getMyPort() const {
        return myPort;
    }

};



int main(){

    stunMessage msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);


    clientImpl c{"195.208.107.138", 3478};

    std::cout << "my ip: " << inet_ntoa({c.getMyIP()}) << ":" << ntohs(c.getMyPort()) << std::endl;

    auto response = c.asyncRequest(msg);

    if (response.get_return_value().empty()){
        std::cout << "timeout" << std::endl;
    } else {
        for (auto& attr : response.get_return_value().getAttributes()){
            switch (attr->type){
                case stun::attribute::MAPPED_ADDRESS:
                    {
                        auto mappedAddress = attr->as<ipv4_mappedAddress>();
                        std::cout << "MAPPED_ADDRESS: " << inet_ntoa({mappedAddress->address}) << ":" << ntohs(mappedAddress->port) << std::endl;
                    }
                    break;
                case stun::attribute::XOR_MAPPED_ADDRESS:
                    {
                        auto mappedAddress = attr->as<ipv4_xor_mappedAddress>();
                        std::cout << "XOR_MAPPED_ADDRESS: " 
                            << inet_ntoa({mappedAddress->x_address ^ stun::MAGIC_COOKIE})
                            << ":" 
                            << ntohs(mappedAddress->x_port ^ stun::MAGIC_COOKIE) << std::endl;
                    }
                    break;
                case stun::attribute::RESPONSE_ORIGIN:
                    {
                        auto responseOrigin = attr->as<ipv4_responseOrigin>();
                        std::cout << "RESPONSE_ORIGIN: " << inet_ntoa({responseOrigin->address}) << ":" << ntohs(responseOrigin->port) << std::endl;
                    }
                    break;
                case stun::attribute::OTHER_ADDRESS:
                    {
                        auto otherAddress = attr->as<ipv4_otherAddress>();
                        std::cout << "OTHER_ADDRESS: " << inet_ntoa({otherAddress->address}) << ":" << ntohs(otherAddress->port) << std::endl;
                    }
                    break;
                case stun::attribute::SOFTWARE:
                    {
                        std::cout << "SOFTWARE: " << attr->value << std::endl;
                    }
                    break;
                default:
                    {
                        std::cout << "unknown attribute" << std::endl;
                    }

            
            }
        }

    }
    return 0;
}

