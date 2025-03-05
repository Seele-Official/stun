#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <expected>
#include "random.h"
#include "log.h"
#include "net_core.h"




struct transactionID_t {
    uint8_t data[12];
    auto operator<=>(const transactionID_t&) const = default;
    transactionID_t(const transactionID_t&) = default;
    operator std::string() const {
        return tohex(*this);
    }
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

class stunMessage_view {
private:
    const stunHeader* header;
    std::vector<stunAttribute*> attributes;

public:
    explicit stunMessage_view(const uint8_t* p);
    explicit stunMessage_view(stunHeader* header, std::vector<stunAttribute*> attributes) 
        : header(header), attributes(std::move(attributes)) {}

    inline const stunHeader* getHeader() const { return header; }
    inline const transactionID_t& getTransactionID() const { return header->transactionID; }
    inline const std::vector<stunAttribute*>& getAttributes() const { return attributes; }
    inline const stunAttribute* operator[](size_t index) const { return attributes[index]; }
    inline size_t size() const { return my_ntohs(header->length) + sizeof(stunHeader); }
    inline const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(header); }
};

class stunMessage {
private:
    uint8_t* data;
    stunHeader* header;
    std::vector<stunAttribute*> attributes;
    uint8_t* endptr;

    inline void setLength(uint16_t length) { header->length = my_htons(length); }

public:
    inline explicit stunMessage() : data{nullptr}, header{nullptr}, endptr{nullptr} {}
    explicit stunMessage(uint16_t type);
    explicit stunMessage(const uint8_t* p);
    
    stunMessage(const stunMessage&) = delete;
    stunMessage(stunMessage&& other) noexcept;
    ~stunMessage();

    stunMessage& operator=(const stunMessage&) = delete;
    stunMessage& operator=(stunMessage&& other) noexcept;

    inline const transactionID_t& getTransactionID() const { return header->transactionID; }
    inline const std::vector<stunAttribute*>& getAttributes() const { return attributes; }
    inline const uint8_t* data_ptr() const { return data; }
    inline size_t size() const { return endptr - data; }
    inline bool empty() const { return header == nullptr; }
    inline operator stunMessage_view() const { return stunMessage_view{header, attributes}; }

    template <is_stunAttribute attribute_t>
    bool append(attribute_t* attribute);

    template <is_stunAttribute attribute_t, typename... args_t>
    bool emplace(args_t&&... args);

    template <is_stunAttribute attribute_t>
    attribute_t* find_one();

    template <is_stunAttribute... attribute_t>
    std::tuple<attribute_t*...> find();

    inline const uint16_t getType() const { return header->type; }


    static bool isValid(uint8_t* p);
};

template <is_stunAttribute attribute_t>
bool stunMessage::append(attribute_t* attribute) {
    if (this->endptr + sizeof(attribute_t) > data + 548) {
        return false;
    }

    std::memcpy(endptr, attribute, sizeof(attribute_t));
    this->attributes.emplace_back(reinterpret_cast<stunAttribute*>(this->endptr));
    this->endptr += sizeof(attribute_t);
    this->setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
    return true;
}

template <is_stunAttribute attribute_t, typename... args_t>
bool stunMessage::emplace(args_t&&... args) {
    if (this->endptr + sizeof(attribute_t) > data + 548) {
        return false;
    }

    new (this->endptr) attribute_t(std::forward<args_t>(args)...);
    this->attributes.emplace_back(reinterpret_cast<stunAttribute*>(this->endptr));
    this->endptr += sizeof(attribute_t);
    this->setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
    return true;
}

template <is_stunAttribute attribute_t>
attribute_t* stunMessage::find_one() {
    for (auto& attr : attributes) {
        if (attr->type == attribute_t::getid()) {
            return attr->as<attribute_t>();
        }
    }
    return nullptr;
}

template <is_stunAttribute... attribute_t>
std::tuple<attribute_t*...>  stunMessage::find() {
    std::tuple<attribute_t*...> res;
    for (auto& attr : attributes) {
        
        if((((std::get<attribute_t*>(res) == nullptr) ?
                !((attr->type == attribute_t::getid()) &&
                (std::get<attribute_t*>(res) = attr->as<attribute_t>()))
                : false
            ) || ...)
        ){
            continue;
        }
        
    }
    return res;
}


void log_stunMessage(stunMessage_view msg);