#pragma once

#include "IpAddress.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include <cstdint>
#include <cstring>

namespace dispatcher {

struct NetworkEndpoint {
    IpAddress address;
    uint16_t  port{0};

    boost::asio::ip::address toAsioAddress() const {
        if (address.isIPv6()) {
            boost::asio::ip::address_v6::bytes_type bytes;
            std::memcpy(bytes.data(), address.getInAddrV6().s6_addr, 16);
            return boost::asio::ip::address_v6(bytes);
        }
        return boost::asio::ip::address_v4(ntohl(address.getInAddrV4().s_addr));
    }
};

} // namespace dispatcher
