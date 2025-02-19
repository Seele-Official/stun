#pragma once
#include "async.h"
#include "stun.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <tuple>


template <typename ipinfo_t>
class TransactionManager{
public:
    using transaction_responce = std::tuple<ipinfo_t, stunMessage>;

    struct registerAwaiter{
        transactionID_t transactionID;
        transaction_responce response;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle) noexcept {
            get_instance().registerTransaction(handle, this);
        }
        transaction_responce& await_resume(){
            return response;
        }
        registerAwaiter(transactionID_t id): transactionID{id} {}
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

    void onResponse(transaction_responce&& response){

        std::lock_guard lock{m};
        auto it = transactions.find(std::get<1>(response).getTransactionID());
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


template <typename Derived, typename ipinfo_t>
class client{
public:    
    using request_t = std::tuple<ipinfo_t, size_t, const uint8_t*>;
    using response_t = std::tuple<ipinfo_t, size_t, uint8_t*>;
    using transaction_responce = std::tuple<ipinfo_t, stunMessage>;
private:


    Timer timer;
    std::jthread listener_thread;

    void listener(std::stop_token st){
        while(!st.stop_requested()){

            auto [ipinfo, size, data] = static_cast<Derived*>(this)->receive();
            // check validity
            if (data == nullptr) continue;

            if (stunMessage::isValid(data)){
                TransactionManager<ipinfo_t>::get_instance().onResponse(transaction_responce{ipinfo, stunMessage{data}});
            }
        }
    }


    void send(request_t request){
        // send
    }

    response_t receive(){
        // receive
        return response_t{ipinfo_t{}, stunMessage{}};
    }

protected:
      void start_listener(){
        listener_thread = std::jthread{&client::listener, this};
    }  


public:
    explicit client() : timer{} {}

    ~client(){
        listener_thread.request_stop();
    }


    lazy_task<transaction_responce> asyncRequest(const ipinfo_t& ip, stunMessage_view msg, size_t retry = 7){
        struct State {
            uint64_t id{};
            size_t times{};
            uint64_t expiry{};

            request_t request;
            
            size_t max_times;
            transactionID_t transactionID;
            std::function<void()> callback;
            State(ipinfo_t ip, stunMessage_view msg, size_t retry): request{ip, msg.size(), msg.data()}, transactionID{msg.getTransactionID()}, max_times{retry} {}
        };
    

        State state{ip, msg, retry};
    
        state.callback = [&state, this]() { 
            if (state.times < state.max_times){ 
                static_cast<Derived*>(this)->send(state.request);
                // if (state.times != 0) std::cout << "retry " << state.times << " id: " << state.id << std::endl;
                state.times++;
                state.expiry = state.expiry * 2 + 500;
                state.id = timer.schedule(state.callback, state.expiry);
            } else {
                TransactionManager<ipinfo_t>::get_instance().onTimeout(state.transactionID);
            }
        };
    
        state.id = timer.schedule(state.callback, 0);



        auto response = std::move(co_await typename TransactionManager<ipinfo_t>::registerAwaiter{state.transactionID});
    
        timer.cancel(state.id);

        co_return std::move(response);
    }


};






