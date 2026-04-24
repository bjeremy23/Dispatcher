#include "dispatcher/Dispatcher.hpp"
#include "signaler/Signaler.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

using dispatcher::Dispatcher;
using dispatcher::Signaler;
using namespace std::chrono_literals;

struct SignalerFixture : public ::testing::Test {
    Dispatcher dispatcher;

    void runUntilStopped() {
        std::thread t([this] { dispatcher.run(); });
        t.join();
    }
};

TEST_F(SignalerFixture, DefaultCallbackStopsDispatcher) {
    Signaler signaler(dispatcher);
    signaler.add(SIGUSR1);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join(); // dispatcher.stop() called by default callback
}

TEST_F(SignalerFixture, CustomCallbackInvoked) {
    std::atomic<int> count{0};
    Signaler signaler(dispatcher, [&] { count++; dispatcher.stop(); });
    signaler.add(SIGUSR1);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(SignalerFixture, MultipleSignalsHandled) {
    std::atomic<int> count{0};
    Signaler signaler(dispatcher, [&] {
        if (++count >= 2) dispatcher.stop();
    });
    signaler.add(SIGUSR1);
    signaler.add(SIGUSR2);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    std::this_thread::sleep_for(10ms);
    std::raise(SIGUSR2);
    t.join();

    EXPECT_GE(count.load(), 2);
}

TEST_F(SignalerFixture, RemoveSignalStopsHandlingIt) {
    std::atomic<bool> called{false};
    // Keep dispatcher alive long enough with SIGUSR2; SIGUSR1 removed before raise
    Signaler signaler(dispatcher, [&] { called = true; dispatcher.stop(); });
    signaler.add(SIGUSR1);
    signaler.add(SIGUSR2);
    signaler.remove(SIGUSR1);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    // SIGUSR1 is removed — raise SIGUSR2 to cleanly stop
    std::raise(SIGUSR2);
    t.join();

    EXPECT_TRUE(called.load());
}
