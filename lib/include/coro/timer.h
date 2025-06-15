#pragma once
#include <chrono>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include <map>
#include "log.h"
#include "threadpool.h"
#include "meta.h"
#include "math.h"
namespace seele::coro::timer {

    class delay_task;
    class timer_impl {
    private:
        std::jthread thread;
        std::condition_variable cv;
        std::mutex m;

        struct task{
            std::coroutine_handle<> handle;

            inline void async_run(){
                seele::coro::thread::dispatch(handle);
            }

            inline task(std::coroutine_handle<> handle) : handle{handle} {}
        };

        std::map<std::chrono::steady_clock::time_point, task> tasks;
        void worker(std::stop_token st);

    public:


        static inline timer_impl& get_instance(){
            static timer_impl instance{};
            return instance;
        }
        template<typename duration_t>
            requires seele::meta::is_specialization_of_v<duration_t, std::chrono::duration>
        inline bool submit(duration_t delay, std::coroutine_handle<> handle){
            std::lock_guard lock{m};
            tasks.emplace(std::chrono::steady_clock::now() + delay, task{handle});
            LOG("submitting task: {}\n", math::tohex(handle.address()));
            cv.notify_one();
            return true;
        }

        bool cancel(std::coroutine_handle<> handle);

        inline explicit timer_impl() : thread{
                [this](std::stop_token st){
                    this->worker(st);
                }
            }, tasks{} {}

        inline ~timer_impl(){
            thread.request_stop();
            cv.notify_one();
        }
    };

    inline void cancel(std::coroutine_handle<> handle){
        if (timer_impl::get_instance().cancel(handle)){
            handle.destroy();
            LOG("destroying task: {}\n", math::tohex(handle.address()));
        }
    }

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
                LOG("final suspend {}\n", math::tohex(this));
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
            if (handle){
                seele::coro::timer::cancel(handle);
            }
        }
    };
    template<typename duration_t>
        requires seele::meta::is_specialization_of_v<duration_t, std::chrono::duration>
    inline auto delay(duration_t delay, std::coroutine_handle<> handle) {
        return seele::coro::timer::timer_impl::get_instance().submit(delay, handle);
    }

    template<typename duration_t>
        requires seele::meta::is_specialization_of_v<duration_t, std::chrono::duration>
    struct delay_awaiter{
        duration_t delay;

        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<> handle) {
            seele::coro::timer::delay(delay, handle);
        }

        void await_resume() {}

        inline explicit delay_awaiter(duration_t delay): delay{delay}{}
    };
}


    
    
    
