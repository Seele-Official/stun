#include "coro/timer.h"

namespace seele::coro::timer {

    void timer_impl::worker(std::stop_token st){
        using std::chrono_literals::operator""ms; 
        
        while(!st.stop_requested()){
            std::unique_lock lock{m};
            if(tasks.empty()){
                cv.wait_until(lock, std::chrono::steady_clock::now() + 100ms, [&]{
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

            task.async_run();   
            lock.unlock();

        }
    }


    bool timer_impl::cancel(std::coroutine_handle<> handle){
        std::lock_guard lock{m};

        for (auto& it : tasks){
            if (it.second.handle == handle){
                tasks.erase(it.first);
                seele::log::sync().info("cancelling task: {}\n", math::tohex(handle.address()));
                return true;
            }
        }
        
        return false;
    }
}