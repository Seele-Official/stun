#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>

template <size_t pool_size>
class thread_pool{
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
        static thread_pool instance;
        return instance;
    } 

    auto submit(std::coroutine_handle<> h){
        std::lock_guard<std::mutex> lock{m};
        tasks.emplace_back(h);
        cv.notify_one();
    }

    thread_pool(){
        for(auto& t: threads){
            t = std::jthread([this](std::stop_token st){
                this->worker(st);
            });
        }
    }

    ~thread_pool(){
        for(auto& t: threads){
            t.request_stop();
        }
        this->cv.notify_all();
    }    

};



#define THREADPOOL thread_pool<1>::get_instance()

struct threadpool_awaiter{
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        THREADPOOL.submit(handle);
    }

    void await_resume() {}

    explicit threadpool_awaiter(){}
};