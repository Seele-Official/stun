#include "client.h"
#include "log.h"



void client_udpv4::listener(std::stop_token st){
    while(!st.stop_requested()){
        constexpr size_t buffer_size = 1024;
        alignas(stun::header) std::byte buffer[buffer_size];
        net::ipv4 ipinfo;
        // check validity
        if (udp.recvfrom(ipinfo, buffer, buffer_size).has_value() && stun::message::is_valid(buffer)){
                            
            auto msg = stun::message{buffer};
            seele::log::async().info("received from {}:{} to:{}\n{}", seele::net::inet_ntoa(ipinfo.net_address), math::ntoh(ipinfo.net_port), math::ntoh(self_addr.net_port), msg.toString());

            this->onResponse(std::move(ipinfo), std::move(msg));
        }
    }
}
void client_udpv4::start_listener(){
    listener_thread = std::jthread{
        [this](std::stop_token st){
            this->listener(st);
        }
    };
}  


seele::coro::timer::delay_task client_udpv4::request(const seele::net::ipv4& ip, const stun::message& msg){
    using std::chrono_literals::operator""ms; 
    constexpr uint64_t retry = 2;
    constexpr std::chrono::milliseconds RTO = 500ms;
    std::chrono::milliseconds delay = 0ms;
    co_await seele::coro::timer::delay_awaiter{delay};
    for (size_t i = 0; i < retry; i++){
        udp.sendto(ip, msg.data_ptr(), msg.size());
        seele::log::async().info("sending from:{} to {}:{} \n{}", math::ntoh(self_addr.net_port), seele::net::inet_ntoa(ip.net_address), math::ntoh(ip.net_port), msg.toString());
        delay = delay*2 + RTO;
        co_await seele::coro::timer::delay_awaiter{delay};
    }

    this->onTimeout(msg.get_txn_id());
    co_return;
};
