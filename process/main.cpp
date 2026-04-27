#include "PingPongComponent.hpp"

#include "component/ComponentMgr.hpp"
#include "dispatcher/Dispatcher.hpp"
#include "signaler/Signaler.hpp"
#include "socket/IpAddress.hpp"
#include "socket/NetworkEndpoint.hpp"

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: pingpong <local_port> <remote_port> [local_ip=127.0.0.1] [remote_ip=127.0.0.1]\n";
        return 1;
    }

    const uint16_t    localPort  = static_cast<uint16_t>(std::atoi(argv[1]));
    const uint16_t    remotePort = static_cast<uint16_t>(std::atoi(argv[2]));
    const std::string localIp    = (argc > 3) ? argv[3] : "127.0.0.1";
    const std::string remoteIp   = (argc > 4) ? argv[4] : "127.0.0.1";

    const dispatcher::NetworkEndpoint local{
        dispatcher::IpAddress(localIp,  AF_INET), localPort
    };
    const dispatcher::NetworkEndpoint remote{
        dispatcher::IpAddress(remoteIp, AF_INET), remotePort
    };

    dispatcher::Dispatcher dispatcher;

    dispatcher::Signaler signaler(dispatcher);
    signaler.add({SIGINT, SIGTERM});  // default callback: stop the dispatcher

    dispatcher::ComponentMgr mgr;
    mgr.add(std::make_unique<dispatcher::PingPongComponent>(dispatcher, local, remote));

    if (!mgr.init()) {
        std::cerr << "init failed\n";
        return 1;
    }

    std::cout << "listening on " << localIp << ":" << localPort
              << ", pinging " << remoteIp << ":" << remotePort
              << " every 5s  (Ctrl-C to stop)\n";

    dispatcher.run();
    return 0;
}
