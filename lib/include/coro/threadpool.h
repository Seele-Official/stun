#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>
namespace seele::coro::thread{

    template <size_t pool_size>
    class thread_pool_impl{
    private:
        std::jthread threads[pool_size];

        std::condition_variable cv;
        std::mutex m;

        std::list<std::coroutine_handle<>> tasks;


        void worker(std::stop_token st){
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



    public:
        static auto& get_instance(){
            static thread_pool_impl instance;
            return instance;
        } 

        auto submit(std::coroutine_handle<> h){
            std::lock_guard<std::mutex> lock{m};
            tasks.emplace_back(h);
            cv.notify_one();
        }

        thread_pool_impl(){
            for(auto& t: threads){
                t = std::jthread([this](std::stop_token st){
                    this->worker(st);
                });
            }
        }

        ~thread_pool_impl(){
            for(auto& t: threads){
                t.request_stop();
            }
            this->cv.notify_all();
        }    

    };

    inline auto dispatch(std::coroutine_handle<> handle) {
        thread_pool_impl<2>::get_instance().submit(handle);
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






