#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <format>
#include <span>
#include "random.h"
#include "log.h"
#define MY_LITTLE_ENDIAN 0
#define MY_BIG_ENDIAN 1
constexpr uint32_t my_stoi(std::string_view str){
    uint32_t num = 0;
    for (auto c : str){
        if (c < '0' || c > '9') return num;
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


constexpr uint32_t my_inet_addr(std::string_view ip) {
    uint32_t addr = 0;
    for (size_t i = 0; i < 4; i++) {
        addr <<= 8;
        addr |= my_stoi(ip.substr(0, ip.find('.')));
        ip.remove_prefix(ip.find('.') + 1);
    }
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


constexpr uint32_t my_inet_addr(std::string_view ip) {
    uint32_t addr = 0;
    for (size_t i = 0; i < 4; i++) {
        addr <<= 8;
        addr |= my_stoi(ip.substr(0, ip.find('.')));
        ip.remove_prefix(ip.find('.') + 1);
    }
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

#endif
struct ipv4info{
    uint32_t net_address;
    uint16_t net_port;
    explicit ipv4info() : net_address{}, net_port{} {}
    explicit ipv4info(uint32_t net_address, uint16_t net_port) : net_address{net_address}, net_port{net_port} {}
    explicit ipv4info(std::string_view ip, uint16_t port) : net_address{my_inet_addr(ip.data())}, net_port{my_htons(port)} {}

    auto operator<=>(const ipv4info&) const = default;

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


struct stunMessage_view {
private:
    const stunHeader* header;

    std::vector<stunAttribute*> attributes;
public:
    explicit stunMessage_view(const uint8_t* p) : header(reinterpret_cast<const stunHeader*>(p)){


        uint8_t* ptr = const_cast<uint8_t*>(p) + sizeof(stunHeader);
        uint8_t* endptr = ptr + my_ntohs(header->length);


        while (ptr < endptr) {
            stunAttribute* attribute = reinterpret_cast<stunAttribute*>(ptr);
            attributes.emplace_back(attribute);

            auto len = sizeof(stunAttribute) + my_ntohs(attribute->length);
            if (len % 4 != 0) len += 4 - len % 4;
            ptr += len;
        }
        
    }

    explicit stunMessage_view(stunHeader* header, std::vector<stunAttribute*> attributes) : header(header), attributes(attributes) {} 

    const stunHeader* getHeader() const {
        return header;
    }

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
        return my_ntohs(header->length) + sizeof(stunHeader);
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
        std::memset(data, 0, 548);
        this->header = reinterpret_cast<stunHeader*>(data);

        this->header->type = type;
        this->header->length = 0;
        this->header->magicCookie = stun::MAGIC_COOKIE;
        this->endptr = data + sizeof(stunHeader);


        for (size_t i = 0; i < 12; i++){
            header->transactionID.data[i] = random<uint8_t>(0, 255);
        }

    }

    explicit stunMessage(const uint8_t* p) {
        const stunHeader* h = reinterpret_cast<const stunHeader*>(p);


        size_t size = sizeof(stunHeader) + my_ntohs(h->length);
        this->data = new uint8_t[size];
        std::memcpy(this->data, h, size);
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


    operator stunMessage_view() const {
        return stunMessage_view{header, attributes};
    }



    template<is_stunAttribute attribute_t>
    bool apppend(attribute_t* attribute){
        if (this->endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }
        
        std::memcpy(endptr, attribute, sizeof(attribute_t));
        this->attributes.emplace_back(reinterpret_cast<stunAttribute*>(this->endptr));
        this->endptr += sizeof(attribute_t);
        this->setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
        return true;
    }

    template<is_stunAttribute attribute_t, typename... args_t>
    bool emplace(args_t&&... args){
        if (this->endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        new(this->endptr) attribute_t(std::forward<args_t>(args)...);
        this->attributes.emplace_back(reinterpret_cast<stunAttribute*>(this->endptr));
        this->endptr += sizeof(attribute_t);
        this->setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
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

    static bool isValid(uint8_t* p){
        if (*p & 0b11000000) return false;

        stunHeader* h = reinterpret_cast<stunHeader*>(p);
        if (h->magicCookie != stun::MAGIC_COOKIE) return false;
        if (my_ntohs(h->length) + sizeof(stunHeader) > 548) return false;
        if (my_ntohs(h->length) % 4 != 0) return false;

        fingerPrint* f = reinterpret_cast<fingerPrint*>(p + sizeof(stunHeader) + my_ntohs(h->length) - sizeof(fingerPrint));
        if (f->type == stun::attribute::FINGERPRINT) {

            uint16_t checklength = my_ntohs(h->length) + sizeof(stunHeader) - sizeof(fingerPrint);
            uint32_t crc = my_htonl(
                fingerPrint::crc32_bitwise(
                    p, 
                    checklength
                ) ^ 0x5354554e
            );

            
            if (crc != f->crc32) {
                LOG.log("CRC32: {} != {}\n", tohex(crc), tohex(f->crc32));
                return false;
            }
        }
        return true;
    }

};


inline void trval_stunMessage(stunMessage_view msg){
    LOG.log("STUN MESSAGE: type: {}, length: {}, magicCookie: {}, transactionID: {}\n", tohex(my_ntohs(msg.getHeader()->type)), my_ntohs(msg.getHeader()->length), my_ntohl(msg.getHeader()->magicCookie), std::string(msg.getTransactionID()));
    for (auto& attr : msg.getAttributes()){
        switch (attr->type){
            case stun::attribute::MAPPED_ADDRESS:
                {
                    auto mappedaddress = attr->as<ipv4_mappedAddress>();
                    LOG.log("   MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->address), my_ntohs(mappedaddress->port));
                }
                break;
            case stun::attribute::XOR_MAPPED_ADDRESS:
                {
                    auto mappedaddress = attr->as<ipv4_xor_mappedAddress>();
                    LOG.log("   XOR_MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->x_address ^ stun::MAGIC_COOKIE), my_ntohs(mappedaddress->x_port ^ stun::MAGIC_COOKIE));
                }
                break;
            case stun::attribute::RESPONSE_ORIGIN:
                {
                    auto responseorigin = attr->as<ipv4_responseOrigin>();
                    LOG.log("   RESPONSE_ORIGIN: {}:{}\n", my_inet_ntoa(responseorigin->address), my_ntohs(responseorigin->port));
                }
                break;
            case stun::attribute::OTHER_ADDRESS:
                {
                    auto otherAddress = attr->as<ipv4_otherAddress>();
                    LOG.log("   OTHER_ADDRESS: {}:{}\n", my_inet_ntoa(otherAddress->address), my_ntohs(otherAddress->port));
                }
                break;
            case stun::attribute::SOFTWARE:
                {
                    auto software = attr->as<softWare>();
                    LOG.log("   SOFTWARE: {}\n", std::string_view(software->value, my_ntohs(attr->length)));
                }
                break;
            case stun::attribute::CHANGE_REQUEST:
                {
                    auto changerequest = attr->as<changeRequest>();
                    LOG.log("   CHANGE_REQUEST: {}\n", tohex(changerequest->flags));
                }
                break;
            case stun::attribute::FINGERPRINT:
                {
                    auto fingerprint = attr->as<fingerPrint>();
                    LOG.log("   FINGERPRINT: {}\n", tohex(fingerprint->crc32));
                }
                break;
            default:
                {
                    LOG.log("   UNKNOWN ATTRIBUTE: {}\n", tohex(attr->type));
                }

        
        }
    }
    LOG << "\n";
}
