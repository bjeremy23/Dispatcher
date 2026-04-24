#include "Signaler.hpp"

#include "../dispatcher/CoroutineHandle.hpp"
#include "../dispatcher/Dispatcher.hpp"
#include "../dispatcher/ExecutorContext.hpp"

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <memory>
#include <set>

namespace dispatcher {

struct Signaler::Impl {
    Impl(Dispatcher& dispatcher, Callback cb)
        : dispatcher(dispatcher)
        , onSignal(std::move(cb))
        , signals(ExecutorContext::get())
    {}

    Dispatcher&                         dispatcher;
    Callback                            onSignal;
    boost::asio::signal_set             signals;
    CoroutineHandle                     handle;
    std::set<int>                       registered;
    std::shared_ptr<std::atomic<bool>>  active{std::make_shared<std::atomic<bool>>(false)};
};

Signaler::Signaler(Dispatcher& dispatcher, Callback onSignal)
    : impl_(std::make_unique<Impl>(
          dispatcher,
          onSignal ? std::move(onSignal) : Callback([&dispatcher] { dispatcher.stop(); })))
{}

Signaler::~Signaler() {
    *impl_->active = false;
    impl_->signals.cancel();
}

void Signaler::add(int signo) {
    impl_->signals.add(signo);
    impl_->registered.insert(signo);
    if (!impl_->handle.running())
        respawn();
}

void Signaler::remove(int signo) {
    impl_->signals.remove(signo);
    impl_->registered.erase(signo);
    if (!impl_->registered.empty())
        respawn();
}

void Signaler::respawn() {
    *impl_->active = false;
    impl_->active = std::make_shared<std::atomic<bool>>(true);
    impl_->signals.cancel();

    auto active = impl_->active;
    impl_->handle = impl_->dispatcher.spawn([this, active]() -> boost::asio::awaitable<void> {
        if (!*active) co_return;
        while (true) {
            boost::system::error_code ec;
            co_await impl_->signals.async_wait(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
            if (!*active || ec == boost::asio::error::operation_aborted) co_return;
            if (!ec) impl_->onSignal();
        }
    });
}

} // namespace dispatcher
