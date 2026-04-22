#pragma once

#include "Socket.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace dispatcher {

class TcpSocket : public Socket {
public:
    // Reads the executor from ExecutorContext — a Dispatcher must exist first.
    TcpSocket();
    ~TcpSocket() override;

    TcpSocket(TcpSocket&&) noexcept;
    TcpSocket& operator=(TcpSocket&&) noexcept;
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    // Async connect (client side).
    boost::asio::awaitable<std::error_code> connect(std::string_view host, uint16_t port);

    // Socket interface.
    boost::asio::awaitable<std::error_code> waitReadable() override;
    boost::asio::awaitable<std::error_code> waitWritable() override;
    std::error_code read(MsgBuffer& buf)               override;
    std::error_code write(std::span<const char> data)  override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
