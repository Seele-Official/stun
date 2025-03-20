#include "timer.h"



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
        if (t->first > std::chrono::steady_clock::now()){
            cv.wait_until(lock, t->first, [&]{
                return tasks.begin()->first <= std::chrono::steady_clock::now() || st.stop_requested();
            });
            continue;
        }
        
        auto task = t->second;
        tasks.erase(t);

        task.run();   
        lock.unlock();

    }
}


bool Timer::cancel(std::coroutine_handle<> handle){
    std::lock_guard lock{m};

    for (auto& it : tasks){
        if (it.second.handle == handle){
            tasks.erase(it.first);
            LOG.log("cancelling task: {}\n", tohex(handle.address()));
            return true;
        }
    }
    
    return false;
}