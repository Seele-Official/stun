#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <type_traits>
#include <vector>
#if BYTE_ORDER == BIG_ENDIAN
constexpr uint32_t my_htonl(uint32_t hostlong) {
    return hostlong;
}

constexpr uint16_t my_htons(uint16_t hostshort) {
    return hostshort;
}

constexpr uint32_t my_ntohl(uint32_t netlong) {
    return my_htonl(netlong);
}

constexpr uint16_t my_ntohs(uint16_t netshort) {
    return my_htons(netshort);
}


#elif BYTE_ORDER == LITTLE_ENDIAN

constexpr uint32_t my_htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8) |
           ((hostlong & 0x00FF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

constexpr uint16_t my_htons(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

constexpr uint32_t my_ntohl(uint32_t netlong) {
    return my_htonl(netlong);
}

constexpr uint16_t my_ntohs(uint16_t netshort) {
    return my_htons(netshort);
}

#endif


struct transactionID_t {
    uint8_t data[12];
    auto operator<=>(const transactionID_t&) const = default;
    transactionID_t(const transactionID_t&) = default;
};


struct stunHeader {
    uint16_t type;
    uint16_t length;
    uint32_t magicCookie;
    transactionID_t transactionID;
};

namespace stun {

    namespace messagetype {
        constexpr uint16_t REQUEST = my_htons(0x0000);
        constexpr uint16_t INDICATION = my_htons(0x0010);
        constexpr uint16_t SUCCESS_RESPONSE = my_htons(0x0100);
        constexpr uint16_t ERROR_RESPONSE = my_htons(0x0110);
    }
    namespace messagemethod{
        constexpr uint16_t BINDING = my_htons(0x0001);
    }

    constexpr uint32_t MAGIC_COOKIE = my_htonl(0x2112A442);

}

#include "stunAttribute.h"


struct stunMessage_view {
private:
    const stunHeader* header;

    std::vector<stunAttribute*> attributes;
public:
    explicit stunMessage_view(stunHeader* header) : header(header) {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(header) + sizeof(stunHeader);
        const uint8_t* end = ptr + my_ntohs(header->length);

        while (ptr < end) {
            stunAttribute* attribute = reinterpret_cast<stunAttribute*>(ptr);
            attributes.emplace_back(attribute);
            ptr += sizeof(stunAttribute) + my_ntohs(attribute->length) + (4 - my_ntohs(attribute->length) % 4);
        }
    }

    explicit stunMessage_view(stunHeader* header, std::vector<stunAttribute*> attributes) : header(header), attributes(attributes) {} 

    // const stunHeader* getHeader() const {
    //     return header;
    // }

    const transactionID_t& getTransactionID() const {
        return header->transactionID;
    }


    const std::vector<stunAttribute*>& getAttributes() const {
        return attributes;
    }

    const stunAttribute* operator[](size_t index) const {
        return attributes[index];
    }

    size_t size() const {
        return my_ntohl(header->length) + sizeof(stunHeader);
    }

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(header);
    }

};



struct stunMessage{
private:
    uint8_t* data;

    stunHeader* header;

    std::vector<stunAttribute*> attributes;
    uint8_t* endptr;

    void setLength(uint16_t length){
        header->length = my_htons(length);
    }
        

public:
    explicit stunMessage() : data{nullptr}, header{nullptr}, endptr{nullptr} {}

    explicit stunMessage(uint16_t type){
        this->data = new uint8_t[548];
        this->header = reinterpret_cast<stunHeader*>(data);

        this->header->type = type;
        this->header->length = 0;
        this->header->magicCookie = stun::MAGIC_COOKIE;
        this->endptr = data + sizeof(stunHeader);


        std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
        urandom.read(reinterpret_cast<char*>(this->header->transactionID.data), 12);
        urandom.close();

    }

    explicit stunMessage(const uint8_t* data, size_t size) : data{new uint8_t[size]}{
        std::memcpy(this->data, data, size);
        this->header = reinterpret_cast<stunHeader*>(this->data);
        this->endptr = this->data + size;

        uint8_t* ptr = this->data + sizeof(stunHeader);


        while (ptr < this->endptr) {
            stunAttribute* attribute = reinterpret_cast<stunAttribute*>(ptr);
            attributes.emplace_back(attribute);

            auto len = sizeof(stunAttribute) + my_ntohs(attribute->length);
            if (len % 4 != 0) len += 4 - len % 4;
            ptr += len;


        }
    }

    explicit stunMessage(stunMessage& other) = delete;
    explicit stunMessage(stunMessage&& other) : data{other.data}, header{other.header}, attributes{std::move(other.attributes)}, endptr{other.endptr} {
        other.data = nullptr;
        other.header = nullptr;
        other.endptr = nullptr;
    }


    ~stunMessage(){
        if (data) delete[] data;
    }

    auto operator=(stunMessage& other) -> stunMessage& = delete;
    auto operator=(stunMessage&& other) -> stunMessage& {
        if (data) delete[] data;
        data = other.data;
        header = other.header;
        attributes = std::move(other.attributes);
        endptr = other.endptr;

        other.data = nullptr;
        other.header = nullptr;
        other.endptr = nullptr;

        return *this;
    }





    // const stunHeader* getHeader() const {
    //     return header;
    // }

    const transactionID_t& getTransactionID() const {
        return header->transactionID;
    }

    const std::vector<stunAttribute*>& getAttributes() const {
        return attributes;
    }

    bool empty() const {
        return header == nullptr;
    }

    bool isValid() const {
        return header->magicCookie == stun::MAGIC_COOKIE;
    }

    operator stunMessage_view() const {
        return stunMessage_view{header, attributes};
    }



    template<is_stunAttribute attribute_t>
    bool apppend(attribute_t* attribute){
        if (endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        std::memcpy(endptr, attribute, sizeof(attribute_t));
        endptr += sizeof(attribute_t);
        setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
        return true;
    }

    template<is_stunAttribute attribute_t, typename... args_t>
    bool emplace(args_t&&... args){
        if (endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        new(endptr) attribute_t(std::forward<args_t>(args)...);
        endptr += sizeof(attribute_t);
        setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
        return true;
    }


    template<is_stunAttribute attribute_t>
    attribute_t* find(){
        for (auto& attr : attributes){
            if (attr->type == attribute_t::getid()){
                return attr->as<attribute_t>();
            }
        }
        return nullptr;
    }

};