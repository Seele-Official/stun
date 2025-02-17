#pragma once
#include "stun.h"
#include <array>
#include <cstdint>

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
}
struct stunAttribute;
template <typename attribute_t>
concept is_stunAttribute = std::is_base_of_v<stunAttribute, attribute_t> && (sizeof(attribute_t) % 4 == 0);

struct stunAttribute {
    uint16_t type;
    uint16_t length;
    uint8_t value[0];

    explicit stunAttribute(uint16_t type, uint16_t length) : type{type}, length{length} {}


    template<is_stunAttribute attribute_t>
    attribute_t* as(){
        return reinterpret_cast<attribute_t*>(this);
    }

    constexpr static uint16_t getid(){ return 0;}
};

struct ipv4_mappedAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::MAPPED_ADDRESS;}
};

struct ipv4_xor_mappedAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t x_port;
    uint32_t x_address;
    constexpr static uint16_t getid(){ return stun::attribute::XOR_MAPPED_ADDRESS;}
};

struct ipv4_responseOrigin : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::RESPONSE_ORIGIN;}
};

struct ipv4_otherAddress : public stunAttribute {
    uint8_t zero;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    constexpr static uint16_t getid(){ return stun::attribute::OTHER_ADDRESS;}
};

struct changeRequest : public stunAttribute {
    uint32_t flags;
    constexpr static uint16_t getid(){ return stun::attribute::CHANGE_REQUEST;}
    
    explicit changeRequest(uint32_t flags) : 
        flags{flags}, 
        stunAttribute{stun::attribute::CHANGE_REQUEST, my_htons(sizeof(flags))} {}
};

struct softWare : public stunAttribute {
    char value[0];
    constexpr static uint16_t getid(){ return stun::attribute::SOFTWARE;}
};


struct fingerPrint : public stunAttribute {
    uint32_t crc32;
    constexpr static uint16_t getid(){ return stun::attribute::FINGERPRINT;}
    explicit fingerPrint(uint32_t crc32) : 
        crc32{crc32}, 
        stunAttribute{stun::attribute::FINGERPRINT, my_htons(sizeof(crc32))} {}


    static constexpr std::array<uint32_t, 256> crc32_table() {
        constexpr uint32_t poly = 0xEDB88320; // 反射多项式
        std::array<uint32_t, 256> table{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
            }
            table[i] = crc;
        }
        return table;
    }
    
    static uint32_t crc32_bitwise(const uint8_t* data, size_t len) {
        
        static constexpr auto CRC32_TABLE = crc32_table();

        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; ++i) {
          crc = (crc >> 8) ^ CRC32_TABLE[(crc ^ data[i]) & 0xFF];
        }
        return crc ^ 0xFFFFFFFF;
      }
        
};