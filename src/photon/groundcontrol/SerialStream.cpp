#include <photon/groundcontrol/SerialStream.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/groundcontrol/Atoms.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <asio/serial_port.hpp>

#include <caf/send.hpp>
#include <caf/others.hpp>
#include <caf/blocking_actor.hpp>

#include <chrono>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

namespace photon {

//TODO: refact into BlockingStreamBase

SerialStream::SerialStream(caf::actor_config& cfg, asio::serial_port&& serial)
    : caf::blocking_actor(cfg)
    , _serial(std::move(serial))
    , _isRunning(true)
{
}

SerialStream::~SerialStream()
{
}

void SerialStream::act()
{
    do {
        if (has_next_message()) {
            receive(
                [this](RecvDataAtom, const bmcl::SharedBytes& data) {
                    asio::error_code err;
                    std::size_t size = _serial.write_some(asio::buffer(data.data(), data.size()), err);
                    if (err) {
                        BMCL_CRITICAL() << "error sending packet: " << err.message();
                    }
                    assert(size == data.size());
                },
                [this](SetStreamDestAtom, const caf::actor& actor) {
                    _dest = actor;
                },
                [this](StartAtom) {
                    _hasStarted = true;
                },
                [&](const caf::down_msg& x) {
                    _isRunning = false;
                },
                [&](const caf::exit_msg& x) {
                    _isRunning = false;
                },
                caf::others >> [](caf::message_view& x) -> caf::result<caf::message> {
                    return caf::sec::unexpected_message;
                }
            );
        }

        if (!_hasStarted) {
            continue;
        }

        recieveFromSerial();
    } while (_isRunning);
}

void SerialStream::recieveFromSerial()
{
    std::array<uint8_t, 2048> buf;
    asio::error_code err;
    std::size_t size = _serial.read_some(asio::buffer(buf), err);
    if (err) {
        BMCL_CRITICAL() << "error recieving packet: " << err.message();
        return;
    }
    if (size == 0) {
        return;
    }

    bmcl::SharedBytes data = bmcl::SharedBytes::create(buf.data(), size);
    send(_dest, RecvDataAtom::value, data);
    if (size != 2048) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SerialStream::on_exit()
{
    send_exit(_dest, caf::exit_reason::user_shutdown);
    destroy(_dest);
}
}

