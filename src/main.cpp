#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "nat_test.h"
#include "net.h"
#include "opts.h"
#include "meta.h"
std::expected<seele::net::ipv4, std::string> parse_addr(std::string_view addr){

    auto pos = addr.find(':');
    if (pos == std::string_view::npos){
        return std::unexpected("parse addr error: missing ':'");
    }

    auto ip = addr.substr(0, pos);
    auto port = addr.substr(pos + 1);

    auto ipaddr = seele::net::inet_addr(ip);
    if (!ipaddr.has_value()){
        return std::unexpected(std::format("parse ip error: {}", ipaddr.error()));
    }
    auto portnum = math::stoi(port);
    if (!portnum.has_value()){
        return std::unexpected(std::format("parse port error: unexpected char '{}'", math::tohex(portnum.error())));
    }
    return seele::net::ipv4{ipaddr.value(), math::hton<uint16_t>(portnum.value())};
}


#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
int main(int argc, char* argv[]){
    #if defined(_WIN32) || defined(_WIN64)
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif
    auto options = seele::opts::make_opts(
            seele::opts::ruler::no_arg("--help", "-h"),
            seele::opts::ruler::no_arg("--query-all-addr", "-q"),
            seele::opts::ruler::opt_arg("--build-binding", "-b"),
            seele::opts::ruler::no_arg("--nat-type", "-t"),
            seele::opts::ruler::no_arg("--nat-lifetime", "-s"),
            seele::opts::ruler::req_arg("--interface_index", "-i"),
            seele::opts::ruler::opt_arg("--log", "-l")
    );
    bool flag[256] = {};
    uint32_t interface_index = 0;
    uint16_t bind_port = 0;
    
    seele::opts::pos_arg p_args;

    auto parser = options.parse(argc, argv);
    for (auto&& item : parser) {
        if (!item) {
            std::cout << "Error: " << item.error() << "\n";
            return 1;
        }
        seele::meta::visit_var(item.value(), 
            [&](seele::opts::no_arg& arg) {
                if (arg.long_name == "--help") {
                    std::cout << std::format("Usage: {} <server_addr>\n options:\n", argv[0]);
                    std::cout << "  -b, --build-binding <bind_port>?: build binding by specified port\n";
                    std::cout << "  -i, --interface_index <index>: specify network interface index\n";
                    std::cout << "  -t, --nat-type: test nat type\n";
                    std::cout << "  -s, --nat-lifetime: test nat lifetime\n";
                    std::cout << "  -q, --query-all-addr: query all device ip\n";
                    std::cout << "  -l, --log <file_name>?: enable log\n";
                    std::exit(0);
                }
                else if (arg.long_name == "--query-all-addr") {
                    auto res = seele::net::query_all_device_ip();
                    for (auto& [index, addr] : res){
                        auto& [name, ip] = addr;
                        std::cout << std::format("[{:0>2}] {}: {}\n", index, reinterpret_cast<const char*>(name.c_str()), seele::net::inet_ntoa(ip));

                    }   
                    std::exit(0);
                }
                else if (arg.long_name == "--nat-type") {
                    flag['t'] = true;
                }
                else if (arg.long_name == "--nat-lifetime") {
                    flag['s'] = true;
                }
            },
            [&](seele::opts::req_arg& arg) {
                if (arg.long_name == "--interface_index") {
                    flag['i'] = true;
                    auto e = math::stoi(arg.value);
                    if (!e.has_value()) {
                        std::cout << std::format("invalid interface index: {}\n", arg.value);
                        std::exit(1);
                    }
                    interface_index = e.value();
                }
            },
            [&](seele::opts::opt_arg& arg) {
                if (arg.long_name == "--build-binding") {
                    flag['b'] = true;
                    if (arg.value.has_value()) {
                        auto e = math::stoi(arg.value.value());
                        if (!e.has_value() || e.value() < 1 || e.value() > 65535) {
                            std::cout << std::format("invalid port: {}\n", arg.value.value());
                            std::exit(1);
                        }
                        bind_port = math::hton<uint16_t>(e.value());
                    } else {
                        bind_port = seele::net::random_pri_iana_net_port();
                    }
                } else if (arg.long_name == "--log") {
                    seele::log::logger().set_enable(true);
                    if (arg.value.has_value()) {
                        seele::log::logger().set_output_file(arg.value.value());
                    } else {
                        seele::log::logger().set_output_file("114514.log");
                    }
                }
            },
            [&](seele::opts::pos_arg& arg) {
                p_args = std::move(arg);
            }
        );
    }




    std::expected<seele::net::ipv4, std::string> server_addr;
    
    if (p_args.values.size() == 1){
        server_addr = parse_addr(*p_args.values.begin());
        if (!server_addr.has_value()){
            std::cout << server_addr.error() << std::endl;
            return 1;
        }
    } else {
        std::cout << "missing server address, use -h for help\n";
        return 1;
    }

    uint32_t bind_addr = 0;
    if (flag['i']){
        if ((bind_addr = seele::net::query_device_ip(interface_index)) == 0){
            std::cout << std::format("failed to query interface address: {}\nplease use -q to query all device ip\n", interface_index);
            return 1;
        }
    } else {
        if ((bind_addr = seele::net::query_device_ip(interface_index)) == 0){
            std::cout << "failed to auto detect network interface address, please use -i to specify network interface\n";
            return 1;
        }
        std::cout << std::format("auto detected network interface address: {}\n", seele::net::inet_ntoa(bind_addr));
    }


    if (flag['s']){
        std::cout << "it may take a while to test nat lifetime, please wait...\n";

        clientImpl X{bind_addr, seele::net::random_pri_iana_net_port()}, Y{bind_addr, seele::net::random_pri_iana_net_port()};
        auto res = lifetime_test(X, Y, server_addr.value());

        if (res.has_value()){
            std::cout << std::format("nat lifetime: {}s\n", res.value());
        } else {
            std::cout << res.error() << std::endl;
        }
    }
    if (flag['t']){
        clientImpl c{bind_addr, flag['b'] ? bind_port : seele::net::random_pri_iana_net_port()};

        auto res = nat_test(c, server_addr.value());
    
    
        if (!res.has_value()){
            std::cout << res.error() << std::endl;
            return 1;
        }
    
        auto nat = res.value();
    
        std::cout << "filtering: ";
        switch (nat.filtering_type)
        {
        case endpoint_independent_filtering:
            std::cout << "endpoint independent\n";
            break;
        case address_dependent_filtering:
            std::cout << "address dependent\n";
            break;
        case address_and_port_dependent_filtering:  
            std::cout << "address and port dependent\n";
            break;
        default:
            std::cout << "undefined\n";
            break;
        }
    
        std::cout << "mapping: ";
        switch (nat.mapping_type)
        {
        case endpoint_independent_mapping:
            std::cout << "endpoint independent\n";
            break;
        case address_dependent_mapping:
            std::cout << "address dependent\n";
            break;
        case address_and_port_dependent_mapping:
            std::cout << "address and port dependent\n";
            break;
        case no_nat_mapping:
            std::cout << "no nat\n";
            break;
        default:
            std::cout << "undefined\n";
            break;
        }
    
        std::cout << "nat type: ";
        switch (nat.type())
        {
        case full_cone:
            std::cout << "full cone\n";
            break;
        case restricted_cone:
            std::cout << "restricted cone\n";
            break;
        case port_restricted_cone:
            std::cout << "port restricted cone\n";
            break;
        case symmetric:
            std::cout << "symmetric\n";
            break;
        default:
            std::cout << "undefined\n";
            break;
        }

        if (flag['b']){
            auto res = build_binding(c, server_addr.value());
            if (!res.has_value()){
                std::cout << res.error() << std::endl;
                return 1;
            }
            std::cout << std::format("build binding success, {} is mapped to public address {}\n", c.get_my_addr().toString(), res.value().toString());
            std::cout << "if the binding does not expire within a period of time, you may reusing the public address to establish connection\n";
        }
        return 0;
    }

    if (flag['b']){
        clientImpl c{bind_addr, bind_port};
        auto res = build_binding(c, server_addr.value());
        if (!res.has_value()){
            std::cout << res.error() << std::endl;
            return 1;
        }
        std::cout << std::format("build binding success, {} is mapped to public address {}\n", c.get_my_addr().toString(), res.value().toString());
        std::cout << "if the binding does not expire within a period of time, you may reusing the public address to establish connection\n";
    }

    return 0;
}
