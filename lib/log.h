#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "async.h"
class logger {
private:
    bool enabled;
    std::mutex mutex;    
    std::ostream* output;
public:
    inline explicit logger() : enabled{false}, output{&std::cout} {}

    inline ~logger() {
        if (output != &std::cout) {
            delete output;
        }
    }

    inline static logger& instance() {
        static logger inst;
        return inst;
    }

    inline void set_enable(bool enabled) {
        this->enabled = enabled;
    }
    inline void set_output(std::ostream& os) {
        this->output = &os;
    }
    inline void set_output_file(std::string_view filename) {
        this->output = new std::ofstream{filename.data()};
    }



    template<typename T>
    using rv_refer_t = std::remove_reference_t<T>;

    template<typename... args_t>
    using fmt_lrefer_t = std::format_string<rv_refer_t<args_t>&...>;

    template<typename... args_t>
    void async_log(fmt_lrefer_t<args_t...> fmt, args_t&&... args) {
        if (!enabled) return;

        coro::async(&logger::log<rv_refer_t<args_t>&...>,
            this,
            std::move(fmt), 
            std::forward<args_t>(args)...
        );

    }

    void async_log(const std::string& str) {
        if (!enabled) return;

        coro::async(
            static_cast<void(logger::*)(const std::string&)>(&logger::log), 
            this, str
        );
    }

    void async_log(std::string&& str) {
        if (!enabled) return;

        coro::async(
            static_cast<void(logger::*)(const std::string&)>(&logger::log), 
            this, std::move(str)
        );
    }

    template<typename... args_t>
    void log(const std::format_string<args_t...>& fmt, args_t&&... args) {
        if (!enabled) return;
        std::lock_guard lock(mutex);
        std::format_to(
            std::ostreambuf_iterator{*output}, 
            fmt, 
            std::forward<args_t>(args)...
        );
    }

    void log(const std::string& str) {
        if (!enabled) return;
        std::lock_guard lock(mutex);
        *output << str;
    }

    void log(std::string&& str) {
        if (!enabled) return;
        std::lock_guard lock(mutex);
        *output << str;
    }

    void log(const std::string_view& str) {
        if (!enabled) return;
        std::lock_guard lock(mutex);
        *output << str;
    }

    template<typename T>
    auto& operator<<(T&& value) {
        if (!enabled) return *this;
        std::lock_guard lock(mutex);
        *output << value;
        return *this;
    } 


};


#define LOG logger::instance()


