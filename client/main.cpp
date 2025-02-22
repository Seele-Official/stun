#include "nat_test.h"


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
    
    if (argc != 2){
        std::cout << "usage: " << argv[0] << " <server address:port>\n";
        return 1;
    }

    auto server_addr = parse_addr(argv[1]);
    if (!server_addr.has_value()){
        std::cout << server_addr.error() << std::endl;
        return 1;
    }
    clientImpl c;
    std::cout << std::format("ip: {}\n", c.getMyInfo().toString());

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



    return 0;
}
