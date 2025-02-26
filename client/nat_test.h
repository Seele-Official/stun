#include "stun.h"
#include <cstdint>
#include <expected>


#if defined(_WIN32) || defined(_WIN64)
#include "win_client.h"
using clientImpl = win_client;
#elif defined(__linux__)
#include "linux_client.h"
using clientImpl = linux_client;
#endif


constexpr uint8_t endpoint_independent_mapping = 0b0000;
constexpr uint8_t address_dependent_mapping = 0b0100;
constexpr uint8_t address_and_port_dependent_mapping = 0b1000;
constexpr uint8_t no_nat_mapping = 0b1100;


constexpr uint8_t endpoint_independent_filtering = 0b0000;
constexpr uint8_t address_dependent_filtering = 0b0001;
constexpr uint8_t address_and_port_dependent_filtering = 0b0010;



constexpr uint8_t full_cone = 0b0000;
constexpr uint8_t restricted_cone = 0b0001;
constexpr uint8_t port_restricted_cone = 0b0010;
constexpr uint8_t symmetric = 0b1010;


struct nat_type
{
    uint8_t filtering_type;
    uint8_t mapping_type;
};

std::expected<ipv4info, std::string> build_binding(clientImpl& c, ipv4info& server_addr);
std::expected<nat_type, std::string> nat_test(clientImpl &c, ipv4info server_addr);
std::expected<uint64_t, std::string> lifetime_test(clientImpl& X, clientImpl& Y, ipv4info& server_addr);