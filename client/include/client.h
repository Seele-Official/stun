#pragma once
#include "async.h"
#include "stun.h"
#include "threadpool.h"
#include "timer.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <tuple>
#include <expected>
template <typename ipinfo_t>
class TransactionManager{
public:
    using transaction_res_t = std::tuple<ipinfo_t, stunMessage>;
    using expected_res_t = std::expected<transaction_res_t, std::string>;

    struct registerAwaiter{
        transactionID_t transactionID;
        expected_res_t response;
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle) noexcept {
            get_instance().registerTransaction(handle, this);
        }
        expected_res_t& await_resume(){
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

    void onResponse(transaction_res_t&& response){
        std::lock_guard lock{m};
        auto it = transactions.find(std::get<1>(response).getTransactionID());
        if (it != transactions.end()){
            it->second.awaiter->response = std::move(response);
            THREADPOOL.submit(it->second.handle);
            transactions.erase(it);
        }

    }

    void onTimeout(transactionID_t transactionID){
        std::lock_guard lock{m};

        auto it = transactions.find(transactionID);
        if (it != transactions.end()){
            it->second.awaiter->response = std::unexpected("Timeout");
            THREADPOOL.submit(it->second.handle);
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
    using transaction_res_t = std::tuple<ipinfo_t, stunMessage>;
    using expected_res_t = std::expected<transaction_res_t, std::string>;
private:


    std::jthread listener_thread;

    void listener(std::stop_token st){
        while(!st.stop_requested()){

            auto [ipinfo, size, data] = static_cast<Derived*>(this)->receive();

            // check validity
            if (data != nullptr && stunMessage::isValid(data)){
                TransactionManager<ipinfo_t>::get_instance().
                    onResponse(transaction_res_t{ipinfo, stunMessage{data}});
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

    Timer::delay_task request(const ipinfo_t& ip, stunMessage_view msg, size_t retry){
        constexpr uint64_t RTO = 500;

        uint64_t delay = 0;
        request_t request{ip, msg.size(), msg.data()};
        for (size_t i = 0; i < retry; i++){
            static_cast<Derived*>(this)->send(request);
            delay = delay*2 + RTO;
            co_await forward2Timer{delay};
        }
        TransactionManager<ipinfo_t>::get_instance().onTimeout(msg.getTransactionID());
        co_return;
    };




protected:
      void start_listener(){
        listener_thread = std::jthread{&client::listener, this};
    }  


public:
    explicit client() {}

    ~client(){
        listener_thread.request_stop();
    }


    lazy_task<expected_res_t> asyncRequest(const ipinfo_t& ip, stunMessage_view msg, size_t retry = 7){


        auto delay = request(ip, msg, retry);

        

        auto response = std::move(co_await typename TransactionManager<ipinfo_t>::registerAwaiter{msg.getTransactionID()});


        co_return std::move(response);
    }


};






