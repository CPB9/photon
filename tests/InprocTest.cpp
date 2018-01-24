#include "UiTest.h"

#include <decode/core/Rc.h>
#include <photon/groundcontrol/Exchange.h>
#include <photon/groundcontrol/FwtState.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include "photongen/onboard//core/Core.Component.h"
#include "photongen/onboard//exc/Exc.Component.h"
#include "photongen/onboard//exc/Device.h"

#include <bmcl/Assert.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <tclap/CmdLine.h>

#include <caf/send.hpp>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

class PhotonStream : public caf::event_based_actor {
public:
    PhotonStream(caf::actor_config& cfg, uint64_t mccId, uint64_t uavId, uint64_t timeout)
        : caf::event_based_actor(cfg)
        , _timeout(timeout)
    {
        Photon_Init();
        PhotonExc_SetAddress(uavId);
        auto err = PhotonExc_RegisterGroundControl(mccId, &_dev);
        BMCL_ASSERT(err == PhotonExcClientError_Ok);
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SendDataAtom, const bmcl::SharedBytes& data) {
                PhotonExcDevice_AcceptInput(_dev, data.data(), data.size());
                Photon_Tick();
            },
            [this](SetStreamDestAtom, const caf::actor& actor) {
                _dest = actor;
            },
            [this](StartAtom) {
                send(this, RepeatStreamAtom::value);
            },
            [this](RepeatStreamAtom) {
                Photon_Tick();

                uint8_t temp[1024];
                PhotonWriter writer;
                PhotonWriter_Init(&writer, temp, 1024);
                PhotonError err = PhotonExcDevice_GenNextPacket(_dev, &writer);

                auto data = bmcl::SharedBytes::create(writer.start, writer.current - writer.start);
                send(_dest, RecvDataAtom::value, data);
                delayed_send(this, std::chrono::milliseconds(_timeout), RepeatStreamAtom::value);
            },
        };
    }

    void on_exit() override
    {
        destroy(_dest);
    }

    PhotonExcDevice* _dev;
    caf::actor _dest;
    uint64_t _timeout;
};

using namespace decode;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("SerialTest");
    TCLAP::ValueArg<uint64_t> mccArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> uavArg("u", "uav-id", "Uav id", false, 2, "number");
    TCLAP::ValueArg<uint64_t> timeoutArg("t", "timeout", "Exchange timeout", false, 10, "milliseconds");

    cmdLine.add(&mccArg);
    cmdLine.add(&uavArg);
    cmdLine.add(&timeoutArg);
    cmdLine.parse(argc, argv);

    uint64_t uavId = uavArg.getValue();
    uint64_t mccId = mccArg.getValue();
    return runUiTest<PhotonStream>(argc, argv, mccId, uavId, mccId, uavId, timeoutArg.getValue());
}

