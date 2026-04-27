#pragma once

#include "component/Component.hpp"
#include "socket/NetworkEndpoint.hpp"

#include <memory>

namespace dispatcher {

class Dispatcher;

class PingPongComponent : public Component {
public:
    PingPongComponent(Dispatcher&           dispatcher,
                      const NetworkEndpoint local,
                      const NetworkEndpoint remote);
    ~PingPongComponent();

    bool init()    override;
    void cleanup() override;

private:
    Dispatcher&     dispatcher_;
    NetworkEndpoint local_;
    NetworkEndpoint remote_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace dispatcher
