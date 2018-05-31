#pragma once

#include "photon/Config.hpp"

#include <asio/serial_port.hpp>

#include <caf/blocking_actor.hpp>

namespace photon {

//TODO: refact into BlockingStreamBase

class SerialStream : public caf::blocking_actor {
public:
    SerialStream(caf::actor_config& cfg, asio::serial_port&& serial);
    ~SerialStream();

    void recieveFromSerial();

    void act() override;
    void on_exit() override;

private:
    bool _hasStarted;
    caf::actor _dest;
    asio::serial_port _serial;
    bool _isRunning;
};
}
