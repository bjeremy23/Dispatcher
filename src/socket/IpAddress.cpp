
#include "IpAddress.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace dispatcher;

IpAddress::IpAddress()
{
    ipAddr_m.ip_family = 0;
}

IpAddress::IpAddress(const struct inx_addr& in) :
    ipAddr_m(in)
{}

IpAddress::IpAddress(uint32_t v4addr)
{
    *this = v4addr;
}

IpAddress::IpAddress(const struct in_addr& v4addr)
{
    *this = v4addr;
}

IpAddress::IpAddress(const IPv6String& v6addr)
{
    *this = v6addr;
}

IpAddress::IpAddress(const struct in6_addr& v6addr)
{
    *this = v6addr;
}

IpAddress::IpAddress(const IpAddress& other) :
    ipAddr_m(other.ipAddr_m)
{}

IpAddress::IpAddress(const std::string& address, uint8_t ip_family)
{
    if (!this->pton(address, ip_family))
    {
        throw std::invalid_argument("Could not convert: " + address +
                                    " Family: " + std::to_string(static_cast<uint32_t>(ip_family)));
    }
}

IpAddress::~IpAddress() {}

IpAddress& IpAddress::operator=(uint32_t v4addr)
{
    ipAddr_m.ip.v4.s_addr = v4addr;
    ipAddr_m.ip_family    = AF_INET;
    return *this;
}

IpAddress& IpAddress::operator=(const struct in_addr& v4addr)
{
    ipAddr_m.ip.v4     = v4addr;
    ipAddr_m.ip_family = AF_INET;
    return *this;
}

IpAddress& IpAddress::operator=(const IPv6String& v6addr)
{
    if (v6addr.size() >= IPV6_LENGTH)
    {
        memcpy(ipAddr_m.ip.v6.s6_addr, &v6addr[0], IPV6_LENGTH);
        ipAddr_m.ip_family = AF_INET6;
    }
    return *this;
}

IpAddress& IpAddress::operator=(const struct in6_addr& v6addr)
{
    ipAddr_m.ip.v6     = v6addr;
    ipAddr_m.ip_family = AF_INET6;
    return *this;
}

IpAddress& IpAddress::operator=(const IpAddress& other)
{
    if (this != &other)
        memcpy(&ipAddr_m, &other.ipAddr_m, sizeof(struct inx_addr));
    return *this;
}

IpAddress& IpAddress::operator=(const struct sockaddr& addr)
{
    ipAddr_m.ip_family = addr.sa_family;
    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
        {
            auto& addr_in = reinterpret_cast<const sockaddr_in&>(addr);
            ipAddr_m.ip.v4.s_addr = addr_in.sin_addr.s_addr;
            break;
        }
        case AF_INET6:
        {
            auto& addr_in6 = reinterpret_cast<const sockaddr_in6&>(addr);
            auto  ipv6_len = sizeof(ipAddr_m.ip.v6.s6_addr);
            memcpy(ipAddr_m.ip.v6.s6_addr, addr_in6.sin6_addr.s6_addr, ipv6_len);
            break;
        }
        default:
            break;
    }
    return *this;
}

bool IpAddress::operator==(const struct in_addr& v4addr) const
{
    return *this == v4addr.s_addr;
}

bool IpAddress::operator==(const struct in6_addr& v6addr) const
{
    IPv6String tmp(v6addr.s6_addr, v6addr.s6_addr + 16);
    return *this == tmp;
}

bool IpAddress::operator==(uint32_t v4addr) const
{
    return isIPv4() && (ipAddr_m.ip.v4.s_addr == v4addr);
}

bool IpAddress::operator==(int32_t v4addr) const
{
    return isIPv4() && (ipAddr_m.ip.v4.s_addr == static_cast<uint32_t>(v4addr));
}

bool IpAddress::operator==(const IPv6String& v6addr) const
{
    if (!isIPv6() || v6addr.size() < IPV6_LENGTH)
        return false;
    return (memcmp(ipAddr_m.ip.v6.s6_addr, &v6addr[0], IPV6_LENGTH) == 0);
}

bool IpAddress::operator==(const IpAddress& other) const
{
    if (ipAddr_m.ip_family != other.ipAddr_m.ip_family)
        return false;

    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
            return (ipAddr_m.ip.v4.s_addr == other.ipAddr_m.ip.v4.s_addr);
        case AF_INET6:
            return (!memcmp(ipAddr_m.ip.v6.s6_addr, other.ipAddr_m.ip.v6.s6_addr, 16));
        case 0:
            return !(other.ipAddr_m.ip_family);
        default:
            return false;
    }
}

bool IpAddress::operator==(const struct inx_addr& other) const
{
    if (ipAddr_m.ip_family != other.ip_family)
        return false;

    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
            return (ipAddr_m.ip.v4.s_addr == other.ip.v4.s_addr);
        case AF_INET6:
            return (!memcmp(ipAddr_m.ip.v6.s6_addr, other.ip.v6.s6_addr, 16));
        case 0:
            return !(other.ip_family);
        default:
            return false;
    }
}

bool IpAddress::operator!=(uint32_t v4addr)        const { return !(*this == v4addr); }
bool IpAddress::operator!=(int32_t v4addr)         const { return !(*this == v4addr); }
bool IpAddress::operator!=(const IPv6String& v6)   const { return !(*this == v6); }
bool IpAddress::operator!=(const IpAddress& other) const { return !(*this == other); }

bool IpAddress::operator<(const IpAddress& other) const
{
    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
            switch (other.ipAddr_m.ip_family)
            {
                case AF_INET:  return ipAddr_m.ip.v4.s_addr < other.ipAddr_m.ip.v4.s_addr;
                case AF_INET6: return true;
                default:       return false;
            }
        case AF_INET6:
            switch (other.ipAddr_m.ip_family)
            {
                case AF_INET:  return false;
                case AF_INET6: return memcmp(ipAddr_m.ip.v6.s6_addr,
                                             other.ipAddr_m.ip.v6.s6_addr, 16) < 0;
                default:       return false;
            }
        case 0:
            return !(*this == other);
        default:
            return false;
    }
}

bool IpAddress::operator>(const IpAddress& other) const
{
    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
            switch (other.ipAddr_m.ip_family)
            {
                case AF_INET:  return ipAddr_m.ip.v4.s_addr > other.ipAddr_m.ip.v4.s_addr;
                case AF_INET6: return false;
                default:       return true;
            }
        case AF_INET6:
            switch (other.ipAddr_m.ip_family)
            {
                case AF_INET:  return true;
                case AF_INET6: return memcmp(ipAddr_m.ip.v6.s6_addr,
                                             other.ipAddr_m.ip.v6.s6_addr, 16) > 0;
                default:       return true;
            }
        default:
            return false;
    }
}

IpAddress::operator const struct inx_addr& () const { return ipAddr_m; }

IpAddress::operator uint32_t() const
{
    return isIPv4() ? ipAddr_m.ip.v4.s_addr : 0u;
}

IpAddress::operator struct in6_addr() const
{
    return isIPv6() ? ipAddr_m.ip.v6 : in6_addr{};
}

std::string IpAddress::toString() const
{
    if (ipAddr_m.ip_family == AF_INET)
    {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddr_m.ip.v4.s_addr, buf, sizeof(buf));
        return buf;
    }
    if (ipAddr_m.ip_family == AF_INET6)
    {
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, ipAddr_m.ip.v6.s6_addr, buf, sizeof(buf));
        return buf;
    }
    return "Invalid";
}

uint8_t IpAddress::getIpAddrFamily() const { return ipAddr_m.ip_family; }
bool    IpAddress::isIPv4()          const { return ipAddr_m.ip_family == AF_INET; }
bool    IpAddress::isIPv6()          const { return ipAddr_m.ip_family == AF_INET6; }

uint32_t IpAddress::getV4Address() const
{
    return isIPv4() ? ipAddr_m.ip.v4.s_addr : 0u;
}

bool IpAddress::getV4Address(uint32_t& ipV4Addr) const
{
    if (!isIPv4()) return false;
    ipV4Addr = ipAddr_m.ip.v4.s_addr;
    return true;
}

IpAddress::IPv6String IpAddress::getV6Address() const
{
    if (!isIPv6()) return {};
    return IPv6String(ipAddr_m.ip.v6.s6_addr,
                      ipAddr_m.ip.v6.s6_addr + IPV6_LENGTH);
}

bool IpAddress::getV6Address(IPv6String& ipV6Addr) const
{
    ipV6Addr = getV6Address();
    return !ipV6Addr.empty();
}

bool IpAddress::getInAddrV4(struct in_addr& retV4Addr) const
{
    if (!isIPv4()) return false;
    memcpy(&retV4Addr, &ipAddr_m.ip.v4, sizeof(struct in_addr));
    return true;
}

const struct in_addr&  IpAddress::getInAddrV4() const { return ipAddr_m.ip.v4; }

bool IpAddress::getInAddrV6(struct in6_addr& retV6Addr) const
{
    if (!isIPv6()) return false;
    memcpy(&retV6Addr, &ipAddr_m.ip.v6, sizeof(struct in6_addr));
    return true;
}

const struct in6_addr& IpAddress::getInAddrV6() const { return ipAddr_m.ip.v6; }

bool IpAddress::isValid() const
{
    switch (ipAddr_m.ip_family)
    {
        case AF_INET:
            return ipAddr_m.ip.v4.s_addr != 0;
        case AF_INET6:
            for (uint8_t i = 0; i < sizeof(ipAddr_m.ip.v6.s6_addr); ++i)
                if (ipAddr_m.ip.v6.s6_addr[i] != 0) return true;
            return false;
        default:
            return false;
    }
}

bool IpAddress::pton(const std::string& address, uint8_t ip_family)
{
    switch (ip_family)
    {
        case AF_INET:
            if (!inet_pton(AF_INET, address.c_str(), &ipAddr_m.ip.v4))
                return false;
            ipAddr_m.ip_family = AF_INET;
            return true;
        case AF_INET6:
            if (!inet_pton(AF_INET6, address.c_str(), &ipAddr_m.ip.v6))
                return false;
            ipAddr_m.ip_family = AF_INET6;
            return true;
        default:
            return false;
    }
}

IpAddress::IpAddress(const struct sockaddr& addr)
{
    *this = addr;
}

namespace dispatcher {

std::ostream& operator<<(std::ostream& io, const IpAddress& addr)
{
    io << addr.toString();
    return io;
}

std::ostream& operator<<(std::ostream& io, const IpAddrList& addrs)
{
    io << "| ";
    for (uint32_t i = 0; i < addrs.size(); ++i)
        io << addrs[i] << " | ";
    return io;
}

} // namespace dispatcher
