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
#include <vector>

namespace dispatcher {

struct Signaler::Impl {
    explicit Impl(Dispatcher& dispatcher)
        : dispatcher(dispatcher)
        , signals(ExecutorContext::get())
    {}

    Dispatcher&                                              dispatcher;
    boost::asio::signal_set                                  signals;
    CoroutineHandle                                          handle;
    std::map<int, std::map<SubscriptionId, Callback>>        handlers;     // signal → {id → cb}
    std::map<SubscriptionId, std::vector<int>>               id_to_signals; // for removal
    SubscriptionId                                           next_id{1};
    std::shared_ptr<std::atomic<bool>>                       active{std::make_shared<std::atomic<bool>>(false)};
};

Signaler::Signaler(Dispatcher& dispatcher)
    : impl_(std::make_unique<Impl>(dispatcher))
{}

Signaler::~Signaler() {
    *impl_->active = false;
    impl_->signals.cancel();
}

Signaler::SubscriptionId Signaler::add(std::initializer_list<int> signals, Callback callback) {
    Callback cb = callback
        ? std::move(callback)
        : Callback([&d = impl_->dispatcher] { d.stop(); });

    SubscriptionId id = impl_->next_id++;
    for (int signo : signals) {
        impl_->signals.add(signo);
        impl_->handlers[signo][id] = cb;
    }
    impl_->id_to_signals[id] = std::vector<int>(signals.begin(), signals.end());

    if (!impl_->handle.running())
        respawn();
    return id;
}

void Signaler::remove(SubscriptionId id) {
    auto it = impl_->id_to_signals.find(id);
    if (it == impl_->id_to_signals.end())
        return;

    for (int signo : it->second) {
        auto& subs = impl_->handlers[signo];
        subs.erase(id);
        if (subs.empty()) {
            impl_->handlers.erase(signo);
            impl_->signals.remove(signo);
        }
    }
    impl_->id_to_signals.erase(it);

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
                    for (auto& [id, cb] : it->second)
                        cb();
            }
        }
    });
}

} // namespace dispatcher
