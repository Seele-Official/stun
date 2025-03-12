#include "timer.h"
#include "log.h"

bool Timer::submit(std::coroutine_handle<> handle, uint64_t delay){
    std::lock_guard lock{m};
    auto status = this->tasks.emplace(std::chrono::steady_clock::now() + std::chrono::milliseconds(delay), handle).second;
    LOG.async_log("task submitted {} {}\n", tohex(handle.address()), status);
    cv.notify_one();
    return status;
}



void Timer::worker(std::stop_token st){
    while(!st.stop_requested()){
        std::unique_lock lock{m};
        if(tasks.empty()){
            cv.wait(lock, [&]{
                return !tasks.empty() || st.stop_requested();
            });
            continue;
        }

        auto t = tasks.begin();
        if (t->expirytime > std::chrono::steady_clock::now()){
            cv.wait_until(lock, t->expirytime, [&]{
                return tasks.begin()->expirytime <= std::chrono::steady_clock::now() || st.stop_requested();
            });
            continue;
        }
        
        auto handle = t->handle;
        tasks.erase(t);
        lock.unlock();
        
        handle.resume();
    }
}


bool Timer::cancel(std::coroutine_handle<> handle){
    std::lock_guard lock{m};

    for (auto it = tasks.begin(); it != tasks.end(); it++){
        if (it->handle == handle){
            tasks.erase(it);
            return true;
        }
    }
    
    return false;
}