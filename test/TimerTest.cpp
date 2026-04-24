#include "dispatcher/Dispatcher.hpp"
#include "timer/Timer.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using dispatcher::Dispatcher;
using dispatcher::Timer;
using namespace std::chrono_literals;

struct TimerFixture : public ::testing::Test {
    Dispatcher dispatcher;

    // Runs the dispatcher on a background thread; stops after ms.
    void runFor(std::chrono::milliseconds ms) {
        std::thread t([this, ms] {
            dispatcher.spawn([this, ms]() -> boost::asio::awaitable<void> {
                boost::asio::steady_timer guard(co_await boost::asio::this_coro::executor);
                guard.expires_after(ms);
                boost::system::error_code ec;
                co_await guard.async_wait(
                    boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                dispatcher.stop();
            });
            dispatcher.run();
        });
        t.join();
    }
};

TEST_F(TimerFixture, OneShotFiresOnce) {
    std::atomic<int> count{0};
    Timer timer(dispatcher);
    timer.start(20ms, [&]() -> boost::asio::awaitable<void> {
        count++;
        co_return;
    });

    runFor(150ms);
    EXPECT_EQ(count.load(), 1);
}

TEST_F(TimerFixture, RepeatingFiresMultipleTimes) {
    std::atomic<int> count{0};
    Timer timer(dispatcher);
    timer.start(20ms, [&]() -> boost::asio::awaitable<void> {
        count++;
        co_return;
    }, Timer::Mode::Repeating);

    runFor(150ms);
    EXPECT_GE(count.load(), 3);
}

TEST_F(TimerFixture, StopCancelsTimer) {
    std::atomic<int> count{0};
    Timer timer(dispatcher);
    timer.start(100ms, [&]() -> boost::asio::awaitable<void> {
        count++;
        co_return;
    });

    // Stop before it fires
    timer.stop();
    runFor(200ms);
    EXPECT_EQ(count.load(), 0);
}

TEST_F(TimerFixture, RunningReflectsState) {
    Timer timer(dispatcher);
    EXPECT_FALSE(timer.running());

    timer.start(500ms, []() -> boost::asio::awaitable<void> { co_return; });
    EXPECT_TRUE(timer.running());

    timer.stop();
    EXPECT_FALSE(timer.running());
}

TEST_F(TimerFixture, RestartCancelsAndRestartsTimer) {
    std::atomic<int> count{0};
    Timer timer(dispatcher);

    // Start with a long delay, then restart with a short one
    timer.start(500ms, [&]() -> boost::asio::awaitable<void> {
        count += 10; // should not fire
        co_return;
    });
    timer.start(20ms, [&]() -> boost::asio::awaitable<void> {
        count++;
        co_return;
    });

    runFor(150ms);
    EXPECT_EQ(count.load(), 1);
}

TEST_F(TimerFixture, FactoryCaptures) {
    int value = 42;
    std::atomic<int> captured{0};
    Timer timer(dispatcher);
    timer.start(20ms, [&, value]() -> boost::asio::awaitable<void> {
        captured = value;
        co_return;
    });

    runFor(100ms);
    EXPECT_EQ(captured.load(), 42);
}
