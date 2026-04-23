#pragma once

#include <stdint.h>
#include <netinet/in.h>

struct inx_addr
{
    union
    {
        struct in_addr v4;
        struct in6_addr v6;
    } ip;

    uint8_t ip_family; /* IP Family: Invalid    = 0,
                       v4 Address = AF_INET,
                       v6 Address = AF_INET6 */

#define ipv6_inaddr    ip.v6
#define ipv6_address   ip.v6.s6_addr
#define ipv6_address16 ip.v6.s6_addr16
#define ipv6_address32 ip.v6.s6_addr32
#define ipv4_inaddr    ip.v4
#define ipv4_address   ip.v4.s_addr
};

// Undefine the struct-member macros so they don't leak into Boost.Asio headers.
#undef ipv6_inaddr
#undef ipv6_address
#undef ipv6_address16
#undef ipv6_address32
#undef ipv4_inaddr
#undef ipv4_address

