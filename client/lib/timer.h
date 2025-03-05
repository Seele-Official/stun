#pragma once
#include "log.h"
#include "stun.h"
#include <cstdint>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <set>
#include <thread>
#include <condition_variable>
#include <tuple>
#include <type_traits>

#define TIMER Timer::get_instance()

class Timer {
public:
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
                return std::suspend_always{};
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
        delay_task& operator=(const delay_task& other) = delete;


        delay_task(delay_task&& other){
            handle = other.handle;
            other.handle = nullptr;
        }

        delay_task& operator=(delay_task&& other){
            handle = other.handle;
            other.handle = nullptr;
            return *this;
        }



        ~delay_task(){
            if (handle) {
                wait_for_done();
                LOG.async_log("task done and destroyed {} \n", tohex(handle.address()));
                handle.destroy();
            }
        }

        void cancel(){
            if (handle) {       
                if (TIMER.cancel(handle)) {
                LOG.async_log("task cancel and destroyed {} \n", tohex(handle.address()));
                } else {
                    wait_for_done();
                    LOG.async_log("task done and destroyed {} \n", tohex(handle.address()));
                }
                handle.destroy();
                handle = nullptr;
            }

        }

        void wait_for_done() const{
            while (!handle.done()) {
                std::this_thread::yield();
            }
        }
    };

    using promise_type = delay_task::promise_type;


private:
    using time_point = std::chrono::steady_clock::time_point;
    std::jthread thread;
    std::condition_variable cv;
    std::mutex m;


    struct task {
        time_point expirytime;
        std::coroutine_handle<> handle;
        inline bool operator<(const task& other) const {
            return expirytime < other.expirytime;
        }
        inline task(time_point expirytime, std::coroutine_handle<> handle) : expirytime{expirytime}, handle{handle} {}
    };

    std::set<task> tasks;
    void worker(std::stop_token st);

public:


    static inline Timer& get_instance(){
        static Timer instance{};
        return instance;
    }

    bool submit(std::coroutine_handle<> handle, uint64_t delay);


    struct forward2Timer{
        uint64_t delay;
        
        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<> handle) {

            Timer::get_instance().submit(handle, delay);

        }
        
        void await_resume() {}
        forward2Timer(uint64_t delay): delay{delay} {}
    };



    template <typename lambda_t, typename... args_t>
    requires std::invocable<lambda_t, std::remove_reference_t<args_t>&...>
    delay_task schedule(uint64_t delay, lambda_t&& callback, args_t&&... args){
        auto handle = std::forward<lambda_t>(callback);
        std::tuple<std::remove_reference_t<args_t>...> args_tuple{std::forward<args_t>(args)...};
        co_await forward2Timer{delay};
        std::apply(handle, args_tuple);
        co_return;
    }

    bool cancel(std::coroutine_handle<> handle);

    inline explicit Timer() : thread{&Timer::worker, this}, tasks{} {}

    inline ~Timer(){
        thread.request_stop();
        cv.notify_one();
    }
};

using forward2Timer = Timer::forward2Timer;

