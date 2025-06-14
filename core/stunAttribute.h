#pragma once
#include <cstdint>
#include <type_traits>
#include "net_core.h"
#include "stun.h"
namespace stun {
    using namespace math;
    namespace attribute {
        // should be understood
        constexpr uint16_t MAPPED_ADDRESS = hton<uint16_t>(0x0001);
        constexpr uint16_t USERNAME = hton<uint16_t>(0x0006);
        constexpr uint16_t MESSAGE_INTEGRITY = hton<uint16_t>(0x0008);
        constexpr uint16_t ERROR_CODE = hton<uint16_t>(0x0009);
        constexpr uint16_t UNKNOWN_ATTRIBUTES = hton<uint16_t>(0x000A);
        constexpr uint16_t REALM = hton<uint16_t>(0x0014);
        constexpr uint16_t NONCE = hton<uint16_t>(0x0015);
        constexpr uint16_t XOR_MAPPED_ADDRESS = hton<uint16_t>(0x0020);

        constexpr uint16_t CHANGE_REQUEST = hton<uint16_t>(0x0003);
        constexpr uint16_t PADDING = hton<uint16_t>(0x0026);
        constexpr uint16_t RESPONSE_PORT = hton<uint16_t>(0x0027);
        constexpr uint16_t RESPONSE_ORIGIN = hton<uint16_t>(0x802B);
        constexpr uint16_t OTHER_ADDRESS = hton<uint16_t>(0x802C);



        // don't have to be understood
        constexpr uint16_t SOFTWARE = hton<uint16_t>(0x8022);
        constexpr uint16_t ALTERNATE_SERVER = hton<uint16_t>(0x8023);
        constexpr uint16_t FINGERPRINT = hton<uint16_t>(0x8028);
    }
    constexpr uint32_t CHANGE_IP_FLAG = hton<uint32_t>(0x04);
    constexpr uint32_t CHANGE_PORT_FLAG = hton<uint32_t>(0x02);

    constexpr uint16_t E300_TRY_ALTERNATE = hton<uint16_t>(0x0300);
    constexpr uint16_t E400_BAD_REQUEST = hton<uint16_t>(0x0400);
    constexpr uint16_t E401_UNAUTHORIZED = hton<uint16_t>(0x0401);
    constexpr uint16_t E420_UNKNOWN_ATTRIBUTE = hton<uint16_t>(0x0420);
    constexpr uint16_t E438_STALE_NONCE = hton<uint16_t>(0x0438);
    constexpr uint16_t E500_SERVER_ERROR = hton<uint16_t>(0x0500);


    struct alignas(4) attr;
    template <typename attribute_t>
    concept is_stunAttribute = std::is_base_of_v<attr, attribute_t> && (sizeof(attribute_t) % 4 == 0);

    struct alignas(4) attr {
        uint16_t type;
        uint16_t length;


        explicit attr(uint16_t type, uint16_t length) : type{type}, length{length} {}


        template<is_stunAttribute attribute_t>
        attribute_t* as(){
            return reinterpret_cast<attribute_t*>(this);
        }
        uint8_t* get_value_ptr() {
            return reinterpret_cast<uint8_t*>(this) + sizeof(attr);
        }
        constexpr static uint16_t getid(){ return 0;}
    };

    struct ipv4_mappedAddress : public attr {
        uint8_t zero;
        uint8_t family;
        uint16_t port;
        uint32_t address;
        constexpr static uint16_t getid(){ return stun::attribute::MAPPED_ADDRESS;}
    };

    struct ipv4_xor_mappedAddress : public attr {
        uint8_t zero;
        uint8_t family;
        uint16_t x_port;
        uint32_t x_address;
        constexpr static uint16_t getid(){ return stun::attribute::XOR_MAPPED_ADDRESS;}
    };

    struct ipv4_responseOrigin : public attr {
        uint8_t zero;
        uint8_t family;
        uint16_t port;
        uint32_t address;
        constexpr static uint16_t getid(){ return stun::attribute::RESPONSE_ORIGIN;}
    };

    struct ipv4_otherAddress : public attr {
        uint8_t zero;
        uint8_t family;
        uint16_t port;
        uint32_t address;
        constexpr static uint16_t getid(){ return stun::attribute::OTHER_ADDRESS;}
    };

    struct changeRequest : public attr {
        uint32_t flags;
        constexpr static uint16_t getid(){ return stun::attribute::CHANGE_REQUEST;}
        
        explicit changeRequest(uint32_t flags) : 
            attr{stun::attribute::CHANGE_REQUEST, math::hton<uint16_t>(sizeof(flags))},
            flags{flags} {}
    };

    struct softWare : public attr {
        char value[0];
        constexpr static uint16_t getid(){ return stun::attribute::SOFTWARE;}
    };


    struct fingerPrint : public attr {
        uint32_t crc32;
        constexpr static uint16_t getid(){ return stun::attribute::FINGERPRINT;}
        explicit fingerPrint(uint32_t crc32) : 
            attr{stun::attribute::FINGERPRINT, math::hton<uint16_t>(sizeof(crc32))}, 
            crc32{crc32} {}

    };

    struct errorCode : public attr {
        uint16_t zero;
        uint16_t error_code;
        char error_reason[0];
        uint16_t unknown_attributes[0];
        constexpr static uint16_t getid(){ return stun::attribute::ERROR_CODE;}
    };

    struct responsePort : public attr {
        uint16_t port;
        uint16_t padding;
        constexpr static uint16_t getid(){ return stun::attribute::RESPONSE_PORT;}
        explicit responsePort(uint16_t port) : 
            attr{stun::attribute::RESPONSE_PORT, math::hton<uint16_t>(sizeof(port))}, 
            port{port} {}
    };
}