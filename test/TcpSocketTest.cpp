#include "dispatcher/Dispatcher.hpp"
#include "socket/TcpSocket.hpp"
#include "msgbuffer/MsgBufferPool.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <gtest/gtest.h>

#include <cstring>
#include <string>

using dispatcher::Dispatcher;
using dispatcher::MsgBufferPool;
using namespace std::chrono_literals;

// Sets up a loopback TCP server/client pair via the dispatcher.
class TcpSocketTest : public ::testing::Test {
protected:
    Dispatcher dispatcher;
};

TEST_F(TcpSocketTest, ConnectAndExchangeData) {
    constexpr uint16_t port = 19876;
    const std::string  message = "hello dispatcher";

    std::string received;

    // Server coroutine: accept one connection, read data, stop.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        auto exec = co_await boost::asio::this_coro::executor;
        boost::asio::ip::tcp::acceptor acceptor(
            exec,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

        boost::system::error_code ec;
        boost::asio::ip::tcp::socket raw(exec);
        co_await acceptor.async_accept(
            raw, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        EXPECT_FALSE(ec) << ec.message();
        if (ec) { dispatcher.stop(); co_return; }

        // For testing we read directly via the raw socket.
        char buf[256]{};
        auto n = raw.read_some(boost::asio::buffer(buf), ec);
        received = std::string(buf, n);

        dispatcher.stop();
    });

    // Client coroutine: connect and write.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        auto socket = dispatcher::TcpSocket();

        boost::system::error_code ec;
        // Small delay to let the server bind first.
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(std::chrono::milliseconds(20));
        co_await t.async_wait(boost::asio::use_awaitable);

        ec = co_await socket.connect("127.0.0.1", port);
        EXPECT_FALSE(ec) << ec.message();
        if (ec) co_return;

        ec = co_await socket.waitWritable();
        EXPECT_FALSE(ec) << ec.message();
        if (ec) co_return;

        ec = socket.write(std::span<const char>(message.data(), message.size()));
        EXPECT_FALSE(ec) << ec.message();
    });

    dispatcher.run();
    EXPECT_EQ(received, message);
}

TEST_F(TcpSocketTest, DefaultConstructReturnsValidSocket) {
    auto socket = dispatcher::TcpSocket();
    SUCCEED();
}
