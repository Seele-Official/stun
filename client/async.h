#pragma once
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <thread>
#include <utility>
#include <type_traits>
#include <functional>
#include <condition_variable>
#include "threadpool.h"
template <typename return_t>
class lazy_task{
public:
    struct promise_type{
        return_t value;

        auto get_return_object(){
            return lazy_task{this};
        }

        auto initial_suspend(){
            return std::suspend_never{};
        }

        auto final_suspend() noexcept{
            return std::suspend_always{};
        }
        void unhandled_exception() {  }


        void return_value(const return_t& v){
            value = v;
        }

        void return_value(return_t&& v){
            value = std::move(v);
        }

    };
private:
    using handle_type = std::coroutine_handle<promise_type>;

    handle_type handle;


public:

    explicit lazy_task(promise_type* p): handle{handle_type::from_promise(*p)}{}
    lazy_task(lazy_task&& other) = delete;
    lazy_task(const lazy_task& other) = delete;
    lazy_task& operator=(lazy_task&& other) = delete;
    lazy_task& operator=(const lazy_task& other) = delete;

    ~lazy_task(){
        while (!handle.done())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        handle.destroy();
    }

    bool done() const{
        return handle.done();
    }

    return_t& get_return_value(size_t sleep_time = 100){
        while (!handle.done()){
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        return handle.promise().value;
    }

};

template <>
class lazy_task<void>{
public:
    struct promise_type{
        auto get_return_object(){
            return lazy_task{this};
        }

        auto initial_suspend(){
            return std::suspend_never{};
        }

        auto final_suspend() noexcept{
            return std::suspend_always{};
        }
        void unhandled_exception() {  }

        void return_void(){}
    };
private:
    using handle_type = std::coroutine_handle<promise_type>;

    handle_type handle;


public:

    explicit lazy_task(promise_type* p): handle{handle_type::from_promise(*p)}{}
    lazy_task(lazy_task&& other) = delete;
    lazy_task(const lazy_task& other) = delete;
    lazy_task& operator=(lazy_task&& other) = delete;
    lazy_task& operator=(const lazy_task& other) = delete;

    ~lazy_task(){
        while (!handle.done())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        handle.destroy();
    }


    bool done() const{
        return handle.done();
    }
};



template <typename lambda_t, typename... args_t>
requires std::invocable<lambda_t, args_t...>
lazy_task<std::invoke_result_t<lambda_t, args_t...>> async(lambda_t lambda, args_t&&... args){ 

    co_await forward2threadpool{};

    if constexpr (std::is_same_v<void, std::invoke_result_t<lambda_t, args_t...>>){
        co_return lambda(std::forward<args_t>(args)...);
    } else {
        co_return std::move(lambda(std::forward<args_t>(args)...));
    }
}


class Timer{
private:
    using callback_t = std::function<void()>;
    
    using time_point = std::chrono::steady_clock::time_point;
    std::jthread thread;

    std::condition_variable cv;
    std::mutex m;

    struct task{
        uint64_t id;
        time_point expirytime;
        callback_t callback;

        auto operator <(const task& other) const{
            return expirytime < other.expirytime;
        }

        task(uint64_t id, time_point expirytime, callback_t callback): id{id}, expirytime{expirytime}, callback{callback}{}
    };

    std::set<task> tasks;


    void worker(std::stop_token st){
        while(!st.stop_requested()){
            std::unique_lock lock{m};

            if(tasks.empty()){
                cv.wait(lock, [&]{
                    return !tasks.empty() || st.stop_requested();
                });
                continue;
            }

            auto t = tasks.begin();
            if (t->expirytime > std::chrono::steady_clock::now()){
                cv.wait_until(lock, t->expirytime, [&]{
                    return tasks.begin()->expirytime <= std::chrono::steady_clock::now() || st.stop_requested();
                });

                continue;
            }

            tasks.erase(t);
            lock.unlock();

            t->callback();


        }
    }

public:
    uint64_t schedule(callback_t callback, uint64_t delay){
        static uint64_t id = 0;
        std::lock_guard lock{m};
        tasks.emplace(id, std::chrono::steady_clock::now() + std::chrono::milliseconds(delay), callback);
        cv.notify_one();
        return id++;
    }

    bool cancel(uint64_t id){
        std::lock_guard lock{m};

        auto it = std::find_if(tasks.begin(), tasks.end(), [&](const task& t){
            return t.id == id;
        });

        if (it == tasks.end()){
            return false;
        }

        tasks.erase(it);
        return true;
    }

    explicit Timer() : thread{&Timer::worker, this} {}

    ~Timer(){
        thread.request_stop();
        cv.notify_one();
    }


};