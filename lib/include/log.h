#pragma once
#include <iostream>
#include <fstream>
#include <mutex>
#include <format>
#include <ostream>
#include <source_location>
#include <string_view>
#include <print>
#include <functional>
#include <utility>
#include <chrono>
#include <atomic>
#include "coro/async.h"
#include "meta.h"
namespace seele::log{    
    enum class level {
        error,
        warn,
        info,
        debug,
        trace
    };
    class logger_impl {
    private:
        std::atomic<bool> enabled;
        std::mutex mutex;    
        std::ostream* output;        
    
        inline explicit logger_impl() : enabled{false}, output{&std::cout} {}

        inline ~logger_impl() {
            if (output != &std::cout) {
                delete output;
            }
        }
        template<typename... args_t>
        void log(
            level level,
            std::source_location loc,
            std::chrono::system_clock::time_point time,
            std::format_string<args_t...> fmt,
            args_t&&... args
        );

    public:
        friend class async_agent;
        friend class sync_agent;
        logger_impl(const logger_impl&) = delete;
        logger_impl& operator=(const logger_impl&) = delete;
        logger_impl(logger_impl&&) = delete;
        logger_impl& operator=(logger_impl&&) = delete;


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
            this->output = new std::ofstream{filename.data(), std::ios::app};
        }

    };

    inline auto& logger() {
        return logger_impl::get_instance();
    }

    template<typename... args_t>
    void logger_impl::log(level lvl, 
        std::source_location loc, std::chrono::system_clock::time_point time,
        std::format_string<args_t...> fmt, args_t&&... args){

        static auto logLevels = seele::meta::enum_name_table<level>();
        if (!enabled) return;
        std::lock_guard lock(mutex);
        std::print(
            *this->output,
            "[{}] {} {}:{}:{} from '{}'\n{}",
            logLevels[static_cast<int>(lvl)], time, loc.file_name(), loc.line(), loc.column(), loc.function_name(),
            std::format(
                fmt, std::forward<args_t>(args)...
            )
        );
    }

    struct sync_agent{
        std::source_location loc;
        std::chrono::system_clock::time_point now;
        
        template<typename... args_t>
        void log(level lvl, std::format_string<args_t...> fmt, args_t&&... args){
            if (!logger().enabled) return;
            logger().log(
                lvl, loc, now, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void error(std::format_string<args_t...> fmt, args_t&&... args){
            this->log(
                level::error, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void warn(std::format_string<args_t...> fmt, args_t&&... args){
            this->log(
                level::warn, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void info(std::format_string<args_t...> fmt, args_t&&... args){
            this->log(
                level::info, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void debug(std::format_string<args_t...> fmt, args_t&&... args){
            this->log(
                level::debug, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void trace(std::format_string<args_t...> fmt, args_t&&... args){
            this->log(
                level::trace, fmt, std::forward<args_t>(args)...
            );
        }
    };

    struct async_agent : sync_agent{
        template<typename... args_t>
        void log(level lvl, std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            if (!logger().enabled) return;
            seele::coro::async(
                &logger_impl::log<std::decay_t<args_t>&...>,
                std::ref(logger()), lvl, loc, now, fmt, std::forward<args_t>(args)...
            );
        }
        template<typename... args_t>
        void error(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            this->log(
                level::error, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void warn(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            this->log(
                level::warn, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void info(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            this->log(
                level::info, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void debug(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            this->log(
                level::debug, fmt, std::forward<args_t>(args)...
            );
        }

        template<typename... args_t>
        void trace(std::format_string<std::decay_t<args_t>&...> fmt, args_t&&... args){
            this->log(
                level::trace, fmt, std::forward<args_t>(args)...
            );
        }

    };
    
    inline async_agent async(
        std::source_location loc = std::source_location::current(), 
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) 
    { return async_agent{loc, now}; }

    inline sync_agent sync(
        std::source_location loc = std::source_location::current(), 
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) 
    { return sync_agent{loc, now}; }

}

