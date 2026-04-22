#pragma once

#include "CoroutineHandle.hpp"

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <cstddef>
#include <memory>

namespace dispatcher {

class Dispatcher {
public:
    // Registers this dispatcher's executor in ExecutorContext so that
    // sockets can be constructed without an explicit Dispatcher reference.
    // threadCount = 1 : single-threaded; call run() to drive the loop.
    // threadCount > 1 : internal thread pool; run() participates as one thread.
    explicit Dispatcher(std::size_t threadCount = 1);
    ~Dispatcher();

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    // Launch a coroutine factory (callable returning awaitable<void>).
    // Returns a handle for cancellation.
    // Each coroutine runs on its own strand for thread safety.
    template<typename Factory>
    CoroutineHandle spawn(Factory&& factory) {
        CoroutineHandle handle;
        // sig is captured in the completion handler, which co_spawn_entry_point
        // keeps alive until its very last dispatch call. This ensures the
        // cancellation_signal (and the proxy signal emplace'd into its slot by
        // co_spawn) outlives every cancellation_state access inside the entry point.
        // The factory is passed directly so co_spawn moves it into
        // co_spawn_entry_point's own frame, fixing the lambda-lifetime problem.
        auto sig = handle.signal_;
        boost::asio::co_spawn(
            boost::asio::make_strand(ioContext()),
            std::forward<Factory>(factory),
            boost::asio::bind_cancellation_slot(
                handle.slot(),
                [sig](std::exception_ptr) {}));
        return handle;
    }

    // Drive the event loop. Blocks until stop() is called.
    void run();

    // Signal the loop to stop. Safe to call from any thread.
    void stop();

private:
    boost::asio::io_context& ioContext();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
