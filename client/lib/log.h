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
class Logger {
private:
    bool enabled_;
    std::mutex mutex_;    
    std::ostream* output_;
public:
    inline explicit Logger() : enabled_{false}, output_{&std::cout} {}

    inline ~Logger() {
        if (output_ != &std::cout) {
            delete output_;
        }
    }

    inline static Logger& instance() {
        static Logger inst;
        return inst;
    }

    inline void set_enable(bool enabled) {
        enabled_ = enabled;
    }
    inline void set_output(std::ostream& os) {
        output_ = &os;
    }
    inline void set_output_file(std::string_view filename) {
        output_ = new std::ofstream{filename.data()};
    }



    template<typename T>
    using rv_refer_t = std::remove_reference_t<T>;

    template<typename... args_t>
    using fmt_lrefer_t = std::format_string<rv_refer_t<args_t>&...>;

    template<typename... args_t>
    void async_log(fmt_lrefer_t<args_t...> fmt, args_t&&... args) {
        if (!enabled_) return;

        async(&Logger::log<rv_refer_t<args_t>&...>,
            this,
            std::move(fmt), 
            std::forward<args_t>(args)...
        );

    }

    void async_log(const std::string& str) {
        if (!enabled_) return;

        async(
            static_cast<void(Logger::*)(const std::string&)>(&Logger::log), 
            this, str
        );
    }

    void async_log(std::string&& str) {
        if (!enabled_) return;

        async(
            static_cast<void(Logger::*)(const std::string&)>(&Logger::log), 
            this, std::move(str)
        );
    }

    template<typename... args_t>
    void log(const std::format_string<args_t...>& fmt, args_t&&... args) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        std::format_to(
            std::ostreambuf_iterator{*output_}, 
            fmt, 
            std::forward<args_t>(args)...
        );
    }

    void log(const std::string& str) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        *output_ << str;
    }

    void log(std::string&& str) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        *output_ << str;
    }

    void log(const std::string_view& str) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        *output_ << str;
    }

    template<typename T>
    auto& operator<<(T&& value) {
        if (!enabled_) return *this;
        std::lock_guard lock(mutex_);
        *output_ << value;
        return *this;
    } 


};


#define LOG Logger::instance()


