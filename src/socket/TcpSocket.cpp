#include "TcpSocket.hpp"

#include "../dispatcher/ExecutorContext.hpp"
#include "../msgbuffer/MsgBuffer.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

#include <optional>
#include <utility>

namespace dispatcher {

struct TcpSocket::Impl {
    explicit Impl(boost::asio::any_io_executor exec)
        : socket(std::move(exec))
    {}

    explicit Impl(boost::asio::any_io_executor exec, NetworkEndpoint ep)
        : socket(std::move(exec)), localEndpoint(std::move(ep))
    {}

    explicit Impl(boost::asio::ip::tcp::socket s)
        : socket(std::move(s))
    {}

    boost::asio::ip::tcp::socket                    socket;
    std::optional<boost::asio::ip::tcp::acceptor>   acceptor;
    std::optional<NetworkEndpoint>                  localEndpoint;
};

TcpSocket::TcpSocket(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{}

TcpSocket::TcpSocket()
    : impl_(std::make_unique<Impl>(ExecutorContext::get()))
{}

TcpSocket::TcpSocket(NetworkEndpoint localEndpoint)
    : impl_(std::make_unique<Impl>(ExecutorContext::get(), std::move(localEndpoint)))
{}

TcpSocket::~TcpSocket() = default;
TcpSocket::TcpSocket(TcpSocket&&) noexcept = default;
TcpSocket& TcpSocket::operator=(TcpSocket&&) noexcept = default;

std::error_code TcpSocket::listen() {
    boost::system::error_code ec;
    auto asioEp = boost::asio::ip::tcp::endpoint(
        impl_->localEndpoint->toAsioAddress(), impl_->localEndpoint->port);
    auto& acc = impl_->acceptor.emplace(impl_->socket.get_executor());

    acc.open(asioEp.protocol(), ec);                                       
    if (ec) return ec;

    acc.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

    acc.bind(asioEp, ec);                                                  
    if (ec) return ec;

    acc.listen(boost::asio::socket_base::max_listen_connections, ec);
    return ec;
}

boost::asio::awaitable<std::pair<TcpSocket, std::error_code>> TcpSocket::accept() {
    boost::system::error_code ec;
    boost::asio::ip::tcp::socket raw(impl_->acceptor->get_executor());
    
    co_await impl_->acceptor->async_accept(
        raw, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

    if (ec)
        co_return std::pair{TcpSocket{}, std::error_code{ec}};

    co_return std::pair{TcpSocket(std::make_unique<Impl>(std::move(raw))), std::error_code{}};
}

boost::asio::awaitable<std::error_code>
TcpSocket::connect(const NetworkEndpoint& endpoint) {
    boost::system::error_code ec;
    auto asioEp = boost::asio::ip::tcp::endpoint(endpoint.toAsioAddress(), endpoint.port);
    impl_->socket.open(asioEp.protocol(), ec);
    if (ec) co_return ec;
    co_await impl_->socket.async_connect(
        asioEp,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

boost::asio::awaitable<std::error_code>
TcpSocket::connect(std::string_view host, uint16_t port) {
    boost::asio::ip::tcp::resolver resolver(impl_->socket.get_executor());
    boost::system::error_code ec;

    auto endpoints = co_await resolver.async_resolve(
        host, std::to_string(port),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));

    if (ec) co_return ec;

    co_await boost::asio::async_connect(
        impl_->socket, endpoints,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));

    co_return ec;
}

boost::asio::awaitable<std::error_code> TcpSocket::read(MsgBuffer& buf) {
    boost::system::error_code ec;
    auto n = co_await impl_->socket.async_read_some(
        boost::asio::buffer(buf.data(), buf.capacity()),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (!ec) buf.setSize(n);
    co_return ec;
}

boost::asio::awaitable<std::error_code> TcpSocket::write(std::span<const char> data) {
    boost::system::error_code ec;
    co_await boost::asio::async_write(
        impl_->socket,
        boost::asio::buffer(data.data(), data.size()),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

} // namespace dispatcher
