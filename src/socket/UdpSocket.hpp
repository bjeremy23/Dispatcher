#pragma once

#include "NetworkEndpoint.hpp"

#include <boost/asio/awaitable.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <system_error>

namespace dispatcher {

class MsgBuffer;

class UdpSocket {
public:
    // Client side: reads the executor from ExecutorContext — a Dispatcher must exist first.
    UdpSocket();
    // Server side: binds to the given endpoint when bind() is called.
    explicit UdpSocket(NetworkEndpoint localEndpoint);
    ~UdpSocket();

    UdpSocket(UdpSocket&&) noexcept;
    UdpSocket& operator=(UdpSocket&&) noexcept;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Bind to the local endpoint from the constructor.
    std::error_code bind();

    // Receive a datagram and record the sender's address.
    boost::asio::awaitable<std::error_code> receiveFrom(MsgBuffer& buf, NetworkEndpoint& sender);

    // Send a datagram to an explicit destination.
    boost::asio::awaitable<std::error_code> sendTo(std::span<const char> data, const NetworkEndpoint& target);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
