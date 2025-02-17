#include "stun.h"


#if defined(_WIN32) || defined(_WIN64)
#include "win_client.h"
using clientImpl = win_client;
#elif defined(__linux__)
#include "linux_client.h"
using clientImpl = linux_client;
#endif


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
    stunMessage request_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    stunMessage request_msg2(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    stunMessage request_msg3(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    
    // test 1
    auto response = c.asyncRequest(server_ip, request_msg);
    auto& [ip, responce_msg] = response.get_return_value();
    if (responce_msg.empty()){ /* unsupported */ return mapping_type::unsupported;}


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
    

    if (first_x_ip == c.getMyInfo()){
        // endpoint independent mapping, no NAT
        return mapping_type::endpoint_independent_mapping_no_nat;
    }


    // test 2
    
    auto response2 = c.asyncRequest(
        ipv4info{server_other_ip.net_address, server_ip.net_port},
        request_msg2
    );

    auto& [ip2, responce_msg2] = response2.get_return_value();
    if (responce_msg2.empty()){/* unsupported */ return mapping_type::unsupported;}

    auto second_x_address = responce_msg2.find<ipv4_xor_mappedAddress>();
    if (second_x_address == nullptr){/* unsupported */ return mapping_type::unsupported;}
    ipv4info second_x_ip{
        second_x_address->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(second_x_address->x_port ^ stun::MAGIC_COOKIE)
    };

    if (second_x_ip == first_x_ip){
        // endpoint independent mapping
        return mapping_type::endpoint_independent_mapping;
    }





    // test 3
    auto response3 = c.asyncRequest(
        server_other_ip,
        request_msg3
    );
    auto& [ip3, responce_msg3] = response3.get_return_value();

    if (responce_msg3.empty()){/* unsupported */ return mapping_type::unsupported;}

    auto third_x_address = responce_msg3.find<ipv4_xor_mappedAddress>();
    if (third_x_address == nullptr){/* unsupported */ return mapping_type::unsupported;}
    ipv4info third_x_ip{
        third_x_address->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(third_x_address->x_port ^ stun::MAGIC_COOKIE)
    };

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
        // endpoint independent filtering
        return filtering_type::endpoint_independent_filtering;
    }


    // test 3
    stunMessage request_msg2(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    request_msg2.emplace<changeRequest>(stun::CHANGE_PORT_FLAG);

    auto response2 = c.asyncRequest(server_ip, request_msg2, 2);
    auto& [ip2, responce_msg2] = response2.get_return_value();
    if (!responce_msg2.empty()){
        // address dependent filtering
        return filtering_type::address_dependent_filtering;
    } else {
        // address and port dependent filtering
        return filtering_type::address_and_port_dependent_filtering;
    }

}


void nat_test(){
    clientImpl c{};
    std::cout << "my ip: " << my_inet_ntoa(c.getMyIP()) << ":" << my_ntohs(c.getMyPort()) << std::endl;

    ipv4info serverip{"3.122.159.108", 3478};


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
    LOG.set_enable(true);
    LOG.set_output_file("nat_test.log");
    nat_test();

    return 0;
}
