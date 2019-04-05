#include "UiTest.h"

#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/groundcontrol/StreamFromString.h>

#include <bmcl/String.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <asio/io_context.hpp>

#include <cstdint>

using namespace photon;

using namespace decode;

const char* usage =
"Examples:\n"
"photon-client --device udp:192.168.0.111:6666\n"
"photon-client --device udp:93.175.77.123:8888\n"
"photon-client --device serial:/dev/ttyUSB0:9600\n"
"photon-client --device serial:COM5:115200:8N1\n"
;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine(usage);
    TCLAP::ValueArg<std::string> deviceArg("d", "device", "Device", true, "udp,127.0.0.1,6666", "device string");
    TCLAP::ValueArg<uint64_t> srcArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> destArg("u", "uav-id", "Uav id", false, 2, "number");

    cmdLine.add(&deviceArg);
    cmdLine.add(&srcArg);
    cmdLine.add(&destArg);
    cmdLine.parse(argc, argv);

    asio::io_context ioService;
    caf::actor_system_config cfg;
    caf::actor_system system(cfg);

    auto rv = spawnActorFromDeviceString(system, ioService, deviceArg.getValue());
    if (rv.isErr()) {
        BMCL_CRITICAL() << rv.unwrapErr();
        return -1;
    }

    caf::actor uiActor = system.spawn<UiActor, caf::detached>(srcArg.getValue(), destArg.getValue(), rv.unwrap(), argc, argv);
    caf::anon_send(uiActor, photon::StartAtom::value);
    system.await_all_actors_done();
    return 0;
}

