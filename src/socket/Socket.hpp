#pragma once

#include <boost/asio/awaitable.hpp>
#include <span>
#include <system_error>

namespace dispatcher {

class MsgBuffer;

// Abstract interface for all socket types.
// co_await waitReadable() / waitWritable() before calling read() / write().
class Socket {
public:
    virtual ~Socket() = default;

    // Signal readiness; returns an error code on failure (e.g. connection reset).
    virtual boost::asio::awaitable<std::error_code> waitReadable() = 0;
    virtual boost::asio::awaitable<std::error_code> waitWritable() = 0;

    // Synchronous after the corresponding wait has resolved.
    virtual std::error_code read(MsgBuffer& buf)           = 0;
    virtual std::error_code write(std::span<const char> data) = 0;
};

} // namespace dispatcher
