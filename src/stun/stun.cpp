#include "stun.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include "net.h"
namespace std{
#ifndef HAVE_ALIGNED_ALLOC 

    inline void* aligned_malloc(size_t align, size_t size) {
        if (size % align) size += align - (size % align);
        return _aligned_malloc(size, align);
    }
    inline void aligned_free(void* p) noexcept { _aligned_free(p); }

#endif
#ifdef HAVE_ALIGNED_ALLOC
    inline void aligned_free(void* p) noexcept { std::free(p); }
#endif
}
namespace stun {


    using namespace math;



    message::message(uint16_t type) {
        this->data = (std::byte*) std::aligned_alloc(alignof(stun::header), 548);
        
        
        this->header = new (this->data) stun::header{};
        this->header->type = type;
        this->header->length = 0;
        this->header->magicCookie = stun::MAGIC_COOKIE;
        this->endptr = data + sizeof(stun::header);

        for (size_t i = 0; i < 12; i++) {
            header->txn_id.data[i] = random<uint8_t>(0, 255);
        }
    }

    message::message(const std::byte* p) {
        stun::header tmpHeader;
        std::memcpy(&tmpHeader, p, sizeof(stun::header));
        size_t size = sizeof(stun::header) + ntoh(tmpHeader.length);

        
        this->data = (std::byte*) std::aligned_alloc(alignof(stun::header), 548);
        std::memcpy(this->data, p, size);
        this->header = reinterpret_cast<stun::header*>(this->data);
        this->endptr = this->data + sizeof(stun::header) + ntoh(this->header->length);

        auto ptr = this->data + sizeof(stun::header);

        while (ptr < this->endptr) {
            attr* attribute = reinterpret_cast<attr*>(ptr);
            attributes.emplace_back(attribute);

            auto len = sizeof(attr) + ntoh(attribute->length);
            len = (len + 3) & ~3;
            ptr += len;
        }
    }

    message::message(message&& other) noexcept
        : data{other.data}, header{other.header}, attributes{std::move(other.attributes)}, endptr{other.endptr} {
        other.data = nullptr;
        other.header = nullptr;
        other.endptr = nullptr;
    }

    message::~message() {
        if (data) std::aligned_free(data);
    }

    message& message::operator=(message&& other) noexcept {
        if (this != &other) {
            if (data) std::aligned_free(data);
            data = other.data;
            header = other.header;
            attributes = std::move(other.attributes);
            endptr = other.endptr;

            other.data = nullptr;
            other.header = nullptr;
            other.endptr = nullptr;
        }
        return *this;
    }

    bool message::is_valid(std::byte* p) {
        if (static_cast<uint8_t>(*p) & 0b11000000) return false;

        auto h = reinterpret_cast<stun::header*>(p);
        if (h->magicCookie != stun::MAGIC_COOKIE) return false;
        if (ntoh(h->length) + sizeof(stun::header) > 548) return false;
        if (ntoh(h->length) % 4 != 0) return false;

        return true;
    }
    std::string message::toString() const {
        std::string str;
        str += std::format("STUN MESSAGE: type: {}, length: {}, magic_cookie: {}, txn_id: {}\n", 
            tohex(ntoh(this->header->type)), 
            ntoh(this->header->length), 
            ntoh(this->header->magicCookie), 
            std::string(this->get_txn_id())
        );
        for (auto& attr : this->get_attrs()){
            switch (attr->type){
                case stun::attribute::MAPPED_ADDRESS:
                    {
                        auto mappedaddress = attr->as<ipv4_mappedAddress>();
                        str += std::format("   MAPPED_ADDRESS: {}:{}\n", seele::net::inet_ntoa(mappedaddress->address), ntoh(mappedaddress->port));
                    }
                    break;
                case stun::attribute::XOR_MAPPED_ADDRESS:
                    {
                        auto mappedaddress = attr->as<ipv4_xor_mappedAddress>();
                        str += std::format("   XOR_MAPPED_ADDRESS: {}:{}\n", seele::net::inet_ntoa(mappedaddress->net_x_address ^ stun::MAGIC_COOKIE), ntoh<uint16_t>(mappedaddress->net_x_port ^ stun::MAGIC_COOKIE));
                    }
                    break;
                case stun::attribute::RESPONSE_ORIGIN:
                    {
                        auto responseorigin = attr->as<ipv4_responseOrigin>();
                        str += std::format("   RESPONSE_ORIGIN: {}:{}\n", seele::net::inet_ntoa(responseorigin->address), ntoh(responseorigin->port));
                    }
                    break;
                case stun::attribute::OTHER_ADDRESS:
                    {
                        auto otherAddress = attr->as<ipv4_otherAddress>();
                        str += std::format("   OTHER_ADDRESS: {}:{}\n", seele::net::inet_ntoa(otherAddress->address), ntoh(otherAddress->port));
                    }
                    break;
                case stun::attribute::SOFTWARE:
                    {
                        auto software = attr->as<softWare>();
                        str += std::format("   SOFTWARE: {}\n", std::string_view(software->value, ntoh(attr->length)));
                    }
                    break;
                case stun::attribute::CHANGE_REQUEST:
                    {
                        auto changerequest = attr->as<changeRequest>();
                        str += std::format("   CHANGE_REQUEST: {}\n", tohex(changerequest->flags));
                    }
                    break;
                case stun::attribute::FINGERPRINT:
                    {
                        auto fingerprint = attr->as<fingerPrint>();
                        str += std::format("   FINGERPRINT: {}\n", tohex(fingerprint->crc32));
                    }
                    break;
                case stun::attribute::ERROR_CODE:
                    {
                        auto errorcode = attr->as<errorCode>();
                        str += std::format("   ERROR_CODE: code: {}, reason: {}\n", errorcode->error_code, std::string_view(errorcode->error_reason, ntoh(attr->length) - 4));
                    }
                    break;
                case stun::attribute::RESPONSE_PORT:
                    {
                        auto responseport = attr->as<responsePort>();
                        str += std::format("   RESPONSE_PORT: {}\n", ntoh(responseport->port));
                    }
                    break;
                default:
                    {
                        str += std::format("   UNKNOWN ATTRIBUTE: type: {}, length: {}, value: {}\n", tohex(attr->type), ntoh(attr->length), tohex(attr->get_value_ptr(), ntoh(attr->length)));

                    }

            }
        }
        str += "\n";
        return str;
    }

}