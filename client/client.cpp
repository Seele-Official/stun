#include <cstdint>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <tuple>
#include "client.h"
#include "stun.h"
#include "stunAttribute.h"


struct ipv4info{
    uint32_t net_address;
    uint16_t net_port;
    explicit ipv4info() : net_address{}, net_port{} {}
    explicit ipv4info(uint32_t net_address, uint16_t net_port) : net_address{net_address}, net_port{net_port} {}
    explicit ipv4info(std::string_view ip, uint16_t port) : net_address{inet_addr(ip.data())}, net_port{htons(port)} {}

    auto operator<=>(const ipv4info&) const = default;

};


class clientImpl : public client<clientImpl, ipv4info>{
private:
    friend class client<clientImpl, ipv4info>;
    using client<clientImpl, ipv4info>::request_t;
    using client<clientImpl, ipv4info>::responce_t;

    int socketfd;
    timeval timeout;
    uint32_t myIP; 
    uint16_t myPort;
    void send(request_t request){
        auto [ip, msg] = request;

        sockaddr_in remoteAddr;
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = ip.net_port;
        remoteAddr.sin_addr.s_addr = ip.net_address;
        sendto(socketfd, msg.data(), msg.size(), 0, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
    }


    responce_t receive(){
        static constexpr size_t buffer_size = 1024;
        static uint8_t buffer[buffer_size];

        sockaddr_in from{};
        socklen_t fromlen = sizeof(from);
        auto recvlen = recvfrom(socketfd, buffer, buffer_size, 0, (sockaddr*)&from, &fromlen);

        if (recvlen == -1){
            return responce_t{ipv4info{}, stunMessage{}};
        }
        std::cout << "received " << recvlen << " bytes" << std::endl;
        return responce_t{
            ipv4info{from.sin_addr.s_addr, from.sin_port}, 
            stunMessage{buffer, static_cast<size_t>(recvlen)}
        };

    }

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
    explicit clientImpl() : myIP{queryMyIP()}, myPort{randomPort()} {


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
        
        timeout.tv_sec = 1;  
        timeout.tv_usec = 0;
        
        setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
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

    ipv4info getMyInfo() const {
        return ipv4info{myIP, myPort};
    }

};


void trval(stunMessage_view msg){
    for (auto& attr : msg.getAttributes()){
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



enum class mapping_type{
    unsupported,
    endpoint_independent_mapping_no_nat,
    endpoint_independent_mapping,
    address_dependent_mapping,
    address_and_port_dependent_mapping,
};

enum class filtering_type{
    endpoint_independent_filtering,
    address_dependent_filtering,
    address_and_port_dependent_filtering
};

mapping_type maping_test(clientImpl& c, ipv4info& server_ip){

    // test 1
    stunMessage request_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);

    auto response = c.asyncRequest(server_ip, request_msg);
    auto& [ip, responce_msg] = response.get_return_value();
    if (responce_msg.empty()){ /* unsupported */ return mapping_type::unsupported;}


    std::cout << "response from: " << inet_ntoa({ip.net_address}) << ":" << ntohs(ip.net_port) << std::endl;

    auto x_address = responce_msg.find<ipv4_xor_mappedAddress>();
    auto other_address = responce_msg.find<ipv4_otherAddress>();
    if (other_address == nullptr || x_address == nullptr){ /* unsupported */ return mapping_type::unsupported;}

    ipv4info first_x_ip{
        x_address->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_address->x_port ^ stun::MAGIC_COOKIE)
    }, server_other_ip{
        other_address->address,
        other_address->port
    };
    
    std::cout << "x_ip: " << inet_ntoa({first_x_ip.net_address}) << ":" << ntohs(first_x_ip.net_port) << std::endl;
    std::cout << "server_other_ip: " << inet_ntoa({server_other_ip.net_address}) << ":" << ntohs(server_other_ip.net_port) << std::endl;

    if (first_x_ip == c.getMyInfo()){
        // endpoint independent mapping, no NAT
        return mapping_type::endpoint_independent_mapping_no_nat;
    }


    // test 2
    stunMessage request_msg2(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    auto response2 = c.asyncRequest(
        ipv4info{server_other_ip.net_address, htons(3478)},
        request_msg2
    );

    auto& [ip2, responce_msg2] = response2.get_return_value();
    if (responce_msg2.empty()){/* unsupported */ return mapping_type::unsupported;}
    std::cout << "response from: " << inet_ntoa({ip2.net_address}) << ":" << ntohs(ip2.net_port) << std::endl;

    auto second_x_address = responce_msg2.find<ipv4_xor_mappedAddress>();
    if (second_x_address == nullptr){/* unsupported */ return mapping_type::unsupported;}
    ipv4info second_x_ip{
        second_x_address->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(second_x_address->x_port ^ stun::MAGIC_COOKIE)
    };
    std::cout << "second_x_ip: " << inet_ntoa({second_x_ip.net_address}) << ":" << ntohs(second_x_ip.net_port) << std::endl;

    if (second_x_ip == first_x_ip){
        // endpoint independent mapping
        return mapping_type::endpoint_independent_mapping;
    }





    // test 3
    stunMessage request_msg3(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    auto response3 = c.asyncRequest(
        server_other_ip,
        request_msg3
    );
    auto& [ip3, responce_msg3] = response3.get_return_value();

    if (responce_msg3.empty()){/* unsupported */ return mapping_type::unsupported;}
    std::cout << "response from: " << inet_ntoa({ip3.net_address}) << ":" << ntohs(ip3.net_port) << std::endl;

    auto third_x_address = responce_msg3.find<ipv4_xor_mappedAddress>();
    if (third_x_address == nullptr){/* unsupported */ return mapping_type::unsupported;}
    ipv4info third_x_ip{
        third_x_address->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(third_x_address->x_port ^ stun::MAGIC_COOKIE)
    };
    std::cout << "third_x_ip: " << inet_ntoa({third_x_ip.net_address}) << ":" << ntohs(third_x_ip.net_port) << std::endl;

    if (third_x_ip == second_x_ip){
        // address dependent mapping
        return mapping_type::address_dependent_mapping;
    } else {
        // address and port dependent mapping
        return mapping_type::address_and_port_dependent_mapping;
    }

}

filtering_type filtering_test(clientImpl& c, ipv4info& server_ip){
    // test 2
    stunMessage request_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    request_msg.emplace<changeRequest>(stun::CHANGE_IP_FLAG | stun::CHANGE_PORT_FLAG);

    auto response = c.asyncRequest(server_ip, request_msg, 2);
    auto& [ip, responce_msg] = response.get_return_value();
    if (!responce_msg.empty()){
        std::cout << "response from: " << inet_ntoa({ip.net_address}) << ":" << ntohs(ip.net_port) << std::endl;
        // endpoint independent filtering
        return filtering_type::endpoint_independent_filtering;
    }


    // test 3
    stunMessage request_msg2(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    request_msg2.emplace<changeRequest>(stun::CHANGE_PORT_FLAG);

    auto response2 = c.asyncRequest(server_ip, request_msg2, 2);
    auto& [ip2, responce_msg2] = response2.get_return_value();
    if (!responce_msg2.empty()){
        std::cout << "response from: " << inet_ntoa({ip2.net_address}) << ":" << ntohs(ip2.net_port) << std::endl;
        // address dependent filtering
        return filtering_type::address_dependent_filtering;
    } else {
        // address and port dependent filtering
        return filtering_type::address_and_port_dependent_filtering;
    }

}


void nat_test(){
    clientImpl c{};
    std::cout << "my ip: " << inet_ntoa({c.getMyIP()}) << ":" << ntohs(c.getMyPort()) << std::endl;

    ipv4info serverip{"195.208.107.138", 3478};


    switch (maping_test(c, serverip)){
        case mapping_type::unsupported:
            std::cout << "unsupported" << std::endl;
            break;
        case mapping_type::endpoint_independent_mapping_no_nat:
            std::cout << "endpoint independent mapping, no NAT" << std::endl;
            break;
        case mapping_type::endpoint_independent_mapping:
            std::cout << "endpoint independent mapping" << std::endl;
            break;
        case mapping_type::address_dependent_mapping:
            std::cout << "address dependent mapping" << std::endl;
            break;
        case mapping_type::address_and_port_dependent_mapping:
            std::cout << "address and port dependent mapping" << std::endl;
            break;
    }


    switch (filtering_test(c, serverip)){
        case filtering_type::endpoint_independent_filtering:
            std::cout << "endpoint independent filtering" << std::endl;
            break;
        case filtering_type::address_dependent_filtering:
            std::cout << "address dependent filtering" << std::endl;
            break;
        case filtering_type::address_and_port_dependent_filtering:
            std::cout << "address and port dependent filtering" << std::endl;
            break;
    }

}



int main(){

    nat_test();

    return 0;
}

