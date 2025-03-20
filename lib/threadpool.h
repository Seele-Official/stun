#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>

template <size_t pool_size>
class threadpool{
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
        static threadpool instance;
        return instance;
    } 

    auto submit(std::coroutine_handle<> h){
        std::lock_guard<std::mutex> lock{m};
        tasks.emplace_back(h);
        cv.notify_one();
    }

    threadpool(){
        for(auto& t: threads){
            t = std::jthread(&threadpool::worker, this);
        }
    }

    ~threadpool(){
        for(auto& t: threads){
            t.request_stop();
        }
        this->cv.notify_all();
    }    

};



#define THREADPOOL threadpool<1>::get_instance()

struct threadpool_awaiter{
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        THREADPOOL.submit(handle);
    }

    void await_resume() {}

    explicit threadpool_awaiter(){}
};