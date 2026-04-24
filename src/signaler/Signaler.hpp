#pragma once

#include <functional>
#include <memory>

namespace dispatcher {

class Dispatcher;

// Listens for POSIX signals and invokes a callback when one fires.
// By default the callback stops the Dispatcher. Call add()/remove() to
// manage the set of watched signals; the internal coroutine is reset
// automatically whenever the set changes.
class Signaler {
public:
    using Callback = std::function<void()>;

    // onSignal defaults to stopping the given dispatcher.
    explicit Signaler(Dispatcher& dispatcher, Callback onSignal = {});
    ~Signaler();

    Signaler(const Signaler&) = delete;
    Signaler& operator=(const Signaler&) = delete;
    Signaler(Signaler&&) = delete;
    Signaler& operator=(Signaler&&) = delete;

    void add(int signo);
    void remove(int signo);

private:
    void respawn();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
