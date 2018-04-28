#include "UiTest.h"

#include <decode/core/Rc.h>
#include <photon/groundcontrol/Exchange.h>
#include <photon/groundcontrol/FwtState.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <asio/serial_port.hpp>

#include <tclap/CmdLine.h>

#include <caf/send.hpp>
#include <caf/others.hpp>
#include <caf/blocking_actor.hpp>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

//TODO: refact into BlockingStreamBase

class SerialStream : public caf::blocking_actor {
public:
    SerialStream(caf::actor_config& cfg, asio::serial_port&& serial)
        : caf::blocking_actor(cfg)
        , _serial(std::move(serial))
        , _isRunning(true)
    {
    }

    void act() override
    {
        do {
            if (has_next_message()) {
                receive(
                    [this](SendDataAtom, const bmcl::SharedBytes& data) {
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

    void recieveFromSerial()
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
    }

    void on_exit() override
    {
        send_exit(_dest, caf::exit_reason::user_shutdown);
        destroy(_dest);
    }

    bool _hasStarted;
    caf::actor _dest;
    asio::serial_port _serial;
    bool _isRunning;
};

using namespace decode;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("SerialTest");
    TCLAP::ValueArg<std::string> serialArg("s", "serial", "Read from serial", true, "", "serial device");
    TCLAP::ValueArg<unsigned> baudArg("b", "baud", "Serial baud rate", false, 9600, "baud rate");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");

    cmdLine.add(&serialArg);
    cmdLine.add(&baudArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.parse(argc, argv);

    asio::io_service ioService;
    asio::serial_port serial(ioService);

    asio::error_code err;
    serial.open(serialArg.getValue(), err);
    if (err) {
        BMCL_CRITICAL() << "could not open serial port: " << err.message();
        return err.value();
    }

    serial.set_option(asio::serial_port::baud_rate(baudArg.getValue()), err);
    if (err) {
        BMCL_CRITICAL() << "could not set baud rate " << baudArg.getValue() << ": " << err.message();
        return err.value();
    }


    return runUiTest<SerialStream>(argc, argv, srcArg.getValue(), destArg.getValue(), std::move(serial));
}
