#pragma once

#include "IpAddress.hpp"
#include <cstdint>

namespace dispatcher {

struct NetworkEndpoint {
    IpAddress address;
    uint16_t  port{0};
};

} // namespace dispatcher
