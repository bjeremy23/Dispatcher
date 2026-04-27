#pragma once

#include "CoroutineHandle.hpp"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <cstddef>
#include <memory>

namespace dispatcher {

class Dispatcher {
public:
    // Registers this dispatcher's executor in ExecutorContext so that
    // sockets can be constructed without an explicit Dispatcher reference.
    // threadCount = 1 : single-threaded; call run() to drive the loop.
    // threadCount > 1 : internal thread pool; run() participates as one thread.
    static Dispatcher& instance();

    explicit Dispatcher(std::size_t threadCount = 1);
    ~Dispatcher();

    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    using Strand = boost::asio::any_io_executor;

    // Returns an awaitable for the calling coroutine's strand.
    // co_await this inside a coroutine to share the strand with a child.
    auto currentStrand() {
        return boost::asio::this_coro::executor;
    }

    // Yields to the scheduler, allowing other queued work on the strand
    // (timers, signals) to run before this coroutine resumes.
    auto yield() {
        return boost::asio::post(boost::asio::use_awaitable);
    }

    // Creates a fresh strand for use outside a coroutine context.
    Strand newStrand() { return boost::asio::make_strand(ioContext()); }

    // Launch a coroutine on a new strand.
    template<typename Factory>
    CoroutineHandle spawn(Factory&& factory) {
        return spawnOn(boost::asio::make_strand(ioContext()),
                       std::forward<Factory>(factory));
    }

    // Launch a coroutine on an existing strand (e.g. inherited from a parent).
    template<typename Factory>
    CoroutineHandle spawn(Strand strand, Factory&& factory) {
        return spawnOn(strand, std::forward<Factory>(factory));
    }

    // Drive the event loop. Blocks until stop() is called.
    void run();

    // Signal the loop to stop. Safe to call from any thread.
    void stop();

private:
    template<typename Executor, typename Factory>
    CoroutineHandle spawnOn(Executor&& executor, Factory&& factory) {
        CoroutineHandle handle;
        auto sig     = handle.signal_;
        auto running = handle.running_;
        *running = true;
        boost::asio::co_spawn(
            std::forward<Executor>(executor),
            std::forward<Factory>(factory),
            boost::asio::bind_cancellation_slot(
                handle.slot(),
                [sig, running](std::exception_ptr) { *running = false; }));
        return handle;
    }

    boost::asio::io_context& ioContext();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
