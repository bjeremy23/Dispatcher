#include "CoroutineHandle.hpp"

#include <boost/asio/cancellation_type.hpp>

namespace dispatcher {

CoroutineHandle::CoroutineHandle()
    : signal_(std::make_shared<boost::asio::cancellation_signal>())
{}

void CoroutineHandle::cancel() {
    signal_->emit(boost::asio::cancellation_type::terminal);
}

boost::asio::cancellation_slot CoroutineHandle::slot() {
    return signal_->slot();
}

} // namespace dispatcher
