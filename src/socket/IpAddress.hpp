#pragma once

#include "InxAddr.h"
#include <boost/functional/hash.hpp>
#include <iosfwd>
#include <vector>

namespace dispatcher
{

class IpAddress
{
  public:
    static const uint32_t IPV6_LENGTH = 16;

    typedef std::vector<uint8_t> IPv6String;

    IpAddress();
    IpAddress(const struct inx_addr& in);
    IpAddress(uint32_t v4addr);
    IpAddress(const struct in_addr& v4addr);
    IpAddress(const IPv6String& v6addr);
    IpAddress(const struct in6_addr& in);
    IpAddress(const struct sockaddr& in);
    IpAddress(const IpAddress& other);
    IpAddress(const std::string& address, uint8_t ip_family);

    virtual ~IpAddress();

    IpAddress& operator=(uint32_t v4addr);
    IpAddress& operator=(const struct in_addr& v4addr);
    IpAddress& operator=(const IPv6String& v6addr);
    IpAddress& operator=(const struct in6_addr& v6addr);
    IpAddress& operator=(const IpAddress& other);
    IpAddress& operator=(const struct sockaddr& addr);

    bool operator==(uint32_t v4addr) const;
    bool operator==(int32_t v4addr) const;
    bool operator==(const IPv6String& v6addr) const;
    bool operator==(const IpAddress& other) const;
    bool operator==(const struct in_addr& other) const;
    bool operator==(const struct in6_addr& other) const;
    bool operator==(const struct inx_addr& other) const;

    bool operator!=(uint32_t v4addr) const;
    bool operator!=(int32_t v4addr) const;
    bool operator!=(const IPv6String& v6addr) const;
    bool operator!=(const IpAddress& other) const;

    bool operator<(const IpAddress& other) const;
    bool operator>(const IpAddress& other) const;

    operator const struct inx_addr& () const;
    operator uint32_t() const;
    operator struct in6_addr() const;

    std::string toString() const;
    uint8_t     getIpAddrFamily() const;
    bool        isIPv4() const;
    bool        isIPv6() const;
    bool        isValid() const;

    uint32_t   getV4Address() const;
    bool       getV4Address(uint32_t& ipV4Addr) const;
    IPv6String getV6Address() const;
    bool       getV6Address(IPv6String& ipV6Addr) const;

    bool                  getInAddrV4(struct in_addr& retV4Addr) const;
    const struct in_addr& getInAddrV4() const;
    bool                  getInAddrV6(struct in6_addr& retV6Addr) const;
    const struct in6_addr& getInAddrV6() const;

    friend std::ostream& operator<<(std::ostream& io, const IpAddress& addr);

  private:
    bool pton(const std::string& address, uint8_t ip_family);

    inx_addr ipAddr_m;
};

typedef std::vector<IpAddress> IpAddrList;

std::ostream& operator<<(std::ostream& io, const IpAddrList& addrs);

inline std::size_t hash_value(const IpAddress& addr)
{
    if (addr.isIPv4())
    {
        boost::hash<int> hasher;
        return hasher(addr.getInAddrV4().s_addr);
    }
    else if (addr.isIPv6())
    {
        boost::hash<uint8_t[IpAddress::IPV6_LENGTH]> array_hasher;
        return array_hasher(addr.getInAddrV6().s6_addr);
    }
    return 0;
}

} // namespace base

