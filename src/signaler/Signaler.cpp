#include "Signaler.hpp"

#include "../dispatcher/CoroutineHandle.hpp"
#include "../dispatcher/Dispatcher.hpp"
#include "../dispatcher/ExecutorContext.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <map>
#include <memory>

namespace dispatcher {

struct Signaler::Impl {
    explicit Impl(Dispatcher& dispatcher)
        : dispatcher(dispatcher)
        , signals(ExecutorContext::get())
    {}

    Dispatcher&                         dispatcher;
    boost::asio::signal_set             signals;
    CoroutineHandle                     handle;
    std::map<int, Callback>             handlers;
    std::shared_ptr<std::atomic<bool>>  active{std::make_shared<std::atomic<bool>>(false)};
};

Signaler::Signaler(Dispatcher& dispatcher)
    : impl_(std::make_unique<Impl>(dispatcher))
{}

Signaler::~Signaler() {
    *impl_->active = false;
    impl_->signals.cancel();
}

void Signaler::add(std::initializer_list<int> signals, Callback callback) {
    Callback cb = callback
        ? std::move(callback)
        : Callback([&d = impl_->dispatcher] { d.stop(); });
    for (int signo : signals) {
        impl_->signals.add(signo);
        impl_->handlers[signo] = cb;
    }
    if (!impl_->handle.running())
        respawn();
}

void Signaler::remove(std::initializer_list<int> signals) {
    for (int signo : signals) {
        impl_->signals.remove(signo);
        impl_->handlers.erase(signo);
    }
    if (!impl_->handlers.empty())
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
            auto [ec, signo] = co_await impl_->signals.async_wait(
                boost::asio::as_tuple(boost::asio::use_awaitable));
            if (!*active || ec == boost::asio::error::operation_aborted) co_return;
            if (!ec) {
                auto it = impl_->handlers.find(signo);
                if (it != impl_->handlers.end())
                    it->second();
            }
        }
    });
}

} // namespace dispatcher
