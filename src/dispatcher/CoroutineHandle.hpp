#pragma once

#include <boost/asio/cancellation_signal.hpp>

#include <atomic>
#include <memory>

namespace dispatcher {

class Dispatcher;

// Returned by Dispatcher::spawn(). Call cancel() to terminate the coroutine
// at its next co_await point. The awaited operation will complete with
// boost::asio::error::operation_aborted.
class CoroutineHandle {
public:
    CoroutineHandle();
    ~CoroutineHandle() = default;

    CoroutineHandle(CoroutineHandle&&) noexcept = default;
    CoroutineHandle& operator=(CoroutineHandle&&) noexcept = default;
    CoroutineHandle(const CoroutineHandle&) = delete;
    CoroutineHandle& operator=(const CoroutineHandle&) = delete;

    void cancel();
    bool running() const;

private:
    friend class Dispatcher;
    boost::asio::cancellation_slot slot();

    std::shared_ptr<boost::asio::cancellation_signal> signal_;
    // Set true by spawn(), false by the completion handler when the coroutine exits.
    std::shared_ptr<std::atomic<bool>> running_;
};

} // namespace dispatcher
