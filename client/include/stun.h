#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <format>
#include <expected>
#include "random.h"
#include "log.h"
#define MY_LITTLE_ENDIAN 0
#define MY_BIG_ENDIAN 1
constexpr std::expected<uint32_t, char> my_stoi(std::string_view str){
    uint32_t num = 0;
    for (auto c : str){
        if (c < '0' || c > '9') return std::unexpected{c};
        num = num * 10 + c - '0';
    }
    return num;
}


constexpr auto tohex(void* ptr, size_t size){
    std::string str{"0x"};
    for (size_t i = 0; i < size; i++){
        constexpr char e[] = "0123456789ABCDEF";
        str += std::format("{}{}", e[((uint8_t*)ptr)[i] >> 4], e[((uint8_t*)ptr)[i] & 0xF]);
    }
    return str;
}

template <typename T>
constexpr auto tohex(T struct_t){
    return tohex(&struct_t, sizeof(T));
}


#if MY_BYTE_ORDER == MY_BIG_ENDIAN
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



#elif MY_BYTE_ORDER == MY_LITTLE_ENDIAN

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

constexpr std::expected<uint32_t, std::string> my_inet_addr(std::string_view ip) {
    uint32_t addr = 0;
    for (size_t i = 0; i < 3; i++) {
        auto pos = ip.find('.');
        if (pos == std::string_view::npos) {
            return std::unexpected{"expected '.'"};
        }
        auto num = my_stoi(ip.substr(0, pos));
        if (!num.has_value()) {
            return std::unexpected{std::string{"unexpected char: "} + num.error()};
        }
        addr = (addr << 8) | num.value();
        ip.remove_prefix(pos + 1);
    }
    auto num = my_stoi(ip);
    if (!num.has_value()) {
        return std::unexpected{std::string{"unexpected char: "} + num.error()};
    }
    addr = (addr << 8) | num.value();

    return my_htonl(addr);
}

constexpr std::string my_inet_ntoa(uint32_t addr) {
    addr = my_ntohl(addr);
    std::string ip;
    for (size_t i = 0; i < 4; i++) {
        ip.insert(0, std::to_string(addr & 0xFF));
        if (i != 3) ip.insert(0, ".");
        addr >>= 8;
    }
    return ip;
}

struct ipv4info{
    uint32_t net_address;
    uint16_t net_port;
    explicit ipv4info() : net_address{}, net_port{} {}
    explicit ipv4info(uint32_t net_address, uint16_t net_port) : net_address{net_address}, net_port{net_port} {}
    auto operator<=>(const ipv4info&) const = default;

    auto toString() const {
        return std::format("{}:{}", my_inet_ntoa(net_address), my_ntohs(net_port));
    }

};


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
    inline bool empty() const { return header == nullptr; }
    inline operator stunMessage_view() const { return stunMessage_view{header, attributes}; }

    template <is_stunAttribute attribute_t>
    bool append(attribute_t* attribute);

    template <is_stunAttribute attribute_t, typename... args_t>
    bool emplace(args_t&&... args);

    template <is_stunAttribute attribute_t>
    attribute_t* find();

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
attribute_t* stunMessage::find() {
    for (auto& attr : attributes) {
        if (attr->type == attribute_t::getid()) {
            return attr->as<attribute_t>();
        }
    }
    return nullptr;
}

void trval_stunMessage(stunMessage_view msg);