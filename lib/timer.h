#pragma once
#include "log.h"
#include "net_core.h"
#include <chrono>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include <type_traits>
template <typename T, template <typename...> class Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename T, template <typename...> class Template>
constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

using std::chrono_literals::operator""ms;


#define TIMER timer::get_instance()
class delay_task;
class timer {
private:
    std::jthread thread;
    std::condition_variable cv;
    std::mutex m;

    struct task{
        std::coroutine_handle<> handle;

        inline void run(){
            handle.resume();
        }

        inline task(std::coroutine_handle<> handle) : handle{handle} {}
    };

    std::map<std::chrono::steady_clock::time_point, task> tasks;
    void worker(std::stop_token st);

public:


    static inline timer& get_instance(){
        static timer instance{};
        return instance;
    }
    template<typename duration_t>
        requires is_specialization_of_v<duration_t, std::chrono::duration>
    inline bool submit(duration_t delay, std::coroutine_handle<> handle){
        std::lock_guard lock{m};
        tasks.emplace(std::chrono::steady_clock::now() + delay, task{handle});
        LOG.log("submitting task: {}\n", tohex(handle.address()));
        cv.notify_one();
        return true;
    }

    template<typename duration_t>
        requires is_specialization_of_v<duration_t, std::chrono::duration>
    inline bool repeat(duration_t delay, std::coroutine_handle<> handle){
        tasks.emplace(std::chrono::steady_clock::now() + delay, task{handle});
        LOG.log("repeat task: {}\n", tohex(handle.address()));
        cv.notify_one();
        return true;
    }

    bool cancel(std::coroutine_handle<> handle);

    inline explicit timer() : thread{
            [this](std::stop_token st){
                this->worker(st);
            }
        }, tasks{} {}

    inline ~timer(){
        thread.request_stop();
        cv.notify_one();
    }
};



class delay_task{
public:
    struct promise_type{
        auto get_return_object(){
            return delay_task{this};
        }

        auto initial_suspend(){
            return std::suspend_never{};
        }

        auto final_suspend() noexcept{
            LOG.log("final suspend {}\n", tohex(this));
            return std::suspend_never{};
        }
        void unhandled_exception() {  }

        void return_void() {}

    };
private:
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle;

public:
    explicit delay_task(promise_type* p): handle{handle_type::from_promise(*p)}{}

    delay_task(const delay_task& other) = delete;
    delay_task(delay_task&& other){
        handle = other.handle;
        other.handle = nullptr;
    }


    delay_task& operator=(const delay_task& other) = delete;
    delay_task& operator=(delay_task&& other){
        handle = other.handle;
        other.handle = nullptr;
        return *this;
    }

    ~delay_task() = default;

    void cancel(){
        if (handle ? TIMER.cancel(handle) : false){
            handle.destroy();
            LOG.log("destroying task: {}\n", tohex(handle.address()));
        }
    }
};
template<typename duration_t>
    requires is_specialization_of_v<duration_t, std::chrono::duration>
struct delay_awaiter{
    duration_t delay;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        TIMER.submit(delay, handle);
    }

    void await_resume() {}

    inline explicit delay_awaiter(duration_t delay): delay{delay}{}
};
template<typename duration_t>
    requires is_specialization_of_v<duration_t, std::chrono::duration>
struct repeat_awaiter{
    duration_t delay;
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        TIMER.repeat(delay, handle);
    }

    void await_resume() {}
    inline explicit repeat_awaiter(duration_t delay): delay{delay}{}
};

// class repeat_task{
// public:
//     struct promise_type{
//         auto get_return_object(){
//             return repeat_task{this};
//         }

//         auto initial_suspend(){
//             std::cout << "initial suspend\n";
//             return std::suspend_always{};
//         }

//         auto final_suspend() noexcept{
//             return std::suspend_always{};
//         }
//         void unhandled_exception() {  }

//         void return_void() {}

//     };
// private:
//     using handle_type = std::coroutine_handle<promise_type>;
//     handle_type handle;

// public:
//     explicit repeat_task(promise_type* p): handle{handle_type::from_promise(*p)}{
//     }

//     repeat_task(const repeat_task& other) = delete;
//     repeat_task(repeat_task&& other){
//         handle = other.handle;
//         other.handle = nullptr;
//     }


//     repeat_task& operator=(const repeat_task& other) = delete;
//     repeat_task& operator=(repeat_task&& other){
//         handle = other.handle;
//         other.handle = nullptr;
//         return *this;
//     }

//     ~repeat_task() = default;

//     void cancel(){
//         if (handle ? TIMER.cancel(handle) : false){
//             handle.destroy();
//             LOG.log("destroying task: {}\n", tohex(handle.address()));
//         }
//     }
// };
    
    
    
