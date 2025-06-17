#pragma once

#include <cstddef>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>
#include <vector>
namespace seele::coro::thread {
    

    class thread_pool_impl{
    private:
        std::vector<std::jthread> workers;

        std::condition_variable cv;
        std::mutex m;

        std::list<std::coroutine_handle<>> tasks;


        void worker(std::stop_token st);



    public:
        static auto& get_instance(){
            static thread_pool_impl instance{2};
            return instance;
        } 

        auto submit(std::coroutine_handle<> h){
            std::lock_guard<std::mutex> lock{m};
            tasks.emplace_back(h);
            cv.notify_one();
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






