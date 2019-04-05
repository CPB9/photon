#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/groundcontrol/StreamFromString.h>
#include <photon/groundcontrol/Atoms.h>

#include <bmcl/String.h>
#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <asio/io_context.hpp>

#include <caf/actor_system_config.hpp>
#include <caf/send.hpp>

#include <cstdint>

using namespace photon;

const char* usage =
"photon-proxy --from udp:192.168.0.111:6666 --to serial:/dev/ttyUSB3:38400:8N1\n"
;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine(usage);
    TCLAP::ValueArg<std::string> fromArg("f", "from", "Device", true, "udp,127.0.0.1,6666", "device string");
    TCLAP::ValueArg<std::string> toArg("t", "to", "Device", true, "udp,127.0.0.1,6666", "device string");

    cmdLine.add(&fromArg);
    cmdLine.add(&toArg);
    cmdLine.parse(argc, argv);

    asio::io_context ioService;
    caf::actor_system_config cfg;
    caf::actor_system system(cfg);

    auto rvFrom = spawnActorFromDeviceString(system, ioService, fromArg.getValue());
    if (rvFrom.isErr()) {
        BMCL_CRITICAL() << rvFrom.unwrapErr();
        return -1;
    }
    auto rvTo = spawnActorFromDeviceString(system, ioService, toArg.getValue());
    if (rvTo.isErr()) {
        BMCL_CRITICAL() << rvTo.unwrapErr();
        return -1;
    }

    caf::anon_send(rvFrom.unwrap(), photon::SetStreamDestAtom::value, rvTo.unwrap());
    caf::anon_send(rvTo.unwrap(), photon::SetStreamDestAtom::value, rvFrom.unwrap());

    caf::anon_send(rvFrom.unwrap(), photon::StartAtom::value);
    caf::anon_send(rvTo.unwrap(), photon::StartAtom::value);

    system.await_all_actors_done();
    return 0;
}



