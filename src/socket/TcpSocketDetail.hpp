#pragma once

#include "TcpSocket.hpp"
#include <boost/asio/ip/tcp.hpp>

#include <optional>

namespace dispatcher {

struct TcpSocket::Impl {
    explicit Impl(boost::asio::any_io_executor exec)
        : socket(std::move(exec))
    {}

    explicit Impl(boost::asio::ip::tcp::socket s)
        : socket(std::move(s))
    {}

    boost::asio::ip::tcp::socket                    socket;
    std::optional<boost::asio::ip::tcp::acceptor>   acceptor;
};

} // namespace dispatcher
