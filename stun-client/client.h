#pragma once
#include "async.h"
#include "log.h"
#include "stun.h"
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
            LOG.log("transaction {} on response\n", tohex(it->first));
            it->second.awaiter->response = std::move(response);
            it->second.handle.resume();
            transactions.erase(it);
        }

    }

    void onTimeout(transactionID_t transactionID){
        std::lock_guard lock{m};

        auto it = transactions.find(transactionID);
        if (it != transactions.end()){
            LOG.log("transaction {} on timeout\n", tohex(it->first));
            it->second.awaiter->response = std::unexpected("Timeout");
            it->second.handle.resume();
            transactions.erase(it);
        }
    }



    static TransactionManager& get_instance(){
        static TransactionManager instance;
        return instance;
    }

};
#define TM TransactionManager<ipinfo_t>::get_instance()

template <typename Derived, typename ipinfo_t>
class client{
public:    
    using request_t = std::tuple<ipinfo_t, size_t, const uint8_t*>;
    using response_t = std::tuple<ipinfo_t, size_t, uint8_t*>;
    using TM_t = TransactionManager<ipinfo_t>;
    using transaction_res_t = TM_t::transaction_res_t;
    using expected_res_t = TM_t::expected_res_t;

protected:
    inline void onResponse(transaction_res_t&& response){
        TM.onResponse(std::move(response));
    }

    inline void onTimeout(transactionID_t transactionID){
        TM.onTimeout(transactionID);
    }
private:
    delay_task request(const ipinfo_t& ip, const stunMessage& msg){
        co_return;
    }
public:

    explicit client() = default;
    lazy_task<expected_res_t> asyncRequest(const ipinfo_t& ip, const stunMessage& msg){
        typename TM_t::registerAwaiter awaiter{msg.getTransactionID()};

        auto delaytask = static_cast<Derived*>(this)->request(ip, msg);
        auto& res = co_await awaiter;
        
        if (res.has_value()){
            delaytask.cancel();
        }

        co_return std::move(res);
    }

};






