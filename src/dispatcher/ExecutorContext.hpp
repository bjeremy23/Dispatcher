#pragma once

#include <boost/asio/any_io_executor.hpp>

namespace dispatcher {

// Holds the executor for the active Dispatcher.
// Dispatcher::Dispatcher() registers here automatically.
// Socket types read from here so they need no Dispatcher dependency.
class ExecutorContext {
public:
    static void set(boost::asio::any_io_executor exec);
    static boost::asio::any_io_executor get();

private:
    static boost::asio::any_io_executor executor_;
};

} // namespace dispatcher
