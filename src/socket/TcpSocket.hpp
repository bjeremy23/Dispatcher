#pragma once

#include "Socket.hpp"

#include <boost/asio/awaitable.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

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

    // Server side: bind and listen on a port, then accept one connection.
    std::error_code listen(uint16_t port);
    boost::asio::awaitable<std::pair<TcpSocket, std::error_code>> accept();

    // Client side: async connect to a remote host.
    boost::asio::awaitable<std::error_code> connect(std::string_view host, uint16_t port);

    // Socket interface.
    boost::asio::awaitable<std::error_code> waitReadable() override;
    boost::asio::awaitable<std::error_code> waitWritable() override;
    std::error_code read(MsgBuffer& buf)              override;
    std::error_code write(std::span<const char> data) override;

private:
    struct Impl;
    explicit TcpSocket(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
