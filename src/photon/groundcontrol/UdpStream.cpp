#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/groundcontrol/UdpStream.h>
#include <photon/groundcontrol/Atoms.h>

#include <bmcl/SharedBytes.h>
#include <bmcl/Logging.h>

#include <asio/ip/udp.hpp>

#include <caf/blocking_actor.hpp>
#include <caf/send.hpp>
#include <caf/others.hpp>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

namespace photon {

using udp = asio::ip::udp;

UdpStream::UdpStream(caf::actor_config& cfg, udp::socket&& socket, udp::endpoint&& endpoint)
    : caf::blocking_actor(cfg)
    , _socket(std::move(socket))
    , _endpoint(std::move(endpoint))
    , _isRunning(true)

{
    _socket.non_blocking(true);
}

UdpStream::~UdpStream()
{
}

void UdpStream::act()
{
    asio::error_code err;
    std::size_t size = _socket.send_to(asio::buffer("", 0), _endpoint, 0, err);
    (void)size;
    do {
        if (has_next_message()) {
            receive(
                [this](RecvDataAtom, const bmcl::SharedBytes& data) {
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

        recieveFromSocket();
    } while (_isRunning);
}

void UdpStream::recieveFromSocket()
{
    std::array<uint8_t, 2048> buf;
    asio::error_code err;
    udp::endpoint endpoint;
    std::size_t size = _socket.receive_from(asio::buffer(buf), endpoint, 0, err);
    if (err) {
        if (err == asio::error::would_block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
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

void UdpStream::on_exit()
{
    send_exit(_dest, caf::exit_reason::user_shutdown);
    destroy(_dest);
}
}
