#include "UiTest.h"
#include "photon/groundcontrol/UdpStream.h"

#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

using namespace decode;
using asio::ip::udp;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("UdpTest");
    TCLAP::ValueArg<std::string> addrArg("a", "address", "Ip address", false, "127.0.0.1", "ip address");
    TCLAP::ValueArg<unsigned short> portArg("p", "port", "Port", false, 6666, "port");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");

    cmdLine.add(&addrArg);
    cmdLine.add(&portArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.parse(argc, argv);

    asio::error_code err;
    asio::ip::address address = asio::ip::make_address(addrArg.getValue(), err);
    if (err) {
        BMCL_CRITICAL() << "invalid ip address: " << err.message();
        return err.value();
    }

    asio::io_context ioService;
    udp::endpoint endpoint(address, portArg.getValue());
    udp::socket socket(ioService);
    socket.open(udp::v4(), err);
    if (err) {
        BMCL_CRITICAL() << "error opening socket: " << err.message();
        return err.value();
    }

    return runUiTest<UdpStream>(argc, argv, srcArg.getValue(), destArg.getValue(), std::move(socket), std::move(endpoint));
}
