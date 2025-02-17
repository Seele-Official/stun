#pragma once
#include <atomic>
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
            std::coroutine_handle<> task;
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

            task.resume();

            std::atomic_thread_fence(std::memory_order_release);
        }
    }



public:
    static auto& get_instance(){
        static threadpool instance;
        return instance;
    } 

    auto submit(std::coroutine_handle<> h){
        std::lock_guard<std::mutex> lock{m};
        tasks.push_back(h);
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



#define THREADPOOL threadpool<4>::get_instance()

struct forward2threadpool{
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        THREADPOOL.submit(handle);
    }

    void await_resume() {}
};