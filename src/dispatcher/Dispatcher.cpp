#include "Dispatcher.hpp"
#include "ExecutorContext.hpp"

#include <boost/asio/executor_work_guard.hpp>

#include <thread>
#include <vector>

namespace dispatcher {

struct Dispatcher::Impl {
    explicit Impl(std::size_t threadCount)
        : workGuard(boost::asio::make_work_guard(ctx))
        , threadCount(threadCount)
    {}

    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;
    std::vector<std::thread> threads;
    const std::size_t threadCount;
};

Dispatcher::Dispatcher(std::size_t threadCount)
    : impl_(std::make_unique<Impl>(threadCount))
{
    ExecutorContext::set(impl_->ctx.get_executor());

    // Spawn threadCount-1 background threads; run() provides the final thread.
    for (std::size_t i = 1; i < threadCount; ++i) {
        impl_->threads.emplace_back([this] { impl_->ctx.run(); });
    }
}

Dispatcher::~Dispatcher() {
    stop();
    for (auto& t : impl_->threads) {
        if (t.joinable()) t.join();
    }
}

void Dispatcher::run() {
    impl_->ctx.run();
}

void Dispatcher::stop() {
    impl_->workGuard.reset();
    impl_->ctx.stop();
}

boost::asio::io_context& Dispatcher::ioContext() {
    return impl_->ctx;
}

} // namespace dispatcher
