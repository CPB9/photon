#include "UiTest.h"

#include <decode/core/Rc.h>
#include <decode/groundcontrol/Exchange.h>
#include <decode/groundcontrol/FwtState.h>
#include <decode/groundcontrol/Atoms.h>
#include <decode/groundcontrol/AllowUnsafeMessageType.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <asio/ip/udp.hpp>

#include <tclap/CmdLine.h>

#include <caf/send.hpp>
#include <caf/others.hpp>
#include <caf/blocking_actor.hpp>

using namespace decode;
using asio::ip::udp;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

class UdpStream : public caf::blocking_actor {
public:
    UdpStream(caf::actor_config& cfg, udp::socket&& socket, udp::endpoint&& endpoint)
        : caf::blocking_actor(cfg)
        , _socket(std::move(socket))
        , _endpoint(std::move(endpoint))
    {
    }

    void act() override
    {
        asio::error_code err;
        std::size_t size = _socket.send_to(asio::buffer("", 0), _endpoint, 0, err);
        do {
            if (has_next_message()) {
                receive(
                    [this](SendDataAtom, const bmcl::SharedBytes& data) {
                        asio::error_code err;
                        std::size_t size = _socket.send_to(asio::buffer(data.data(), data.size()), _endpoint, 0, err);
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
                    caf::others >> [](caf::message_view& x) -> caf::result<caf::message> {
                        return caf::sec::unexpected_message;
                    }
                );
            }

            if (!_hasStarted) {
                continue;
            }

            recieveFromSocket();
        } while (true);
    }

    void recieveFromSocket()
    {
        std::array<uint8_t, 2048> buf;
        asio::error_code err;
        udp::endpoint endpoint;
        std::size_t size = _socket.receive_from(asio::buffer(buf), endpoint, 0, err);
        if (err) {
            BMCL_CRITICAL() << "error recieving packet: " << err.message();
            return;
        }
        if (endpoint != _endpoint) {
            BMCL_WARNING() << "recieved packet from different endpoint";
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
        destroy(_dest);
    }

    bool _hasStarted;
    caf::actor _dest;
    udp::socket _socket;
    udp::endpoint _endpoint;
};

using namespace decode;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("UdpTest");
    TCLAP::ValueArg<std::string> addrArg("a", "address", "Ip address", false, "127.0.0.1", "ip address");
    TCLAP::ValueArg<unsigned short> portArg("p", "port", "Port", false, 6666, "port");

    cmdLine.add(&addrArg);
    cmdLine.add(&portArg);
    cmdLine.parse(argc, argv);

    asio::error_code err;
    asio::ip::address address = asio::ip::make_address(addrArg.getValue(), err);
    if (err) {
        BMCL_CRITICAL() << "invalid ip address: " << err.message();
        return err.value();
    }

    asio::io_service ioService;
    udp::endpoint endpoint(address, portArg.getValue());
    udp::socket socket(ioService);
    socket.open(udp::v4(), err);
    if (err) {
        BMCL_CRITICAL() << "error opening socket: " << err.message();
        return err.value();
    }

    return runUiTest<UdpStream>(argc, argv, std::move(socket), std::move(endpoint));
}
