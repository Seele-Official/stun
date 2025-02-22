#include "nat_test.h"
#include "stunAttribute.h"
std::expected<uint8_t, std::string> maping_test(clientImpl& c, ipv4info& server_addr, ipv4info& server_altaddr, ipv4info& first_x_maddr){

    stunMessage ipmaping_test_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    
    auto res = c.asyncRequest(
        ipv4info{
            server_altaddr.net_address,
            server_addr.net_port
        }, ipmaping_test_msg, 2)
        .get_return_rvalue();
    
    if (!res.has_value()){
        return std::unexpected(res.error());
    }



    auto x_addr = std::get<1>(res.value()).find<ipv4_xor_mappedAddress>();
    if (x_addr == nullptr){
        return std::unexpected("server has undefined behavior");
    }

    ipv4info second_x_maddr{
        x_addr->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_addr->x_port ^ stun::MAGIC_COOKIE)
    };

    if (first_x_maddr == second_x_maddr){
        return endpoint_independent_mapping;
    }


    stunMessage portmaping_test_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    auto res2 = c.asyncRequest(
        ipv4info{
            server_altaddr.net_address,
            server_altaddr.net_port
        }, portmaping_test_msg, 2)
        .get_return_rvalue();

    if (!res2.has_value()){
        return std::unexpected(res2.error());
    }

    auto x_addr2 = std::get<1>(res2.value()).find<ipv4_xor_mappedAddress>();
    if (x_addr2 == nullptr){
        return std::unexpected("server has undefined behavior");
    }

    ipv4info third_x_maddr{
        x_addr2->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_addr2->x_port ^ stun::MAGIC_COOKIE)
    };

    return first_x_maddr == third_x_maddr ? address_dependent_mapping : address_and_port_dependent_mapping;

}

uint8_t filtering_test(clientImpl& c, ipv4info& server_addr){

    stunMessage ipfiltering_test_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    
    ipfiltering_test_msg.emplace<changeRequest>(stun::CHANGE_IP_FLAG | stun::CHANGE_PORT_FLAG);

    if(c.asyncRequest(server_addr, ipfiltering_test_msg, 2)
        .get_return_lvalue()
        .has_value()){
        return endpoint_independent_filtering;
    }

    stunMessage portfiltering_test_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);
    portfiltering_test_msg.emplace<changeRequest>(stun::CHANGE_PORT_FLAG);

    return c.asyncRequest(
        ipv4info{
            server_addr.net_address,
            server_addr.net_port
        }, portfiltering_test_msg, 2)
        .get_return_lvalue()
        .has_value() ? 
        address_dependent_filtering : address_and_port_dependent_filtering;
}

std::expected<nat_type, std::string> nat_test(clientImpl &c, ipv4info server_addr){

    stunMessage udp_test_msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);

    auto res = c.asyncRequest(server_addr, udp_test_msg, 2)
                    .get_return_rvalue();

    if (!res.has_value()) {
        return std::unexpected(res.error());
    }



    auto& [ipinfo, responce_msg] = res.value();    
    auto x_addr = responce_msg.find<ipv4_xor_mappedAddress>();
    auto otheraddr = responce_msg.find<ipv4_otherAddress>();
    if (otheraddr == nullptr || x_addr == nullptr){
        return std::unexpected("server does not support STUN");
    }

    ipv4info first_x_maddr{
        x_addr->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_addr->x_port ^ stun::MAGIC_COOKIE)
    }, server_altaddr{
        otheraddr->address,
        otheraddr->port
    };
    
    if (server_addr.net_address == server_altaddr.net_address || server_addr.net_port == server_altaddr.net_port){
        return std::unexpected("server has undefined behavior");
    }


    if (first_x_maddr == c.getMyInfo()){
        return nat_type{
            filtering_test(c, server_addr),
            no_nat_mapping
        };

    } else {
        auto filtering = filtering_test(c, server_addr);
        auto res = maping_test(c, server_addr, server_altaddr, first_x_maddr);
        if (!res.has_value()){
            return std::unexpected(res.error());
        }

        auto mapping = res.value();
        
        return nat_type{
            filtering,
            mapping
        };
    }



}


