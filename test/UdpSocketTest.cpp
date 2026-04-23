#include "dispatcher/Dispatcher.hpp"
#include "socket/UdpSocket.hpp"
#include "msgbuffer/MsgBufferPool.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <gtest/gtest.h>

#include <cstring>
#include <string>

using dispatcher::Dispatcher;
using dispatcher::MsgBufferPool;
using dispatcher::NetworkEndpoint;

class UdpSocketTest : public ::testing::Test {
protected:
    Dispatcher   dispatcher;
    MsgBufferPool pool{4};
};

TEST_F(UdpSocketTest, SendAndReceiveLoopback) {
    constexpr uint16_t serverPort = 19877;
    const std::string  message    = "udp hello";

    std::string received;

    // Receiver coroutine.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        auto server = dispatcher::UdpSocket();
        auto ec     = server.bind(serverPort);
        EXPECT_FALSE(ec) << ec.message();
        if (ec) { dispatcher.stop(); co_return; }

        ec = co_await server.waitReadable();
        EXPECT_FALSE(ec) << ec.message();
        if (ec) { dispatcher.stop(); co_return; }

        auto buf = pool.acquire();
        NetworkEndpoint sender;
        ec = server.receiveFrom(*buf, sender);
        EXPECT_FALSE(ec) << ec.message();

        received = std::string(buf->data(), buf->size());
        dispatcher.stop();
    });

    // Sender coroutine.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        // Small delay to ensure the server binds first.
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(std::chrono::milliseconds(20));
        co_await t.async_wait(boost::asio::use_awaitable);

        auto client = dispatcher::UdpSocket();
        auto ec     = co_await client.waitWritable();
        EXPECT_FALSE(ec) << ec.message();
        if (ec) co_return;

        NetworkEndpoint target{dispatcher::IpAddress("127.0.0.1", AF_INET), serverPort};
        ec = client.sendTo(
            std::span<const char>(message.data(), message.size()), target);
        EXPECT_FALSE(ec) << ec.message();
    });

    dispatcher.run();
    EXPECT_EQ(received, message);
}

TEST_F(UdpSocketTest, DefaultConstructReturnsValidSocket) {
    auto socket = dispatcher::UdpSocket();
    SUCCEED();
}
