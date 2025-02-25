#include "nat_test.h"
#include "stun.h"
#include <getopt.h> 
#include <iostream>

std::expected<ipv4info, std::string> parse_addr(std::string_view addr){

    auto pos = addr.find(':');
    if (pos == std::string_view::npos){
        return std::unexpected("invalid address");
    }

    auto ip = addr.substr(0, pos);
    auto port = addr.substr(pos + 1);

    auto ipaddr = my_inet_addr(ip);
    if (!ipaddr.has_value()){
        return std::unexpected(std::format("parse ip error: {}", ipaddr.error()));
    }
    auto portnum = my_stoi(port);
    if (!portnum.has_value()){
        return std::unexpected(std::format("parse port error: unexpected char '{}'", portnum.error()));
    }
    return ipv4info{ipaddr.value(), my_htons(portnum.value())};
}



int main(int argc, char* argv[]){
    LOG.set_enable(true);
    LOG.set_output_file("nat_test.log");
    
    struct option opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"nat-type", no_argument, nullptr, 't'},
        {"nat-lifetime", no_argument, nullptr, 's'},
        {"server-addr", required_argument, nullptr, 'a'}
    };

    bool flag[256] = {};

    std::expected<ipv4info, std::string> server_addr;
    
    for (int opt; (opt = getopt_long(argc, argv, "a:hts", opts, nullptr)) != -1;){
        switch (opt)
        {
            case 'a':
                {
                    server_addr = parse_addr(optarg);

                    if (!server_addr.has_value()){
                        std::cout << server_addr.error() << std::endl;
                        return 1;
                    }
                    flag['a'] = true;
                }
                break;
            case 's':
                flag['s'] = true;
                
                break;
            case 't':
                flag['t'] = true;
                break;        
            case 'h':
                {
                    std::cout << std::format("Usage: {} -a <server_addr>\n options:\n", argv[0]);
                    std::cout << "  -t, --nat-type: test nat type\n";
                    std::cout << "  -s, --nat-lifetime: test nat lifetime\n";
                }
                return 0;
            default:
                std::cout << "unknown option, use -h for help\n";
                return 1;
        }

    }



    if (!flag['a']){
        std::cout << "server address is required, use -h for help\n";
        return 1;
    }
    if (flag['s']){
        clientImpl X, Y;
        std::cout << std::format("X: {}, Y: {}\n", X.getMyInfo().toString(), Y.getMyInfo().toString());

        auto res = lifetime_test(X, Y, server_addr.value());

        if (res.has_value()){
            std::cout << "lifetime: " << res.value() << "ms\n";
        } else {
            std::cout << res.error() << std::endl;
        }
    }
    if (flag['t']){
        clientImpl c;
        std::cout << std::format("bind: {}\n", c.getMyInfo().toString());
    
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
