#include "TcpSocket.hpp"

#include "../dispatcher/ExecutorContext.hpp"
#include "../msgbuffer/MsgBuffer.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

namespace dispatcher {

struct TcpSocket::Impl {
    explicit Impl(boost::asio::any_io_executor exec)
        : socket(std::move(exec))
    {}

    boost::asio::ip::tcp::socket socket;
};

TcpSocket::TcpSocket()
    : impl_(std::make_unique<Impl>(ExecutorContext::get()))
{}

TcpSocket::~TcpSocket() = default;
TcpSocket::TcpSocket(TcpSocket&&) noexcept = default;
TcpSocket& TcpSocket::operator=(TcpSocket&&) noexcept = default;

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
