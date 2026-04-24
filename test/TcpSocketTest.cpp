#include "dispatcher/Dispatcher.hpp"
#include "socket/TcpSocket.hpp"
#include "msgbuffer/MsgBufferPool.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <gtest/gtest.h>

#include <string>

using dispatcher::Dispatcher;
using dispatcher::MsgBufferPool;
using namespace std::chrono_literals;

class TcpSocketTest : public ::testing::Test {
protected:
    Dispatcher    dispatcher;
    MsgBufferPool pool{4};
};

TEST_F(TcpSocketTest, ConnectAndExchangeData) {
    constexpr uint16_t port = 19876;
    const std::string  message = "hello dispatcher";

    std::string received;

    // Server coroutine: listen, accept one connection, read data, stop.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        dispatcher::TcpSocket server(dispatcher::NetworkEndpoint{dispatcher::IpAddress{uint32_t(INADDR_ANY)}, port});
        auto ec = server.listen();
        EXPECT_FALSE(ec) << ec.message();
        if (ec) { dispatcher.stop(); co_return; }

        auto [client, acceptEc] = co_await server.accept();
        EXPECT_FALSE(acceptEc) << acceptEc.message();
        if (acceptEc) { dispatcher.stop(); co_return; }

        auto buf = pool.acquire();
        acceptEc = co_await client.read(*buf);
        EXPECT_FALSE(acceptEc) << acceptEc.message();
        received = std::string(buf->data(), buf->size());

        dispatcher.stop();
    });

    // Client coroutine: connect and write.
    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        // Small delay to let the server bind first.
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(std::chrono::milliseconds(20));
        co_await t.async_wait(boost::asio::use_awaitable);

        dispatcher::TcpSocket client;
        auto ec = co_await client.connect("127.0.0.1", port);
        EXPECT_FALSE(ec) << ec.message();
        if (ec) co_return;

        ec = co_await client.write(std::span<const char>(message.data(), message.size()));
        EXPECT_FALSE(ec) << ec.message();
    });

    dispatcher.run();
    EXPECT_EQ(received, message);
}

TEST_F(TcpSocketTest, DefaultConstructReturnsValidSocket) {
    auto socket = dispatcher::TcpSocket();
    SUCCEED();
}
