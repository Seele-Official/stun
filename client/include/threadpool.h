#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <coroutine>
#include <tuple>

template <size_t pool_size>
class threadpool{
private:
    std::jthread threads[pool_size];

    std::condition_variable cv;
    std::mutex m;

    std::list<std::tuple<std::coroutine_handle<>, bool>> tasks;


    void worker(std::stop_token st){
        while(!st.stop_requested()){
            std::tuple<std::coroutine_handle<>, bool> task;
            {
                std::unique_lock lock{m};
                if (tasks.empty()){
                    cv.wait(lock, [&]{
                        return !tasks.empty() || st.stop_requested();
                    });
                    continue;
                }
                
                task = tasks.front();
                tasks.pop_front();
            }

            auto& [h, own] = task;
            if (h.resume(), own){ h.destroy(); std::cout<<("destroyed coroutine\n");}

        }
    }



public:
    static auto& get_instance(){
        static threadpool instance;
        return instance;
    } 

    auto submit(std::coroutine_handle<> h, bool own = false){
        std::lock_guard<std::mutex> lock{m};
        tasks.emplace_back(h, own);
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



#define THREADPOOL threadpool<3>::get_instance()

struct forward2threadpool{
    bool own;
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        THREADPOOL.submit(handle);
    }

    void await_resume() {}

    explicit forward2threadpool(bool own = false): own{own}{}
};