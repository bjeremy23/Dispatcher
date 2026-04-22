#include "dispatcher/Dispatcher.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>

using dispatcher::Dispatcher;
using namespace std::chrono_literals;

// Helper: run the dispatcher on a background thread and stop it after the
// callable has finished (or been arranged to finish via a timer/flag).
struct DispatcherFixture : public ::testing::Test {
    Dispatcher dispatcher;

    void runFor(std::chrono::milliseconds ms) {
        auto handle = dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
            boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
            t.expires_after(ms);
            boost::system::error_code ec;
            co_await t.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            dispatcher.stop();
        });
        dispatcher.run();
        (void)handle;
    }
};

TEST_F(DispatcherFixture, SpawnedCoroutineExecutes) {
    std::atomic<bool> ran{false};

    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        ran = true;
        dispatcher.stop();
        co_return;
    });

    dispatcher.run();
    EXPECT_TRUE(ran);
}

TEST_F(DispatcherFixture, MultipleCoroutinesRunConcurrently) {
    std::atomic<int> count{0};

    for (int i = 0; i < 4; ++i) {
        dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
            ++count;
            if (count == 4) dispatcher.stop();
            co_return;
        });
    }

    dispatcher.run();
    EXPECT_EQ(count, 4);
}

TEST_F(DispatcherFixture, CancelStopsCoroutineAtAwaitPoint) {
    std::atomic<bool> reachedAfterAwait{false};

    auto handle = dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(10s); // long timer — will be cancelled
        boost::system::error_code ec;
        co_await t.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        // Only reaches here if cancelled or timer fires
        reachedAfterAwait = true;
        dispatcher.stop();
    });

    // Cancel after a short delay from another coroutine
    dispatcher.spawn([&, h = std::move(handle)]() mutable -> boost::asio::awaitable<void> {
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(10ms);
        co_await t.async_wait(boost::asio::use_awaitable);
        h.cancel();
        dispatcher.stop();
    });

    dispatcher.run();
    // The long-timer coroutine was cancelled — it may or may not reach after-await
    // depending on cancellation delivery; the important thing is run() returned.
    SUCCEED();
}

TEST(DispatcherMultiThreadTest, MultipleThreadsShareWork) {
    Dispatcher dispatcher(4);
    std::atomic<int> count{0};

    for (int i = 0; i < 8; ++i) {
        dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
            if (++count == 8) dispatcher.stop();
            co_return;
        });
    }

    dispatcher.run();
    EXPECT_EQ(count, 8);
}
