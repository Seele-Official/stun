#include "coro/threadpool.h"

namespace seele::coro::thread {

    void thread_pool_impl::worker(std::stop_token st){
        while(!st.stop_requested()){
            std::coroutine_handle<> h;
            {
                std::unique_lock lock{m};
                if (tasks.empty()){
                    cv.wait(lock, [&]{
                        return !tasks.empty() || st.stop_requested();
                    });
                    continue;
                }
                
                h = tasks.front();
                tasks.pop_front();
            }

            h.resume();
        }
    }
    thread_pool_impl::thread_pool_impl(size_t worker_count) {
        workers.reserve(worker_count);
        for(size_t i = 0; i < worker_count; ++i){
            workers.emplace_back([this](std::stop_token st){
                this->worker(st);
            });
        }
    }

    thread_pool_impl::~thread_pool_impl() {
        for (auto& worker : workers) {
            worker.request_stop();
        }
        cv.notify_all();
    }



}