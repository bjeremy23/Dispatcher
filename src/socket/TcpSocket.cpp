#include "TcpSocketDetail.hpp"

#include "../dispatcher/ExecutorContext.hpp"
#include "../msgbuffer/MsgBuffer.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

#include <utility>

namespace dispatcher {

TcpSocket::TcpSocket(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{}

TcpSocket::TcpSocket()
    : impl_(std::make_unique<Impl>(ExecutorContext::get()))
{}

TcpSocket::~TcpSocket() = default;
TcpSocket::TcpSocket(TcpSocket&&) noexcept = default;
TcpSocket& TcpSocket::operator=(TcpSocket&&) noexcept = default;

std::error_code TcpSocket::listen(uint16_t port) {
    boost::system::error_code ec;
    auto& acc = impl_->acceptor.emplace(impl_->socket.get_executor());
    acc.open(boost::asio::ip::tcp::v4(), ec);                              if (ec) return ec;
    acc.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acc.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port), ec); if (ec) return ec;
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

boost::asio::awaitable<std::error_code> TcpSocket::waitReadable() {
    boost::system::error_code ec;
    co_await impl_->socket.async_wait(
        boost::asio::ip::tcp::socket::wait_read,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

boost::asio::awaitable<std::error_code> TcpSocket::waitWritable() {
    boost::system::error_code ec;
    co_await impl_->socket.async_wait(
        boost::asio::ip::tcp::socket::wait_write,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

std::error_code TcpSocket::read(MsgBuffer& buf) {
    boost::system::error_code ec;
    auto n = impl_->socket.read_some(
        boost::asio::buffer(buf.data(), buf.capacity()), ec);
    if (!ec) buf.setSize(n);
    return ec;
}

std::error_code TcpSocket::write(std::span<const char> data) {
    boost::system::error_code ec;
    boost::asio::write(
        impl_->socket,
        boost::asio::buffer(data.data(), data.size()),
        ec);
    return ec;
}

} // namespace dispatcher
