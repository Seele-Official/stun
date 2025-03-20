#pragma once
#include <concepts>
#include <coroutine>
#include <cstdio>
#include <thread>
#include <tuple>
#include <utility>
#include <type_traits>
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
        lazy_task<void> get_return_object(){
            return {};
        }

        auto initial_suspend(){
            return std::suspend_never{};
        }

        auto final_suspend() noexcept{
            return std::suspend_never{};
        }
        void unhandled_exception() {  }

        void return_void(){}
    };
};

template <typename lambda_t, typename... args_t>
requires std::invocable<lambda_t, std::remove_reference_t<args_t>&...>
lazy_task<std::invoke_result_t<lambda_t, std::remove_reference_t<args_t>&...>> async(lambda_t&& lambda, args_t&&... args){ 
    auto l = std::forward<lambda_t>(lambda);
    std::tuple<std::remove_reference_t<args_t>...> t{std::forward<args_t>(args)...};

    co_await threadpool_awaiter{};
    co_return std::apply(l, t);
}
