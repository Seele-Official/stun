#include "stun.h"
#include "net_core.h"
#include "stunAttribute.h"
#include "math.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>




using namespace math;



stunMessage::stunMessage(uint16_t type) {
    this->data = (std::byte*) std::aligned_alloc(alignof(stunHeader), 548);
    
    
    this->header = new (this->data) stunHeader{};
    this->header->type = type;
    this->header->length = 0;
    this->header->magicCookie = stun::MAGIC_COOKIE;
    this->endptr = data + sizeof(stunHeader);

    for (size_t i = 0; i < 12; i++) {
        header->txn_id.data[i] = random<uint8_t>(0, 255);
    }
}

stunMessage::stunMessage(const std::byte* p) {
    stunHeader tmpHeader;
    std::memcpy(&tmpHeader, p, sizeof(stunHeader));
    size_t size = sizeof(stunHeader) + ntoh(tmpHeader.length);

    
    this->data = (std::byte*) std::aligned_alloc(alignof(stunHeader), 548);
    std::memcpy(this->data, p, size);
    this->header = std::launder(reinterpret_cast<stunHeader*>(this->data));
    this->endptr = this->data + sizeof(stunHeader) + ntoh(this->header->length);

    auto ptr = this->data + sizeof(stunHeader);

    while (ptr < this->endptr) {
        stun_attr* attribute = std::launder(reinterpret_cast<stun_attr*>(ptr));
        attributes.emplace_back(attribute);

        auto len = sizeof(stun_attr) + ntoh(attribute->length);
        len = (len + 3) & ~3;
        ptr += len;
    }
}

stunMessage::stunMessage(stunMessage&& other) noexcept
    : data{other.data}, header{other.header}, attributes{std::move(other.attributes)}, endptr{other.endptr} {
    other.data = nullptr;
    other.header = nullptr;
    other.endptr = nullptr;
}

stunMessage::~stunMessage() {
    if (data) std::free(data);
}

stunMessage& stunMessage::operator=(stunMessage&& other) noexcept {
    if (this != &other) {
        if (data) std::free(data);
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

bool stunMessage::is_valid(std::byte* p) {
    if (static_cast<uint8_t>(*p) & 0b11000000) return false;

    stunHeader* h = reinterpret_cast<stunHeader*>(p);
    if (h->magicCookie != stun::MAGIC_COOKIE) return false;
    if (ntoh(h->length) + sizeof(stunHeader) > 548) return false;
    if (ntoh(h->length) % 4 != 0) return false;

    return true;
}
std::string stunMessage::toString() const {
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
                    str += std::format("   MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->address), ntoh(mappedaddress->port));
                }
                break;
            case stun::attribute::XOR_MAPPED_ADDRESS:
                {
                    auto mappedaddress = attr->as<ipv4_xor_mappedAddress>();
                    str += std::format("   XOR_MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->x_address ^ stun::MAGIC_COOKIE), ntoh(mappedaddress->x_port ^ stun::MAGIC_COOKIE));
                }
                break;
            case stun::attribute::RESPONSE_ORIGIN:
                {
                    auto responseorigin = attr->as<ipv4_responseOrigin>();
                    str += std::format("   RESPONSE_ORIGIN: {}:{}\n", my_inet_ntoa(responseorigin->address), ntoh(responseorigin->port));
                }
                break;
            case stun::attribute::OTHER_ADDRESS:
                {
                    auto otherAddress = attr->as<ipv4_otherAddress>();
                    str += std::format("   OTHER_ADDRESS: {}:{}\n", my_inet_ntoa(otherAddress->address), ntoh(otherAddress->port));
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

