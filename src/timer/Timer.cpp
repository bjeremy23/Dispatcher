#include "Timer.hpp"

#include "../dispatcher/CoroutineHandle.hpp"
#include "../dispatcher/Dispatcher.hpp"
#include "../dispatcher/ExecutorContext.hpp"

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <atomic>
#include <memory>

namespace dispatcher {

struct Timer::Impl {
    explicit Impl(Dispatcher& dispatcher)
        : dispatcher(dispatcher)
        , timer(ExecutorContext::get())
    {}

    Dispatcher&                        dispatcher;
    CoroutineHandle                    handle;
    boost::asio::steady_timer          timer;
    std::shared_ptr<std::atomic<bool>> active{std::make_shared<std::atomic<bool>>(false)};
};

Timer::Timer(Dispatcher& dispatcher)
    : impl_(std::make_unique<Impl>(dispatcher))
{}

Timer::~Timer() {
    stop();
}

Timer::Timer(Timer&&) noexcept = default;
Timer& Timer::operator=(Timer&&) noexcept = default;

void Timer::start(std::chrono::steady_clock::duration interval,
                  Factory factory,
                  Mode mode)
{
    *impl_->active = false;
    impl_->active = std::make_shared<std::atomic<bool>>(true);
    impl_->timer.cancel();

    auto active = impl_->active;
    impl_->handle = impl_->dispatcher.spawn(
        [this, active, interval, factory = std::move(factory), mode]() -> boost::asio::awaitable<void> {
            if (!*active) co_return;
            while (true) {
                impl_->timer.expires_after(interval);
                boost::system::error_code ec;
                co_await impl_->timer.async_wait(
                    boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (!*active) co_return;
                if (ec) { *active = false; co_return; }
                co_await factory();
                if (!*active || mode == Mode::OneShot) { *active = false; co_return; }
            }
        });
}

void Timer::stop() {
    *impl_->active = false;
    impl_->timer.cancel();
}

bool Timer::running() const {
    return *impl_->active;
}

} // namespace dispatcher
