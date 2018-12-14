#include "UiTest.h"

#include <decode/core/Rc.h>
#include <photon/groundcontrol/Exchange.h>
#include <photon/groundcontrol/FwtState.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>

#include "Photon.h"

#include <bmcl/Assert.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <tclap/CmdLine.h>

#include <caf/send.hpp>

#include <random>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

class PhotonStream : public caf::event_based_actor {
public:
    using RecvModifiedData = caf::atom_constant<caf::atom("_ecvmdata")>;

    struct ErrorState {
        explicit ErrorState(uint32_t rate)
            : engine(std::random_device{}())
            , dist(1.0 / rate)
        {
        }

        bool shouldToggle()
        {
            return dist(engine);
        }

        std::default_random_engine engine;
        std::bernoulli_distribution dist;
    };

    PhotonStream(caf::actor_config& cfg, uint64_t mccId, uint64_t uavId,
                 std::chrono::milliseconds timeout,
                 std::chrono::milliseconds exchangeDelay,
                 const bmcl::Option<uint32_t>& errorRate)
        : caf::event_based_actor(cfg)
        , _timeout(timeout)
        , _exchangeDelay(exchangeDelay)
    {
        Photon_Init();
        PhotonExc_SetAddress(uavId);
#ifdef PHOTON_HAS_MODULE_ASV
        PhotonAsv_LoadVars();
#endif
        auto err = PhotonExc_RegisterGroundControl(mccId, &_dev);
        BMCL_ASSERT(err == PhotonExcClientError_Ok);

        if (errorRate.isSome()) {
            _errorState.emplace(errorRate.unwrap());
        }
    }

    ~PhotonStream()
    {
#ifdef PHOTON_HAS_MODULE_ASV
        BMCL_DEBUG() << "saving params";
        PhotonError rv = PhotonAsv_SaveVars();
        if (rv != PhotonError_Ok) {
            BMCL_CRITICAL() << "failed to save vars";
        }
#endif
    }

    void breakData(uint8_t* dest, std::size_t size)
    {
        for (std::size_t i = 0; i < size; i++) {
            for (unsigned j = 0; j < 8; j++) {
                if (_errorState->shouldToggle()) {
                    dest[i] ^= (1 << j);
                }
            }
        }
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](RecvDataAtom, const bmcl::SharedBytes& data) {
                if (_errorState.isSome()) {
                    bmcl::SharedBytes temp(data);
                    breakData(temp.data(), temp.size());
                    delayed_send(this, _exchangeDelay, RecvModifiedData::value, temp);
                } else {
                    delayed_send(this, _exchangeDelay, RecvModifiedData::value, data);
                }
            },
            [this](RecvModifiedData, const bmcl::SharedBytes& data) {
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
                PhotonWriter_Init(&writer, temp, sizeof(temp));
                PhotonExcDevice_GenNextPacket(_dev, &writer);

                if (_errorState.isSome()) {
                    breakData(temp, writer.current - writer.start);
                }

                auto data = bmcl::SharedBytes::create(writer.start, writer.current - writer.start);
                delayed_send(_dest, _exchangeDelay, RecvDataAtom::value, data);
                delayed_send(this, _timeout, RepeatStreamAtom::value);
            },
        };
    }

    void on_exit() override
    {
        destroy(_dest);
    }

    PhotonExcDevice* _dev;
    caf::actor _dest;
    std::chrono::milliseconds _timeout;
    std::chrono::milliseconds _exchangeDelay;
    bmcl::Option<ErrorState> _errorState;
};

using namespace decode;

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("SerialTest");
    TCLAP::ValueArg<uint64_t> mccArg("m", "mcc-id", "Mcc id", false, 1, "number");
    TCLAP::ValueArg<uint64_t> uavArg("u", "uav-id", "Uav id", false, 2, "number");
    TCLAP::ValueArg<uint64_t> timeoutArg("t", "timeout", "Tick timeout", false, 10, "milliseconds");
    TCLAP::ValueArg<uint64_t> exchangeDelayArg("d", "delay", "Packet delivery delay", false, 100, "milliseconds");
    TCLAP::ValueArg<uint32_t> errorRateArg("e", "error-rate", "Bit error rate R (1 toggled bit per R recieved)", false, 1000000, "bits");
    TCLAP::ValueArg<unsigned> logArg("l", "log-level", "Log level", false, 5, "level");

    cmdLine.add(&mccArg);
    cmdLine.add(&uavArg);
    cmdLine.add(&timeoutArg);
    cmdLine.add(&exchangeDelayArg);
    cmdLine.add(&errorRateArg);
    cmdLine.add(&logArg);
    cmdLine.parse(argc, argv);

    bmcl::setLogLevel(bmcl::LogLevel(logArg.getValue()));

    uint64_t uavId = uavArg.getValue();
    uint64_t mccId = mccArg.getValue();
    auto exchangeDelay = std::chrono::milliseconds(exchangeDelayArg.getValue());
    auto timeout = std::chrono::milliseconds(timeoutArg.getValue());
    bmcl::Option<uint32_t> errorRate;
    if (errorRateArg.isSet()) {
        errorRate.emplace(errorRateArg.getValue());
    }
    return runUiTest<PhotonStream>(argc, argv, mccId, uavId, mccId, uavId, timeout, exchangeDelay, errorRate);
}

