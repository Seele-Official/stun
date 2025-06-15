#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <expected>
#include "math.h"



namespace stun {
    struct txn_id_t {
        uint8_t data[12];

        txn_id_t() = default;

        auto operator<=>(const txn_id_t&) const = default;
        txn_id_t(const txn_id_t&) = default;
        operator std::string() const {
            return math::tohex(*this);
        }
    };


    struct header {
        uint16_t type;
        uint16_t length;
        uint32_t magicCookie;
        txn_id_t txn_id;
    };


    using namespace math;
    namespace msg_type {
        constexpr uint16_t REQUEST = hton<uint16_t>(0x0000);
        constexpr uint16_t INDICATION = hton<uint16_t>(0x0010);
        constexpr uint16_t SUCCESS_RESPONSE = hton<uint16_t>(0x0100);
        constexpr uint16_t ERROR_RESPONSE = hton<uint16_t>(0x0110);
    }
    namespace msg_method{
        constexpr uint16_t BINDING = hton<uint16_t>(0x0001);
    }

    constexpr uint32_t MAGIC_COOKIE = hton<uint32_t>(0x2112A442);

}

#include "stunAttribute.inl"

namespace stun {
    class message {
    private:
        std::byte* data;
        stun::header* header;
        std::vector<attr*> attributes;
        std::byte* endptr;

        inline void setLength(uint16_t length) { header->length = math::hton<uint16_t>(length); }

    public:
        inline explicit message() : data{nullptr}, header{nullptr}, endptr{nullptr} {}
        explicit message(uint16_t type);
        explicit message(const std::byte* p);
        
        message(const message&) = delete;
        message(message&& other) noexcept;
        ~message();

        message& operator=(const message&) = delete;
        message& operator=(message&& other) noexcept;

        inline const txn_id_t& get_txn_id() const { return header->txn_id; }
        inline const std::vector<attr*>& get_attrs() const { return attributes; }
        inline const std::byte* data_ptr() const { return data; }
        inline size_t size() const { return endptr - data; }
        inline bool empty() const { return header == nullptr; }


        template <is_stunAttribute attribute_t>
        bool append(attribute_t* attribute);

        template <is_stunAttribute attribute_t, typename... args_t>
        bool emplace(args_t&&... args);

        template <is_stunAttribute attribute_t>
        attribute_t* find_one();

        template <is_stunAttribute... attribute_t>
        std::tuple<attribute_t*...> find();

        inline uint16_t get_type() const { return header->type; }
        std::string toString() const;

        static bool is_valid(std::byte* p);
    };

    template <is_stunAttribute attribute_t>
    bool message::append(attribute_t* attribute) {
        if (this->endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        std::memcpy(endptr, attribute, sizeof(attribute_t));
        this->attributes.emplace_back(reinterpret_cast<attribute_t*>(endptr));
        this->endptr += sizeof(attribute_t);
        this->setLength(endptr - data - sizeof(stun::header));
        return true;
    }

    template <is_stunAttribute attribute_t, typename... args_t>
    bool message::emplace(args_t&&... args) {
        if (this->endptr + sizeof(attribute_t) > data + 548) {
            return false;
        }

        this->attributes.emplace_back(new (this->endptr) attribute_t(std::forward<args_t>(args)...));
        this->endptr += sizeof(attribute_t);
        this->setLength(endptr - data - sizeof(stun::header));
        return true;
    }

    template <is_stunAttribute attribute_t>
    attribute_t* message::find_one() {
        for (auto& attr : attributes) {
            if (attr->type == attribute_t::getid()) {
                return attr->as<attribute_t>();
            }
        }
        return nullptr;
    }



    template <typename... args_t>
    struct check_unique;
    template <typename T>
    struct check_unique<T> {
        static constexpr bool value = true;
    };
    template <typename T, typename... args_t>
    struct check_unique<T, args_t...> {
        static constexpr bool value = (!std::is_same_v<T, args_t> && ...) && check_unique<args_t...>::value;
    };
    template <typename... args_t>
    inline constexpr bool check_unique_v = check_unique<args_t...>::value;

    template <is_stunAttribute... attribute_t>
    std::tuple<attribute_t*...>  message::find() {
        static_assert(check_unique_v<attribute_t...>, "Attributes must be unique");
        std::tuple<attribute_t*...> res{};

        for (auto& attr : attributes) {

            (   
                ((std::get<attribute_t*>(res) == nullptr) && 
                (attr->type == attribute_t::getid()) && 
                (std::get<attribute_t*>(res) = attr->as<attribute_t>())) 
            || ...);

            if (((std::get<attribute_t*>(res) != nullptr) && ...)){
                break;
            }
            
        }
        return res;
    }

}