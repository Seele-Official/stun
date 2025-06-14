#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <format>
#include <ostream>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "coro/async.h"


class logger {
private:
    std::atomic<bool> enabled;
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

    auto with_loc(std::source_location loc = std::source_location::current()) {
        return [loc, this]<typename... args_t>(const std::format_string<args_t...>& fmt, args_t&&... args) {
            if (!enabled) return;
            std::lock_guard lock(mutex);
            std::format_to(
                std::ostreambuf_iterator{*output}, 
                "{}:{}:{}\n",
                loc.file_name(),
                loc.line(),
                loc.column()
            );
            std::format_to(
                std::ostreambuf_iterator{*output}, 
                fmt,
                std::forward<args_t>(args)...
            );
        };
    }


    auto async_with_loc(std::source_location loc = std::source_location::current()) {
        return [loc, this]<typename... args_t>(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args) {
            if (!enabled) return;
            auto&& func = this->with_loc(loc);
            coro::async(
                std::forward<decltype(func)>(func),
                fmt,
                std::forward<args_t>(args)...
            );
        };

    }



};

#define LOGGER logger::instance()

#define LOG LOGGER.with_loc()

#define ASYNC_LOG LOGGER.async_with_loc() 

