#include "PingPongComponent.hpp"

#include "dispatcher/Dispatcher.hpp"
#include "dispatcher/CoroutineHandle.hpp"
#include "msgbuffer/MsgBuffer.hpp"
#include "socket/UdpSocket.hpp"
#include "timer/Timer.hpp"

#include <boost/asio/awaitable.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>

using namespace std::chrono_literals;

namespace dispatcher {

struct PingPongComponent::Impl {
    std::unique_ptr<UdpSocket> socket;
    std::unique_ptr<Timer>     timer;
    CoroutineHandle            handle;
};

PingPongComponent::PingPongComponent(Dispatcher&           dispatcher,
                                     const NetworkEndpoint local,
                                     const NetworkEndpoint remote)
    : dispatcher_(dispatcher)
    , local_(local)
    , remote_(remote)
    , impl_(std::make_unique<Impl>())
{}

PingPongComponent::~PingPongComponent() = default;

bool PingPongComponent::init() {
    impl_->socket = std::make_unique<UdpSocket>(local_);
    if (auto ec = impl_->socket->bind(); ec) {
        std::cerr << "bind failed: " << ec.message() << "\n";
        return false;
    }

    // Both the receive loop and timer share a strand so they never interleave.
    auto strand = dispatcher_.newStrand();

    impl_->handle = dispatcher_.spawn(strand,
        [this]() -> boost::asio::awaitable<void> {
            MsgBuffer buf;
            while (true) {
                co_await dispatcher_.yield();
                NetworkEndpoint sender;
                auto ec = co_await impl_->socket->receiveFrom(buf, sender);
                if (ec) co_return;

                std::string_view msg(buf.data(), buf.size());
                std::cout << "[recv] " << msg
                          << " from " << sender.address.toString()
                          << ":" << sender.port << "\n";

                if (msg == "ping") {
                    std::string_view reply = "pong";
                    co_await impl_->socket->sendTo({reply.data(), reply.size()}, sender);
                    std::cout << "[sent] pong → "
                              << sender.address.toString() << ":" << sender.port << "\n";
                }
            }
        });

    impl_->timer = std::make_unique<Timer>(dispatcher_);
    impl_->timer->start(strand, 5s,
        [this]() -> boost::asio::awaitable<void> {
            std::string_view msg = "ping";
            co_await impl_->socket->sendTo({msg.data(), msg.size()}, remote_);
            std::cout << "[sent] ping → "
                      << remote_.address.toString() << ":" << remote_.port << "\n";
        },
        Timer::Mode::Repeating);

    return true;
}

void PingPongComponent::cleanup() {
    if (impl_->timer)  impl_->timer->stop();
    impl_->handle.cancel();
}

} // namespace dispatcher
