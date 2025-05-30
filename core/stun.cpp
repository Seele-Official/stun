#include "stun.h"
#include "stunAttribute.h"
#include "random.h"
#include <format>

constexpr std::array<uint32_t, 256> crc32_table() {
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

uint32_t fingerPrint::crc32_bitwise(const uint8_t* data, size_t len) {

    static constexpr auto CRC32_TABLE = crc32_table();

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc = (crc >> 8) ^ CRC32_TABLE[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}







stunMessage_view::stunMessage_view(const uint8_t* p) : header(reinterpret_cast<const stunHeader*>(p)) {
    uint8_t* ptr = const_cast<uint8_t*>(p) + sizeof(stunHeader);
    uint8_t* endptr = ptr + my_ntohs(header->length);

    while (ptr < endptr) {
        stun_attr* attribute = reinterpret_cast<stun_attr*>(ptr);
        attributes.emplace_back(attribute);

        auto len = sizeof(stun_attr) + my_ntohs(attribute->length);
        if (len % 4 != 0) len += 4 - len % 4;
        ptr += len;
    }
}


stunMessage::stunMessage(uint16_t type) {
    this->data = new uint8_t[548]();
    this->header = reinterpret_cast<stunHeader*>(data);
    this->header->type = type;
    this->header->length = 0;
    this->header->magicCookie = stun::MAGIC_COOKIE;
    this->endptr = data + sizeof(stunHeader);

    for (size_t i = 0; i < 12; i++) {
        header->txn_id.data[i] = random<uint8_t>(0, 255);
    }
}

stunMessage::stunMessage(const uint8_t* p) {
    const stunHeader* h = reinterpret_cast<const stunHeader*>(p);
    size_t size = sizeof(stunHeader) + my_ntohs(h->length);
    this->data = new uint8_t[size];
    std::memcpy(this->data, h, size);
    this->header = reinterpret_cast<stunHeader*>(this->data);
    this->endptr = this->data + size;

    uint8_t* ptr = this->data + sizeof(stunHeader);

    while (ptr < this->endptr) {
        stun_attr* attribute = reinterpret_cast<stun_attr*>(ptr);
        attributes.emplace_back(attribute);

        auto len = sizeof(stun_attr) + my_ntohs(attribute->length);
        if (len % 4 != 0) len += 4 - len % 4;
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
    if (data) delete[] data;
}

stunMessage& stunMessage::operator=(stunMessage&& other) noexcept {
    if (this != &other) {
        if (data) delete[] data;
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

bool stunMessage::is_valid(uint8_t* p) {
    if (*p & 0b11000000) return false;

    stunHeader* h = reinterpret_cast<stunHeader*>(p);
    if (h->magicCookie != stun::MAGIC_COOKIE) return false;
    if (my_ntohs(h->length) + sizeof(stunHeader) > 548) return false;
    if (my_ntohs(h->length) % 4 != 0) return false;

    return true;
}
std::string stunMessage::toString() const {
    std::string str;
    str += std::format("STUN MESSAGE: type: {}, length: {}, magic_cookie: {}, txn_id: {}\n", 
        tohex(my_ntohs(this->header->type)), 
        my_ntohs(this->header->length), 
        my_ntohl(this->header->magicCookie), 
        std::string(this->get_txn_id())
    );
    for (auto& attr : this->get_attrs()){
        switch (attr->type){
            case stun::attribute::MAPPED_ADDRESS:
                {
                    auto mappedaddress = attr->as<ipv4_mappedAddress>();
                    str += std::format("   MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->address), my_ntohs(mappedaddress->port));
                }
                break;
            case stun::attribute::XOR_MAPPED_ADDRESS:
                {
                    auto mappedaddress = attr->as<ipv4_xor_mappedAddress>();
                    str += std::format("   XOR_MAPPED_ADDRESS: {}:{}\n", my_inet_ntoa(mappedaddress->x_address ^ stun::MAGIC_COOKIE), my_ntohs(mappedaddress->x_port ^ stun::MAGIC_COOKIE));
                }
                break;
            case stun::attribute::RESPONSE_ORIGIN:
                {
                    auto responseorigin = attr->as<ipv4_responseOrigin>();
                    str += std::format("   RESPONSE_ORIGIN: {}:{}\n", my_inet_ntoa(responseorigin->address), my_ntohs(responseorigin->port));
                }
                break;
            case stun::attribute::OTHER_ADDRESS:
                {
                    auto otherAddress = attr->as<ipv4_otherAddress>();
                    str += std::format("   OTHER_ADDRESS: {}:{}\n", my_inet_ntoa(otherAddress->address), my_ntohs(otherAddress->port));
                }
                break;
            case stun::attribute::SOFTWARE:
                {
                    auto software = attr->as<softWare>();
                    str += std::format("   SOFTWARE: {}\n", std::string_view(software->value, my_ntohs(attr->length)));
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
                    str += std::format("   ERROR_CODE: code: {}, reason: {}\n", errorcode->error_code, std::string_view(errorcode->error_reason, my_ntohs(attr->length) - 4));
                }
                break;
            case stun::attribute::RESPONSE_PORT:
                {
                    auto responseport = attr->as<responsePort>();
                    str += std::format("   RESPONSE_PORT: {}\n", my_ntohs(responseport->port));
                }
                break;
            default:
                {
                    str += std::format("   UNKNOWN ATTRIBUTE: type: {}, length: {}, value: {}\n", tohex(attr->type), my_ntohs(attr->length), tohex(attr->get_value_ptr(), my_ntohs(attr->length)));

                }

        
        }
    }
    str += "\n";
    return str;
}

