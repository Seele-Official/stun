#include "async.h"
#include "stun.h"
#include "threadpool.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sys/types.h>

class TransactionManager{
public:
    struct registerAwaiter{
        transactionID_t transactionID;
        stunMessage response;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle) noexcept {
            get_instance().registerTransaction(handle, this);
        }
        stunMessage& await_resume(){
            return response;
        }
        registerAwaiter(transactionID_t transactionID): transactionID{transactionID} {}
    };



private:

    struct transaction_t{
        std::coroutine_handle<> handle;
        registerAwaiter* awaiter;

        transaction_t(std::coroutine_handle<> handle,  registerAwaiter* awaiter): handle{handle}, awaiter{awaiter} {}
    };

    std::map<transactionID_t, transaction_t> transactions;
    std::mutex m;
public:
    void registerTransaction(std::coroutine_handle<> handle, registerAwaiter* awaiter){
        std::lock_guard lock{m};

        transactions.emplace(std::piecewise_construct, 
            std::forward_as_tuple(awaiter->transactionID), 
            std::forward_as_tuple(handle, awaiter));
    }

    void onResponse(stunMessage&& response){
        std::lock_guard lock{m};

        auto it = transactions.find(response.getHeader()->transactionID);
        if (it != transactions.end()){
            it->second.awaiter->response = std::move(response);
            it->second.handle.resume();
            transactions.erase(it);
        }

    }

    void onTimeout(transactionID_t transactionID){
        std::lock_guard lock{m};

        auto it = transactions.find(transactionID);
        if (it != transactions.end()){
            it->second.handle.resume();
            transactions.erase(it);
        }
    }



    static TransactionManager& get_instance(){
        static TransactionManager instance;
        return instance;
    }

};


template <typename Derived>
class client{
private:
    Timer timer;

    std::jthread listener_thread;

    void listener(std::stop_token st){
        while(!st.stop_requested()){
            auto response = receive();
            TransactionManager::get_instance().onResponse(std::move(response));
        }
    }

    void send(stunMessage_view msg){
        // send
    }

    stunMessage receive(){
        // receive
        return stunMessage{};
    }


    


public:
    explicit client() : listener_thread{&client::listener, this} , timer{} {}

    ~client(){
        listener_thread.request_stop();
    }

    lazy_task<stunMessage> asyncRequest(stunMessage_view msg){
        struct State {
            uint64_t id{};
            uint64_t times{};
            uint64_t expiry{};
            stunMessage_view msg;
            transactionID_t transactionID;
            std::function<void()> callback;
            State(stunMessage_view msg): msg{msg}, transactionID{msg.getHeader()->transactionID} {}
        };
    

        State state{msg};
    
        state.callback = [&state, this]() { 
            if (state.times < 7) {
                static_cast<Derived*>(this)->send(state.msg);

                state.times++;
                state.expiry = state.expiry * 2 + 500;
                state.id = timer.schedule(state.callback, state.expiry);
                std::cout << "retry " << state.times << " id: " << state.id << std::endl;
            } else {
                TransactionManager::get_instance().onTimeout(state.transactionID);
            }
        };
    
        state.id = timer.schedule(state.callback, 0);
    
        auto response = co_await TransactionManager::registerAwaiter{msg.getHeader()->transactionID};
    
        timer.cancel(state.id);

        co_return std::move(response);
    }


};

class clientImpl : public client<clientImpl>{
private:
    friend class client<clientImpl>;
    void send(stunMessage_view msg){
        std::cout << "send" << std::endl;
    }
    void receive(){
        std::cout << "receive" << std::endl;
    }
public:
    explicit clientImpl() {}

};



int main(){

    stunMessage msg(stun::messagemethod::BINDING | stun::messagetype::REQUEST);


    clientImpl c{};

    auto response = c.asyncRequest(msg);

    if (response.get_return_value().empty()){
        std::cout << "timeout" << std::endl;
    } else {
        std::cout << "response received" << std::endl;
    }
    return 0;
}


