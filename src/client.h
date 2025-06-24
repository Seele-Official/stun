#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <tuple>
#include <expected>

#include "log.h"
#include "stun.h"
#include "coro/timer.h"
#include "net/udpv4.h"
using namespace seele;
template <typename Derived, typename ipinfo_t>
class client{
public:    
    class txn_manager{
    public:
        using expected_res_t = std::expected<
                                    std::tuple<ipinfo_t, stun::message>, 
                                    std::string
                                >;

        struct reg_awaiter{
            stun::txn_id_t txn_id;
            expected_res_t response;
            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle) noexcept {
                get_instance().register_txn(handle, this);
            }
            expected_res_t& await_resume(){
                return response;
            }
            reg_awaiter(stun::txn_id_t id): txn_id{id} {}
            
        };

    private:

        struct txn_t{
            std::coroutine_handle<> handle;
            reg_awaiter* awaiter;

            txn_t(std::coroutine_handle<> handle,  reg_awaiter* awaiter): handle{handle}, awaiter{awaiter} {}
        };

        std::map<stun::txn_id_t, txn_t> txns;
        std::mutex m;
    public:
        void register_txn(std::coroutine_handle<> handle, reg_awaiter* awaiter){
            std::lock_guard lock{m};

            txns.emplace(std::piecewise_construct, 
                std::forward_as_tuple(awaiter->txn_id), 
                std::forward_as_tuple(handle, awaiter));
        }

        void onResponse(ipinfo_t&& ip, stun::message&& msg){


            std::lock_guard lock{m};
            auto it = txns.find(msg.get_txn_id());
            if (it != txns.end()){
                log::sync().info("transaction {} on response\n", math::tohex(it->first));
                it->second.awaiter->response = std::move(std::make_tuple(std::move(ip), std::move(msg)));
                it->second.handle.resume();
                txns.erase(it);
            }

        }

        void onTimeout(stun::txn_id_t txn_id){
            std::lock_guard lock{m};

            auto it = txns.find(txn_id);
            if (it != txns.end()){
                log::sync().info("transaction {} on timeout\n", math::tohex(it->first));
                it->second.awaiter->response = std::unexpected("Timeout");
                it->second.handle.resume();
                txns.erase(it);
            }
        }



        static txn_manager& get_instance(){
            static txn_manager instance;
            return instance;
        }

    };
protected:
    inline void onResponse(ipinfo_t&& ip, stun::message&& msg){
        txn_manager::get_instance().onResponse(std::move(ip), std::move(msg));
    }

    inline void onTimeout(stun::txn_id_t txn_id){
        txn_manager::get_instance().onTimeout(txn_id);
    }
private:
    coro::timer::delay_task request(const ipinfo_t& ip, const stun::message& msg){
        co_return;
    }

public:

    explicit client() = default;
    
    coro::lazy_task<typename txn_manager::expected_res_t> 
        async_req(const ipinfo_t& ip, const stun::message& msg){
            typename txn_manager::reg_awaiter awaiter{msg.get_txn_id()};

            auto delaytask = static_cast<Derived*>(this)->request(ip, msg);
            auto& res = co_await awaiter;
            
            if (res.has_value()){
                delaytask.cancel();
            }

            co_return std::move(res);
        }

};


class client_udpv4 : public client<client_udpv4, net::ipv4>{
private:
    friend class client<client_udpv4, net::ipv4>;

    net::udpv4 udp;
    net::ipv4 self_addr;

    std::jthread listener_thread;

    void listener(std::stop_token st);
    void start_listener();


    coro::timer::delay_task request(const net::ipv4& ip, const stun::message& msg);

public:
    explicit inline client_udpv4(uint32_t net_ip, uint16_t net_port) : self_addr{net_ip, net_port} {
        if (!udp.bind(net::ipv4{net_ip, net_port})){
            std::exit(1);
        }
        udp.set_timeout(3);
        start_listener();
    }

    ~client_udpv4(){ listener_thread.request_stop(); }
    inline const net::ipv4& get_self_addr() const { return self_addr; }

};





