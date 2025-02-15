#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
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



struct stunAttribute {
    uint16_t type;
    uint16_t length;
    uint8_t value[0];
};




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

    const stunHeader* getHeader() const {
        return header;
    }

    const std::vector<stunAttribute*>& getAttributes() const {
        return attributes;
    }

    const stunAttribute* operator[](size_t index) const {
        return attributes[index];
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
        for (int i = 0; i < 12; i++) {
            this->header->transactionID.data[i] = 0; // TODO: randomize
        }



    }

    ~stunMessage(){
        if (data) delete[] data;
    }
    const stunHeader* getHeader() const {
        return header;
    }
    
    bool empty() const {
        return header == nullptr;
    }

    operator stunMessage_view() const {
        return stunMessage_view{header, attributes};
    }



    template<typename attribute_t>
    requires std::is_base_of_v<stunAttribute, attribute_t> && (sizeof(attribute_t) % 4 == 0)
    bool apppend(attribute_t* attribute){
        if (endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        std::memcpy(endptr, attribute, sizeof(attribute_t));
        endptr += sizeof(attribute_t);
        setLength(endptr - reinterpret_cast<uint8_t*>(header) - sizeof(stunHeader));
        return true;
    }


};