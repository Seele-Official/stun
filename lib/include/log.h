#pragma once
#include <iostream>
#include <fstream>
#include <mutex>
#include <format>
#include <ostream>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>
#include "coro/async.h"

namespace seele{
    class logger_impl {
    private:
        std::atomic<bool> enabled;
        std::mutex mutex;    
        std::ostream* output;
    public:
        inline explicit logger_impl() : enabled{false}, output{&std::cout} {}

        inline ~logger_impl() {
            if (output != &std::cout) {
                delete output;
            }
        }

        inline static logger_impl& get_instance() {
            static logger_impl inst;
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

        inline auto with_loc(std::source_location loc = std::source_location::current());

        inline auto async_with_loc(std::source_location loc = std::source_location::current());


    };

    inline auto logger_impl::with_loc(std::source_location loc) {
        return [loc, this]<typename... args_t>(std::format_string<args_t...> fmt, args_t&&... args) {
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


    inline auto logger_impl::async_with_loc(std::source_location loc) {
        return [loc, this]<typename... args_t>(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args) {
            if (!enabled) return;
            auto&& func = this->with_loc(loc);
            seele::coro::async(
                std::forward<decltype(func)>(func),
                fmt, std::forward<args_t>(args)...
            );
        };

    }
}


#define LOGGER seele::logger_impl::get_instance()

#define LOG LOGGER.with_loc()

#define ASYNC_LOG LOGGER.async_with_loc() 

