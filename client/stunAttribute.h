#pragma once
#include "stun.h"

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
    constexpr uint8_t CHANGE_IP_FLAG = 0x04;
    constexpr uint8_t CHANGE_PORT_FLAG = 0x02;
}
struct stunAttribute;
template <typename attribute_t>
concept is_stunAttribute = std::is_base_of_v<stunAttribute, attribute_t> && (sizeof(attribute_t) % 4 == 0);

struct stunAttribute {
    uint16_t type;
    uint16_t length;
    uint8_t value[0];

    template<is_stunAttribute attribute_t>
    attribute_t* as(){
        return reinterpret_cast<attribute_t*>(this);
    }

    constexpr uint16_t getid(){ return 0;}
};

struct ipv4_mappedAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr uint16_t getid(){ return stun::attribute::MAPPED_ADDRESS;}
};

struct ipv4_xor_mappedAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t x_port;
    uint32_t x_address;
    constexpr uint16_t getid(){ return stun::attribute::XOR_MAPPED_ADDRESS;}
};

struct ipv4_responseOrigin : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr uint16_t getid(){ return stun::attribute::RESPONSE_ORIGIN;}
};

struct ipv4_otherAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr uint16_t getid(){ return stun::attribute::OTHER_ADDRESS;}
};

struct changeRequest : public stunAttribute {
    uint8_t zero[3];
    uint8_t flags;
    constexpr uint16_t getid(){ return stun::attribute::CHANGE_REQUEST;}
};

struct softWare : public stunAttribute {
    char value[0];
    constexpr uint16_t getid(){ return stun::attribute::SOFTWARE;}
};