#include "client.h"
#include "net_core.h"
#include "stun.h"
#include "coro/timer.h"
#include <chrono>
#include <cstddef>

class udpv4_client : public client<udpv4_client, ipv4info>{
private:
    friend class client<udpv4_client, ipv4info>;

    udpv4 udp;
    ipv4info my_addr;

    std::jthread listener_thread;

    void listener(std::stop_token st){
        while(!st.stop_requested()){
            constexpr size_t buffer_size = 1024;
            alignas(stun::header) std::byte buffer[buffer_size];
            ipv4info ipinfo;
            // check validity
            if (udp.recvfrom(ipinfo, buffer, buffer_size).has_value() && stun::message::is_valid(buffer)){
                                
                auto msg = stun::message{buffer};
                LOG("received from {}:{} to:{}\n{}", my_inet_ntoa(ipinfo.net_address), math::ntoh(ipinfo.net_port), math::ntoh(my_addr.net_port), msg.toString());

                this->onResponse(std::move(ipinfo), std::move(msg));
            }
        }
    }
    void start_listener(){
        listener_thread = std::jthread{
            [this](std::stop_token st){
                this->listener(st);
            }
        };
    }  


    coro::timer::delay_task request(const ipv4info& ip, const stun::message& msg){
        using std::chrono_literals::operator""ms; 
        constexpr uint64_t retry = 2;
        constexpr std::chrono::milliseconds RTO = 500ms;
        std::chrono::milliseconds delay = 0ms;
        co_await coro::timer::delay_awaiter{delay};
        for (size_t i = 0; i < retry; i++){
            udp.sendto(ip, msg.data_ptr(), msg.size());
            ASYNC_LOG("sending from:{} to {}:{} \n{}", math::ntoh(my_addr.net_port), my_inet_ntoa(ip.net_address), math::ntoh(ip.net_port), msg.toString());
            delay = delay*2 + RTO;
            co_await coro::timer::delay_awaiter{delay};
        }

        this->onTimeout(msg.get_txn_id());
        co_return;
    };

public:
    explicit udpv4_client(uint32_t net_ip, uint16_t net_port) : my_addr{net_ip, net_port} {
        if (!udp.bind(ipv4info{net_ip, net_port})){
            std::exit(1);
        }
        udp.set_timeout(3);
        start_listener();
    }

    ~udpv4_client(){listener_thread.request_stop();}
    const ipv4info& get_my_addr() const { return my_addr; }


};
