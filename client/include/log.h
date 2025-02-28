#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <format>
#include <ostream>
#include <string_view>
#include <utility>
#include "async.h"
class Logger {
private:
    bool enabled_;
    std::mutex mutex_;    
    std::ostream* output_;
public:
    explicit Logger() : enabled_{false}, output_{&std::cout} {}
    ~Logger() {
        if (output_ != &std::cout) {
            delete output_;
        }
    }

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_enable(bool enabled) {
        enabled_ = enabled;
    }
    void set_output(std::ostream& os) {
        output_ = &os;
    }
    void set_output_file(std::string_view filename) {
        output_ = new std::ofstream{filename.data()};
    }



    template<typename... args_t>
    void async_log(std::format_string<args_t...> fmt, args_t&&... args) {
        if (!enabled_) return;
        async([](Logger* l, std::format_string<args_t...> fmt, args_t... args) -> void {
            std::lock_guard<std::mutex> lock{l->mutex_};

            std::format_to(std::ostreambuf_iterator{*l->output_}, fmt, std::forward<args_t>(args)...);

        }, this, std::move(fmt), std::forward<args_t>(args)...);
    }

    template<typename... args_t>
    void log(std::format_string<args_t...> fmt, args_t&&... args) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        std::format_to(std::ostreambuf_iterator{*output_}, fmt, std::forward<args_t>(args)...);
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


