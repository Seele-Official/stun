#pragma once
#include <atomic>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
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
            std::this_thread::yield();

        handle.destroy();
    }

    bool done() const{
        return handle.done();
    }

    return_t& get_return_lvalue(){
        while (!handle.done()){
            std::this_thread::yield();
        }
        return handle.promise().value;
    }

    return_t&& get_return_rvalue(){
        while (!handle.done()){
            std::this_thread::yield();
        }
        return std::move(handle.promise().value);
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
            std::this_thread::yield();
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

