#pragma once

#include "NetworkEndpoint.hpp"
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
    // Client side: reads the executor from ExecutorContext — a Dispatcher must exist first.
    TcpSocket();
    // Server side: binds to the given endpoint when listen() is called.
    explicit TcpSocket(NetworkEndpoint localEndpoint);
    ~TcpSocket() override;

    TcpSocket(TcpSocket&&) noexcept;
    TcpSocket& operator=(TcpSocket&&) noexcept;
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    // Server side: bind and listen using the endpoint from the constructor.
    std::error_code listen();
    boost::asio::awaitable<std::pair<TcpSocket, std::error_code>> accept();

    // Client side: async connect to a remote host (DNS resolved) or a resolved address.
    boost::asio::awaitable<std::error_code> connect(std::string_view host, uint16_t port);
    boost::asio::awaitable<std::error_code> connect(const NetworkEndpoint& endpoint);

    // Socket interface.
    boost::asio::awaitable<std::error_code> read(MsgBuffer& buf)              override;
    boost::asio::awaitable<std::error_code> write(std::span<const char> data) override;

private:
    struct Impl;
    explicit TcpSocket(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
