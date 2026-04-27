#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>

namespace dispatcher {

class Dispatcher;

// Listens for POSIX signals and invokes registered callbacks when one fires.
// Multiple objects may subscribe to the same signal; each gets its own callback.
// Call add() to register interest and receive a SubscriptionId; pass that id
// to remove() to unsubscribe without affecting other subscribers.
// Omit the callback in add() to use the default (stop the dispatcher).
class Signaler {
public:
    using Callback       = std::function<void()>;
    using SubscriptionId = uint64_t;

    explicit Signaler(Dispatcher& dispatcher);
    ~Signaler();

    Signaler(const Signaler&) = delete;
    Signaler& operator=(const Signaler&) = delete;
    Signaler(Signaler&&) = delete;
    Signaler& operator=(Signaler&&) = delete;

    // Default callback stops the dispatcher.
    SubscriptionId add(std::initializer_list<int> signals, Callback callback = {});
    void remove(SubscriptionId id);

private:
    void respawn();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
