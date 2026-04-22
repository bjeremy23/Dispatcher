#pragma once

#include "Socket.hpp"
#include "NetworkEndpoint.hpp"

#include <cstdint>
#include <memory>

namespace dispatcher {

class UdpSocket : public Socket {
public:
    // Reads the executor from ExecutorContext — a Dispatcher must exist first.
    UdpSocket();
    ~UdpSocket() override;

    UdpSocket(UdpSocket&&) noexcept;
    UdpSocket& operator=(UdpSocket&&) noexcept;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Bind to a local port for receiving datagrams.
    std::error_code bind(uint16_t port);

    // Socket interface (send/receive without explicit endpoint).
    boost::asio::awaitable<std::error_code> waitReadable() override;
    boost::asio::awaitable<std::error_code> waitWritable() override;
    std::error_code read(MsgBuffer& buf)               override;
    std::error_code write(std::span<const char> data)  override;

    // UDP-specific: receive and record the sender's address.
    std::error_code receiveFrom(MsgBuffer& buf, NetworkEndpoint& sender);

    // UDP-specific: send to an explicit destination.
    std::error_code sendTo(std::span<const char> data, const NetworkEndpoint& target);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
