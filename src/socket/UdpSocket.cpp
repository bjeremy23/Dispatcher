#include "UdpSocket.hpp"

#include "../dispatcher/ExecutorContext.hpp"
#include "../msgbuffer/MsgBuffer.hpp"

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace dispatcher {

struct UdpSocket::Impl {
    explicit Impl(boost::asio::any_io_executor exec)
        : socket(std::move(exec), boost::asio::ip::udp::v4())
    {}

    boost::asio::ip::udp::socket socket;
};

UdpSocket::UdpSocket()
    : impl_(std::make_unique<Impl>(ExecutorContext::get()))
{}

UdpSocket::~UdpSocket() = default;
UdpSocket::UdpSocket(UdpSocket&&) noexcept = default;
UdpSocket& UdpSocket::operator=(UdpSocket&&) noexcept = default;

std::error_code UdpSocket::bind(uint16_t port) {
    boost::system::error_code ec;
    impl_->socket.bind(
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port), ec);
    return ec;
}

boost::asio::awaitable<std::error_code> UdpSocket::waitReadable() {
    boost::system::error_code ec;
    co_await impl_->socket.async_wait(
        boost::asio::ip::udp::socket::wait_read,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

boost::asio::awaitable<std::error_code> UdpSocket::waitWritable() {
    boost::system::error_code ec;
    co_await impl_->socket.async_wait(
        boost::asio::ip::udp::socket::wait_write,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

std::error_code UdpSocket::read(MsgBuffer& buf) {
    boost::system::error_code ec;
    auto n = impl_->socket.receive(
        boost::asio::buffer(buf.data(), buf.capacity()), 0, ec);
    if (!ec) buf.setSize(n);
    return ec;
}

std::error_code UdpSocket::write(std::span<const char> data) {
    boost::system::error_code ec;
    impl_->socket.send(
        boost::asio::buffer(data.data(), data.size()), 0, ec);
    return ec;
}

std::error_code UdpSocket::receiveFrom(MsgBuffer& buf, NetworkEndpoint& sender) {
    boost::asio::ip::udp::endpoint ep;
    boost::system::error_code ec;

    auto n = impl_->socket.receive_from(
        boost::asio::buffer(buf.data(), buf.capacity()), ep, 0, ec);

    if (!ec) {
        buf.setSize(n);
        sender.address = IpAddress(ep.address().to_string(),
                                   ep.address().is_v4() ? AF_INET : AF_INET6);
        sender.port    = ep.port();
    }
    return ec;
}

std::error_code UdpSocket::sendTo(std::span<const char> data, const NetworkEndpoint& target) {
    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address(target.address.toString(), ec);
    if (ec) return ec;

    boost::asio::ip::udp::endpoint ep(addr, target.port);
    impl_->socket.send_to(
        boost::asio::buffer(data.data(), data.size()), ep, 0, ec);
    return ec;
}

} // namespace dispatcher
