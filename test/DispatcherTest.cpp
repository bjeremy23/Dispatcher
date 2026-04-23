#include "dispatcher/Dispatcher.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

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
    boost::system::error_code cancelEc;

    auto handle = dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(10s);
        co_await t.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, cancelEc));
        dispatcher.stop();
    });

    dispatcher.spawn([&, h = std::move(handle)]() mutable -> boost::asio::awaitable<void> {
        boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
        t.expires_after(10ms);
        co_await t.async_wait(boost::asio::use_awaitable);
        h.cancel();
    });

    dispatcher.run();
    EXPECT_EQ(cancelEc, boost::asio::error::operation_aborted);
}

TEST(DispatcherMultiThreadTest, MultipleThreadsShareWork) {
    Dispatcher dispatcher(4);
    std::atomic<int> count{0};
    std::mutex idMutex;
    std::set<std::thread::id> threadIds;

    for (int i = 0; i < 8; ++i) {
        dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
            {
                std::lock_guard lock(idMutex);
                threadIds.insert(std::this_thread::get_id());
            }
            if (++count == 8) dispatcher.stop();
            co_return;
        });
    }

    dispatcher.run();
    EXPECT_EQ(count, 8);

    std::cout << "[   INFO   ] Coroutines ran on " << threadIds.size() << " thread(s):\n";
    for (auto id : threadIds)
        std::cout << "[   INFO   ]   " << id << "\n";
}
