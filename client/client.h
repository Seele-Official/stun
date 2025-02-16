#pragma once
#include "async.h"
#include "stun.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <tuple>
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
        registerAwaiter(stunMessage_view msg): transactionID{msg.getTransactionID()} {}
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
        if (!response.isValid()) return;

        std::lock_guard lock{m};
        auto it = transactions.find(response.getTransactionID());
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
            auto response = static_cast<Derived*>(this)->receive();
            if (response.empty()) continue;
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

    lazy_task<stunMessage> asyncRequest(stunMessage_view msg, size_t retry = 7){
        struct State {
            uint64_t id{};
            size_t times{};
            uint64_t expiry{};

            stunMessage_view msg; 
            size_t max_times;
            transactionID_t transactionID;
            std::function<void()> callback;
            State(stunMessage_view msg, size_t retry): msg{msg}, transactionID{msg.getTransactionID()}, max_times{retry} {}
        };
    

        State state{msg, retry};
    
        state.callback = [&state, this]() { 
            if (state.times < state.max_times){ 
                static_cast<Derived*>(this)->send(state.msg);
                if (state.times != 0) std::cout << "retry " << state.times << " id: " << state.id << std::endl;
                state.times++;
                state.expiry = state.expiry * 2 + 500;
                state.id = timer.schedule(state.callback, state.expiry);
            } else {
                TransactionManager::get_instance().onTimeout(state.transactionID);
            }
        };
    
        state.id = timer.schedule(state.callback, 0);
    
        stunMessage response{std::move(co_await TransactionManager::registerAwaiter{msg})};
    
        timer.cancel(state.id);

        co_return std::move(response);
    }


};






