#pragma once

#include "photon/Config.hpp"

#include <caf/blocking_actor.hpp>

#include <asio/ip/udp.hpp>

namespace photon {

class UdpStream : public caf::blocking_actor {
public:
    using udp = asio::ip::udp;

    UdpStream(caf::actor_config& cfg, udp::socket&& socket, udp::endpoint&& endpoint);
    ~UdpStream();

    void recieveFromSocket();

    void act() override;
    void on_exit() override;

private:
    bool _hasStarted;
    caf::actor _dest;
    udp::socket _socket;
    udp::endpoint _endpoint;
    bool _isRunning;
};
}
