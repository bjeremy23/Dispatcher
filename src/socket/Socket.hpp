#pragma once

#include <boost/asio/awaitable.hpp>
#include <span>
#include <system_error>

namespace dispatcher {

class MsgBuffer;

// Abstract interface for all socket types.
class Socket {
public:
    virtual ~Socket() = default;

    virtual boost::asio::awaitable<std::error_code> read(MsgBuffer& buf)              = 0;
    virtual boost::asio::awaitable<std::error_code> write(std::span<const char> data) = 0;
};

} // namespace dispatcher
