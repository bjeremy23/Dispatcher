#pragma once

#include <cstdint>
#include <string>

namespace dispatcher {

// A transport-agnostic network address. Keeps Boost.Asio endpoint types
// out of the public socket API.
struct NetworkEndpoint {
    std::string address; // Dotted-decimal IPv4 or IPv6 string, e.g. "127.0.0.1"
    uint16_t    port{0};
};

} // namespace dispatcher
