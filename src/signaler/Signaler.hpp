#pragma once

#include <functional>
#include <initializer_list>
#include <memory>

namespace dispatcher {

class Dispatcher;

// Listens for POSIX signals and invokes a per-signal callback when one fires.
// Call add() to register a signal and its handler; omit the handler to use the
// default (stop the dispatcher). Call remove() to deregister.
class Signaler {
public:
    using Callback = std::function<void()>;

    explicit Signaler(Dispatcher& dispatcher);
    ~Signaler();

    Signaler(const Signaler&) = delete;
    Signaler& operator=(const Signaler&) = delete;
    Signaler(Signaler&&) = delete;
    Signaler& operator=(Signaler&&) = delete;

    // Default callback stops the dispatcher.
    void add(std::initializer_list<int> signals, Callback callback = {});
    void remove(std::initializer_list<int> signals);

private:
    void respawn();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
