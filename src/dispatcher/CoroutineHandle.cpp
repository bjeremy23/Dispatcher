#include "CoroutineHandle.hpp"

#include <boost/asio/cancellation_type.hpp>

namespace dispatcher {

CoroutineHandle::CoroutineHandle()
    : signal_(std::make_shared<boost::asio::cancellation_signal>())
    , running_(std::make_shared<std::atomic<bool>>(false))
{}

void CoroutineHandle::cancel() {
    signal_->emit(boost::asio::cancellation_type::terminal);
}

bool CoroutineHandle::running() const {
    return running_ && *running_;
}

boost::asio::cancellation_slot CoroutineHandle::slot() {
    return signal_->slot();
}

} // namespace dispatcher
