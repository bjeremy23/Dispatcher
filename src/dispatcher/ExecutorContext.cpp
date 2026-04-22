#include "ExecutorContext.hpp"

#include <stdexcept>

namespace dispatcher {

boost::asio::any_io_executor ExecutorContext::executor_;

void ExecutorContext::set(boost::asio::any_io_executor exec) {
    executor_ = std::move(exec);
}

boost::asio::any_io_executor ExecutorContext::get() {
    if (!executor_) {
        throw std::runtime_error(
            "ExecutorContext: no Dispatcher has been constructed yet");
    }
    return executor_;
}

} // namespace dispatcher
