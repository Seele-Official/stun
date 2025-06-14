#include "nat_test.h"
#include "log.h"
#include "net_core.h"
#include "stun.h"
#include "stunAttribute.h"
#include <cstdint>
#include <expected>
std::expected<uint8_t, std::string> maping_test(clientImpl& c, ipv4info& server_addr, ipv4info& server_altaddr, ipv4info& first_x_maddr){
    
    stun::message ipmaping_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
    
    auto res = c.async_req(
        ipv4info{
            server_altaddr.net_address,
            server_addr.net_port
        }, ipmaping_test_msg)
        .get_as_rvalue();
    
    if (!res.has_value()){
        return std::unexpected(res.error());
    }



    auto x_addr = std::get<1>(res.value()).find_one<stun::ipv4_xor_mappedAddress>();
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


    stun::message portmaping_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
    auto res2 = c.async_req(server_altaddr, portmaping_test_msg)
        .get_as_rvalue();

    if (!res2.has_value()){
        return std::unexpected(res2.error());
    }

    auto x_addr2 = std::get<1>(res2.value()).find_one<stun::ipv4_xor_mappedAddress>();
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

    stun::message ipfiltering_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
    
    ipfiltering_test_msg.emplace<stun::changeRequest>(stun::CHANGE_IP_FLAG | stun::CHANGE_PORT_FLAG);

    if(c.async_req(server_addr, ipfiltering_test_msg)
        .get()
        .has_value()){
        return endpoint_independent_filtering;
    }

    stun::message portfiltering_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
    portfiltering_test_msg.emplace<stun::changeRequest>(stun::CHANGE_PORT_FLAG);

    return c.async_req(server_addr, portfiltering_test_msg)
        .get()
        .has_value() ? 
        address_dependent_filtering : address_and_port_dependent_filtering;
}

std::expected<nat_type, std::string> nat_test(clientImpl &c, ipv4info server_addr){

    stun::message udp_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);

    auto res = c.async_req(server_addr, udp_test_msg)
                    .get_as_rvalue();

    if (!res.has_value()) return std::unexpected(res.error());

    auto& [ipinfo, responce_msg] = res.value();    
    auto [x_addr, otheraddr] = responce_msg.find<stun::ipv4_xor_mappedAddress, stun::ipv4_otherAddress>();
    if (otheraddr == nullptr || x_addr == nullptr) return std::unexpected("server does not support stun-behavior");

    ipv4info first_x_maddr{
        x_addr->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_addr->x_port ^ stun::MAGIC_COOKIE)
    }, server_altaddr{
        otheraddr->address,
        otheraddr->port
    };

    if (server_addr.net_address == server_altaddr.net_address || 
        server_addr.net_port == server_altaddr.net_port) return std::unexpected("server has undefined behavior");

    if (first_x_maddr == c.get_my_addr()){
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

std::expected<ipv4info, std::string> build_binding(clientImpl& c, ipv4info& server_addr){
    stun::message ip_test_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
    auto res = c.async_req(server_addr, ip_test_msg)
                    .get_as_rvalue();

    if (!res.has_value()) return std::unexpected(res.error());

    auto& [ipinfo, responce_msg] = res.value();    
    auto x_addr = responce_msg.find_one<stun::ipv4_xor_mappedAddress>();
    if (x_addr == nullptr) return std::unexpected("server does not support stun-behavior");

    return ipv4info{
        x_addr->x_address ^ stun::MAGIC_COOKIE, 
        static_cast<uint16_t>(x_addr->x_port ^ stun::MAGIC_COOKIE)
    };
}

std::expected<uint64_t, std::string> lifetime_test(clientImpl& X, clientImpl& Y, ipv4info& server_addr) {
    // Phase 1: Exponential search
    constexpr uint64_t ACCEPTABLE_ERROR = 15;
    uint64_t low = 0;
    uint64_t high = 0;
    uint64_t lifetime = 10;

    while (true) {
        ASYNC_LOG("Testing lifetime={}s\n", lifetime);
        stun::message X_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
        auto res = X.async_req(server_addr, X_msg).get_as_rvalue();
        if (!res.has_value()) return std::unexpected(res.error());

        auto x_addr = std::get<1>(res.value()).find_one<stun::ipv4_xor_mappedAddress>();
        if (!x_addr) return std::unexpected("server does not support stun-behavior");
        uint16_t X_port = x_addr->x_port ^ stun::MAGIC_COOKIE;

        std::this_thread::sleep_for(std::chrono::seconds(lifetime));

        stun::message Y_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
        Y_msg.emplace<stun::responsePort>(X_port);
        auto res2 = Y.async_req(server_addr, Y_msg).get_as_rvalue();

        if (!res2.has_value()) {
            high = lifetime;
            low = high / 2;
            break;
        }

        auto& Y_res = std::get<1>(res2.value());
        if (Y_res.get_type() == (stun::msg_type::ERROR_RESPONSE | stun::msg_method::BINDING)) {
            auto err = Y_res.find_one<stun::errorCode>();
            if (!err) return std::unexpected("Server error: Missing error code");
            
            if (err->error_code == stun::E420_UNKNOWN_ATTRIBUTE) {
                std::string err_msg = "Unsupported attributes:";
                for (size_t i = 0; i < err->length/sizeof(uint16_t); i++) {
                    err_msg += tohex(math::ntoh(err->unknown_attributes[i]));
                }
                return std::unexpected(err_msg);
            }
            return std::unexpected("Server error: Code=" + std::to_string(err->error_code));
        }

        low = lifetime;
        lifetime *= 2;
    }

    // Phase 2: Binary search
    while (low < high && high - low > ACCEPTABLE_ERROR) {
        uint64_t mid = low + (high - low) / 2;
        ASYNC_LOG("Testing lifetime={}s\n", mid);

        stun::message X_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
        auto res = X.async_req(server_addr, X_msg).get_as_rvalue();
        if (!res.has_value()) return std::unexpected(res.error());

        auto x_addr = std::get<1>(res.value()).find_one<stun::ipv4_xor_mappedAddress>();
        if (!x_addr) return std::unexpected("server has undefined behavior");
        uint16_t X_port = x_addr->x_port ^ stun::MAGIC_COOKIE;

        std::this_thread::sleep_for(std::chrono::seconds(mid));

        stun::message Y_msg(stun::msg_method::BINDING | stun::msg_type::REQUEST);
        Y_msg.emplace<stun::responsePort>(X_port);
        auto res2 = Y.async_req(server_addr, Y_msg).get_as_rvalue();

        if (!res2.has_value()) {
            high = mid;
        } else {
            auto& Y_res = std::get<1>(res2.value());
            if (Y_res.get_type() == (stun::msg_type::ERROR_RESPONSE | stun::msg_method::BINDING)) {
                auto err = Y_res.find_one<stun::errorCode>();
                if (!err) return std::unexpected("Server error: Missing error code");
                
                if (err->error_code == stun::E420_UNKNOWN_ATTRIBUTE) {
                    return std::unexpected("Unexpected E420 error in binary phase");
                }
                high = mid;
            } else {
                low = mid + 1;
            }
        }
    }

    return high;
}