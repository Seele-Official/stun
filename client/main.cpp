#include "nat_test.h"
#include "stun.h"
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
    struct option opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"query-all-addr", no_argument, nullptr, 'q'},
        {"nat-type", no_argument, nullptr, 't'},
        {"nat-lifetime", no_argument, nullptr, 's'},
        {"interface", required_argument, nullptr, 'i'},
        {"log", no_argument, nullptr, 'l'}
    };

    bool flag[256] = {};

    std::string_view interface;
    for (int opt; (opt = getopt_long(argc, argv, "qhltsi:", opts, nullptr)) != -1;){
        switch (opt)
        {
   
            case 'h':
                {
                    std::cout << std::format("Usage: {} <server_addr>\n options:\n", argv[0]);
                    std::cout << "  -t, --nat-type: test nat type\n";
                    std::cout << "  -s, --nat-lifetime: test nat lifetime\n";
                    std::cout << "  -i, --interface: specify network interface\n";
                    std::cout << "  -q, --query-all-addr: query all device ip\n";
                    std::cout << "  -l, --log: enable log\n";
                }
                return 0;

            case 'q':
                {
                    auto res = linux_client::query_all_device_ip();
                    for (auto& [name, addr] : res){
                        std::cout << std::format("{}: {}\n", name, my_inet_ntoa(addr));
                    }    
                }
                return 0;            
            case 'i':
                {
                    flag['i'] = true;
                    interface = optarg;
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
                    LOG.set_output_file("nat_test.log");
                }
                break;
            default:
                std::cout << "unknown option, use -h for help\n";
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
        if ((bind_addr = linux_client::query_device_ip(interface)) == 0){
            std::cout << std::format("failed to query interface address: {}\nplease use -q to query all device ip\n", interface);
            return 1;
        }
    } else {
        bind_addr = linux_client::query_device_ip("auto");
        std::cout << std::format("auto detected interface address: {}\n", my_inet_ntoa(bind_addr));
    }


    if (flag['s']){
        std::cout << "it may take a while to test nat lifetime, please wait...\n";

        clientImpl X{bind_addr}, Y{bind_addr};
        auto res = lifetime_test(X, Y, server_addr.value());

        if (res.has_value()){
            std::cout << std::format("nat lifetime: {}s\n", res.value());
        } else {
            std::cout << res.error() << std::endl;
        }
    }
    if (flag['t']){
        clientImpl c{bind_addr};

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
    }

    return 0;
}
