#include "nat_test.h"
#include <cstdint>
#include <getopt.h> 
#include <iostream>
#include <string>
#include <string_view>

std::expected<ipv4info, std::string> parse_addr(std::string_view addr){

    auto pos = addr.find(':');
    if (pos == std::string_view::npos){
        return std::unexpected("parse addr error: missing ':'");
    }

    auto ip = addr.substr(0, pos);
    auto port = addr.substr(pos + 1);

    auto ipaddr = my_inet_addr(ip);
    if (!ipaddr.has_value()){
        return std::unexpected(std::format("parse ip error: {}", ipaddr.error()));
    }
    auto portnum = my_stoi(port);
    if (!portnum.has_value()){
        return std::unexpected(std::format("parse port error: unexpected char '{}'", tohex(portnum.error())));
    }
    return ipv4info{ipaddr.value(), my_htons(portnum.value())};
}



int main(int argc, char* argv[]){
    #if defined(_WIN32) || defined(_WIN64)
    // set utf-8 console
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif

    opterr = 0;
    struct option opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"query-all-addr", no_argument, nullptr, 'q'},
        {"build-binding", required_argument, nullptr, 'b'},
        {"nat-type", no_argument, nullptr, 't'},
        {"nat-lifetime", no_argument, nullptr, 's'},
        {"interface_index", required_argument, nullptr, 'i'},
        {"log", no_argument, nullptr, 'l'}
    };

    bool flag[256] = {};

    uint32_t interface_index = 0;
    uint16_t bind_port = 0;
    for (int opt; (opt = getopt_long(argc, argv, "qhb:ltsi:", opts, nullptr)) != -1;){
        switch (opt)
        {
   
            case 'h':
                {
                    std::cout << std::format("Usage: {} <server_addr>\n options:\n", argv[0]);
                    std::cout << "  -b, --build-binding <bind_port>: build binding by specified port\n";
                    std::cout << "  -i, --interface_index <index>: specify network interface index\n";
                    std::cout << "  -t, --nat-type: test nat type\n";
                    std::cout << "  -s, --nat-lifetime: test nat lifetime\n";
                    std::cout << "  -q, --query-all-addr: query all device ip\n";
                    std::cout << "  -l, --log: enable log\n";
                }
                return 0;

            case 'q':
                {
                    auto res = query_all_device_ip();
                    for (auto& [index, addr] : res){
                        auto& [name, ip] = addr;
                        std::cout << std::format("[{:0>2}] {}: {}\n", index, reinterpret_cast<const char*>(name.c_str()), my_inet_ntoa(ip));

                    }    
                }
                return 0;            
            case 'i':
                {
                    flag['i'] = true;
                    auto e = my_stoi(optarg);
                    if (!e.has_value()){
                        std::cout << std::format("invalid interface index: {}\n", optarg);
                        return 1;
                    }
                    interface_index = e.value();
                }
                break;              
            case 'b':
                {
                    flag['b'] = true;
                    auto e = my_stoi(optarg);
                    if (!e.has_value() || e.value() < 1 || e.value() > 65535){
                        std::cout << std::format("invalid port: {}\n", optarg);
                        return 1;
                    }
                    bind_port = e.value();
                }
                break;          
            case 's':
                flag['s'] = true;
                break;
            case 't':
                flag['t'] = true;
                break;

            case 'l':
                {
                    LOG.set_enable(true);
                    LOG.set_output_file("114514.log");
                }
                break;
            default:
                std::cout << std::format("unknown option: \"-{}\", please use \"-h\" for help\n", static_cast<char>(optopt));
                return 1;
        }

    }


    std::expected<ipv4info, std::string> server_addr;
    
    if (optind < argc){
        server_addr = parse_addr(argv[optind]);
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
        if ((bind_addr = query_device_ip(interface_index)) == 0){
            std::cout << std::format("failed to query interface address: {}\nplease use -q to query all device ip\n", interface_index);
            return 1;
        }
    } else {
        if ((bind_addr = query_device_ip(interface_index)) == 0){
            std::cout << "failed to auto detect network interface address, please use -i to specify network interface\n";
            return 1;
        }
        std::cout << std::format("auto detected network interface address: {}\n", my_inet_ntoa(bind_addr));
    }


    if (flag['s']){
        std::cout << "it may take a while to test nat lifetime, please wait...\n";

        clientImpl X{bind_addr, randomPort()}, Y{bind_addr, randomPort()};
        auto res = lifetime_test(X, Y, server_addr.value());

        if (res.has_value()){
            std::cout << std::format("nat lifetime: {}s\n", res.value());
        } else {
            std::cout << res.error() << std::endl;
        }
    }
    if (flag['t']){
        clientImpl c{bind_addr, flag['b'] ? my_htons(bind_port) : randomPort()};

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
        switch (nat.filtering_type | nat.mapping_type)
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
