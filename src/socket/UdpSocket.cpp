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

    explicit Impl(boost::asio::any_io_executor exec, NetworkEndpoint ep)
        : socket(std::move(exec), boost::asio::ip::udp::v4()), localEndpoint(std::move(ep))
    {}

    boost::asio::ip::udp::socket    socket;
    std::optional<NetworkEndpoint>  localEndpoint;
};

UdpSocket::UdpSocket()
    : impl_(std::make_unique<Impl>(ExecutorContext::get()))
{}

UdpSocket::UdpSocket(NetworkEndpoint localEndpoint)
    : impl_(std::make_unique<Impl>(ExecutorContext::get(), std::move(localEndpoint)))
{}

UdpSocket::~UdpSocket() = default;
UdpSocket::UdpSocket(UdpSocket&&) noexcept = default;
UdpSocket& UdpSocket::operator=(UdpSocket&&) noexcept = default;

std::error_code UdpSocket::bind() {
    boost::system::error_code ec;
    auto proto = impl_->localEndpoint->address.isIPv6() ? boost::asio::ip::udp::v6()
                                                        : boost::asio::ip::udp::v4();
    impl_->socket.close(ec);                                    
    if (ec) return ec;

    impl_->socket.open(proto, ec);                             
     if (ec) return ec;

    impl_->socket.bind(
        boost::asio::ip::udp::endpoint(
            impl_->localEndpoint->toAsioAddress(), impl_->localEndpoint->port), ec);

    return ec;
}

boost::asio::awaitable<std::error_code> UdpSocket::receiveFrom(MsgBuffer& buf, NetworkEndpoint& sender) {
    boost::asio::ip::udp::endpoint ep;
    boost::system::error_code ec;
    auto n = co_await impl_->socket.async_receive_from(
        boost::asio::buffer(buf.data(), buf.capacity()), ep,
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (!ec) {
        buf.setSize(n);
        sender.address = IpAddress(ep.address().to_string(),
                                   ep.address().is_v4() ? AF_INET : AF_INET6);
        sender.port    = ep.port();
    }
    co_return ec;
}

boost::asio::awaitable<std::error_code> UdpSocket::sendTo(std::span<const char> data, const NetworkEndpoint& target) {
    boost::system::error_code ec;
    co_await impl_->socket.async_send_to(
        boost::asio::buffer(data.data(), data.size()),
        boost::asio::ip::udp::endpoint(target.toAsioAddress(), target.port),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    co_return ec;
}

} // namespace dispatcher
