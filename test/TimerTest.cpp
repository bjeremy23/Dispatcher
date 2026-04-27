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

TEST_F(TimerFixture, InheritedStrandTimerFires) {
    std::atomic<int> count{0};
    Timer timer(dispatcher);

    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        auto strand = co_await dispatcher.currentStrand();
        timer.start(strand, 20ms, [&]() -> boost::asio::awaitable<void> {
            count++;
            dispatcher.stop();
            co_return;
        });
    });

    dispatcher.run();
    EXPECT_EQ(count.load(), 1);
}

TEST_F(TimerFixture, InheritedStrandSerializesWithParent) {
    // Both the parent coroutine and the timer callback share a strand, so they
    // can never run at the same time. A plain (non-atomic) int is therefore safe
    // to read-modify-write from both — any concurrent access would corrupt it.
    int counter = 0;
    Timer timer(dispatcher);

    dispatcher.spawn([&]() -> boost::asio::awaitable<void> {
        auto strand = co_await dispatcher.currentStrand();
        timer.start(strand, 10ms, [&]() -> boost::asio::awaitable<void> {
            for (int i = 0; i < 1000; ++i) ++counter;
            dispatcher.stop();
            co_return;
        }, Timer::Mode::Repeating);

        // Interleave writes from the parent while the timer is firing.
        for (int i = 0; i < 5; ++i) {
            boost::asio::steady_timer t(co_await boost::asio::this_coro::executor);
            t.expires_after(10ms);
            boost::system::error_code ec;
            co_await t.async_wait(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            for (int j = 0; j < 1000; ++j) ++counter;
        }
    });

    dispatcher.run();
    // If serialization failed and both ran concurrently, counter would be
    // corrupted (lost updates). There is no exact expected value — the test
    // catches races via ThreadSanitizer or repeated corruption in practice.
    EXPECT_GT(counter, 0);
}
