#pragma once

#include "../dispatcher/Dispatcher.hpp"

#include <boost/asio/awaitable.hpp>

#include <chrono>
#include <functional>
#include <memory>

namespace dispatcher {

class Dispatcher;

// Fires a coroutine factory after a delay. Supports one-shot and repeating
// modes. Call stop() or destroy the Timer to cancel a running timer.
class Timer {
public:
    enum class Mode { OneShot, Repeating };
    using Factory = std::function<boost::asio::awaitable<void>()>;

    explicit Timer(Dispatcher& dispatcher);
    ~Timer();

    Timer(Timer&&) noexcept;
    Timer& operator=(Timer&&) noexcept;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    // Starts the timer on a new strand, cancelling any previously running one.
    void start(std::chrono::steady_clock::duration interval,
               Factory factory,
               Mode mode = Mode::OneShot);

    // Starts the timer on an inherited strand for synchronization with the caller.
    void start(Dispatcher::Strand strand,
               std::chrono::steady_clock::duration interval,
               Factory factory,
               Mode mode = Mode::OneShot);

    void stop();
    bool running() const;

private:
    void startOn(Dispatcher::Strand strand,
                 std::chrono::steady_clock::duration interval,
                 Factory factory,
                 Mode mode);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
