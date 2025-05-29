#pragma once
#include <cstdint>
#include <type_traits>
#include "net_core.h"
namespace stun {
    namespace attribute {
        // should be understood
        constexpr uint16_t MAPPED_ADDRESS = my_htons(0x0001);
        constexpr uint16_t USERNAME = my_htons(0x0006);
        constexpr uint16_t MESSAGE_INTEGRITY = my_htons(0x0008);
        constexpr uint16_t ERROR_CODE = my_htons(0x0009);
        constexpr uint16_t UNKNOWN_ATTRIBUTES = my_htons(0x000A);
        constexpr uint16_t REALM = my_htons(0x0014);
        constexpr uint16_t NONCE = my_htons(0x0015);
        constexpr uint16_t XOR_MAPPED_ADDRESS = my_htons(0x0020);

        constexpr uint16_t CHANGE_REQUEST = my_htons(0x0003);
        constexpr uint16_t PADDING = my_htons(0x0026);
        constexpr uint16_t RESPONSE_PORT = my_htons(0x0027);
        constexpr uint16_t RESPONSE_ORIGIN = my_htons(0x802B);
        constexpr uint16_t OTHER_ADDRESS = my_htons(0x802C);



        // don't have to be understood
        constexpr uint16_t SOFTWARE = my_htons(0x8022);
        constexpr uint16_t ALTERNATE_SERVER = my_htons(0x8023);
        constexpr uint16_t FINGERPRINT = my_htons(0x8028);
    }
    constexpr uint32_t CHANGE_IP_FLAG = my_htonl(0x04);
    constexpr uint32_t CHANGE_PORT_FLAG = my_htonl(0x02);

    constexpr uint16_t E300_TRY_ALTERNATE = my_htons(0x0300);
    constexpr uint16_t E400_BAD_REQUEST = my_htons(0x0400);
    constexpr uint16_t E401_UNAUTHORIZED = my_htons(0x0401);
    constexpr uint16_t E420_UNKNOWN_ATTRIBUTE = my_htons(0x0420);
    constexpr uint16_t E438_STALE_NONCE = my_htons(0x0438);
    constexpr uint16_t E500_SERVER_ERROR = my_htons(0x0500);

}
struct stun_attr;
template <typename attribute_t>
concept is_stunAttribute = std::is_base_of_v<stun_attr, attribute_t> && (sizeof(attribute_t) % 4 == 0);

struct stun_attr {
    uint16_t type;
    uint16_t length;


    explicit stun_attr(uint16_t type, uint16_t length) : type{type}, length{length} {}


    template<is_stunAttribute attribute_t>
    attribute_t* as(){
        return reinterpret_cast<attribute_t*>(this);
    }
    uint8_t* get_value_ptr() {
        return reinterpret_cast<uint8_t*>(this) + sizeof(stun_attr);
    }
    constexpr static uint16_t getid(){ return 0;}
};

struct ipv4_mappedAddress : public stun_attr {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::MAPPED_ADDRESS;}
};

struct ipv4_xor_mappedAddress : public stun_attr {
    uint8_t zero;
    uint8_t family;
    uint16_t x_port;
    uint32_t x_address;
    constexpr static uint16_t getid(){ return stun::attribute::XOR_MAPPED_ADDRESS;}
};

struct ipv4_responseOrigin : public stun_attr {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::RESPONSE_ORIGIN;}
};

struct ipv4_otherAddress : public stun_attr {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::OTHER_ADDRESS;}
};

struct changeRequest : public stun_attr {
    uint32_t flags;
    constexpr static uint16_t getid(){ return stun::attribute::CHANGE_REQUEST;}
    
    explicit changeRequest(uint32_t flags) : 
        stun_attr{stun::attribute::CHANGE_REQUEST, my_htons(sizeof(flags))},
        flags{flags} {}
};

struct softWare : public stun_attr {
    char value[0];
    constexpr static uint16_t getid(){ return stun::attribute::SOFTWARE;}
};


struct fingerPrint : public stun_attr {
    uint32_t crc32;
    constexpr static uint16_t getid(){ return stun::attribute::FINGERPRINT;}
    explicit fingerPrint(uint32_t crc32) : 
        stun_attr{stun::attribute::FINGERPRINT, my_htons(sizeof(crc32))}, 
        crc32{crc32} {}

    static uint32_t crc32_bitwise(const uint8_t* data, size_t len);

};

struct errorCode : public stun_attr {
    uint16_t zero;
    uint16_t error_code;
    char error_reason[0];
    uint16_t unknown_attributes[0];
    constexpr static uint16_t getid(){ return stun::attribute::ERROR_CODE;}
};

struct responsePort : public stun_attr {
    uint16_t port;
    uint16_t padding;
    constexpr static uint16_t getid(){ return stun::attribute::RESPONSE_PORT;}
    explicit responsePort(uint16_t port) : 
        stun_attr{stun::attribute::RESPONSE_PORT, my_htons(sizeof(port))}, 
        port{port} {}
};
