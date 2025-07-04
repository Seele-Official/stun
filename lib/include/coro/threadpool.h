#pragma once

#include <climits>
#include <cstddef>
#include <mutex>
#include <semaphore>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>
#include <vector>
#include "struct/ms_queue.h"
namespace seele::coro::thread {
    

    class thread_pool_impl{
    private:
        std::vector<std::jthread> workers;

        structs::ms_queue<std::coroutine_handle<>> tasks;
        std::counting_semaphore<> sem;

        void worker(std::stop_token st);



    public:
        static auto& get_instance(){
            static thread_pool_impl instance{4};
            return instance;
        } 

        auto submit(std::coroutine_handle<> h){
            tasks.emplace_back(h);
            sem.release();
        }

        thread_pool_impl(size_t worker_count);

        ~thread_pool_impl();    

    };

    inline auto dispatch(std::coroutine_handle<> handle) {
        thread_pool_impl::get_instance().submit(handle);
    }    

    struct dispatch_awaiter{
        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<> handle) {
            dispatch(handle);
        }

        void await_resume() {}

        explicit dispatch_awaiter(){}
    };

}






