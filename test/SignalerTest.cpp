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
};

TEST_F(SignalerFixture, DefaultCallbackStopsDispatcher) {
    Signaler signaler(dispatcher);
    signaler.add({SIGUSR1});

    std::atomic<bool> ran{false};
    std::thread t([this, &ran] { dispatcher.run(); ran = true; });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join();

    EXPECT_TRUE(ran);
}

TEST_F(SignalerFixture, CustomCallbackInvoked) {
    std::atomic<int> count{0};
    Signaler signaler(dispatcher);
    signaler.add({SIGUSR1}, [&] { count++; dispatcher.stop(); });

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(SignalerFixture, SeparateCallbacksPerSignal) {
    std::atomic<int> usr1Count{0}, usr2Count{0};
    Signaler signaler(dispatcher);
    signaler.add({SIGUSR1}, [&] { usr1Count++; });
    signaler.add({SIGUSR2}, [&] { usr2Count++; dispatcher.stop(); });

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    std::this_thread::sleep_for(10ms);
    std::raise(SIGUSR2);
    t.join();

    EXPECT_EQ(usr1Count.load(), 1);
    EXPECT_EQ(usr2Count.load(), 1);
}

TEST_F(SignalerFixture, MultipleSignalsSameCallback) {
    std::atomic<int> count{0};
    Signaler signaler(dispatcher);
    signaler.add({SIGUSR1, SIGUSR2}, [&] {
        if (++count >= 2) dispatcher.stop();
    });

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    std::this_thread::sleep_for(10ms);
    std::raise(SIGUSR2);
    t.join();

    EXPECT_EQ(count.load(), 2);
}

TEST_F(SignalerFixture, RemoveSignalStopsHandlingIt) {
    std::atomic<bool> called{false};
    Signaler signaler(dispatcher);
    auto id = signaler.add({SIGUSR1}, [&] { called = true; });
    signaler.add({SIGUSR2}, [&] { dispatcher.stop(); });
    signaler.remove(id);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR2);
    t.join();

    EXPECT_FALSE(called.load());
}

TEST_F(SignalerFixture, MultipleCallbacksSameSignal) {
    std::atomic<int> countA{0}, countB{0};
    Signaler signaler(dispatcher);
    signaler.add({SIGUSR1}, [&] { countA++; });
    signaler.add({SIGUSR1}, [&] { countB++; dispatcher.stop(); });

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join();

    EXPECT_EQ(countA.load(), 1);
    EXPECT_EQ(countB.load(), 1);
}

TEST_F(SignalerFixture, RemoveOneSubscriberLeavesOthers) {
    std::atomic<bool> calledA{false}, calledB{false};
    Signaler signaler(dispatcher);
    auto idA = signaler.add({SIGUSR1}, [&] { calledA = true; });
    signaler.add({SIGUSR1}, [&] { calledB = true; dispatcher.stop(); });
    signaler.remove(idA);

    std::thread t([this] { dispatcher.run(); });

    std::this_thread::sleep_for(20ms);
    std::raise(SIGUSR1);
    t.join();

    EXPECT_FALSE(calledA.load());
    EXPECT_TRUE(calledB.load());
}
