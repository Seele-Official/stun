#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <format>
#include <ostream>
#include <string_view>

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



    template<typename... Args>
    void log(std::format_string<Args...> fmt, Args&&... args) {
        if (!enabled_) return;
        std::lock_guard lock(mutex_);
        std::format_to(std::ostreambuf_iterator{*output_}, fmt, std::forward<Args>(args)...);
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


