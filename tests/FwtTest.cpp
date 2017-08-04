#include "decode/groundcontrol/GroundControl.h"
#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/core/Core.Component.h"

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <caf/all.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <random>

using namespace decode;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

class FwtTest;

struct TestConfig {
    FwtTest* parent;
    bool exitOnProjectUpdate = false;
    bool projectUpdated = false;
    bool stopOnError = true;
    std::size_t errorCount = 0;
};

static std::default_random_engine _deliverEngine;
static std::default_random_engine _recieveEngine;

struct StreamConfig {
    StreamConfig()
    {
        setDeliveryProbability(1);
        setRecieveProbability(1);
    }

    void setDeliveryProbability(double p)
    {
        packetDeliverSuccessDist = std::bernoulli_distribution(p);
    }

    void setRecieveProbability(double p)
    {
        packetRecieveSuccessDist = std::bernoulli_distribution(p);
    }

    bool shouldDeliverPacket()
    {
        return packetDeliverSuccessDist(_deliverEngine);
    }

    bool shouldRecievePacket()
    {
        return packetRecieveSuccessDist(_recieveEngine);
    }

    std::size_t chunkSize = 1;
    std::bernoulli_distribution packetDeliverSuccessDist;
    std::bernoulli_distribution packetRecieveSuccessDist;

};

class FwtTest : public ::testing::Test {
public:
    void run();
    void stop();

protected:
    void SetUp() override;
    void TearDown() override;

    caf::actor_system_config _cfg;
    std::unique_ptr<caf::actor_system> _system;
    caf::actor _gc;
    caf::actor _handler;
    caf::actor _stream;
    TestConfig _testCfg;
    StreamConfig _streamCfg;
};

class FakeEventHandler : public caf::event_based_actor {
public:
    FakeEventHandler(caf::actor_config& cfg, TestConfig* testCfg)
        : caf::event_based_actor(cfg)
        , _testCfg(testCfg)
    {
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SetProjectAtom, const Rc<const Project>& proj, const Rc<const Device>& dev) {
                _testCfg->projectUpdated = true;
                if (_testCfg->exitOnProjectUpdate) {
                    _testCfg->parent->stop();
                }
            },
            [this](FirmwareErrorEventAtom, const std::string& msg) {
                BMCL_CRITICAL() << "error: " << msg;
                _testCfg->errorCount++;
                if (_testCfg->stopOnError) {
                    _testCfg->parent->stop();
                }
            },
            [this](FirmwareDownloadStartedEventAtom) {
            },
            [this](FirmwareDownloadFinishedEventAtom) {
            },
            [this](FirmwareStartCmdSentEventAtom) {
            },
            [this](FirmwareStartCmdPassedEventAtom) {
            },
            [this](FirmwareProgressEventAtom, std::size_t size) {
            },
            [this](FirmwareSizeRecievedEventAtom, std::size_t size) {
            },
            [this](FirmwareHashDownloadedEventAtom, const std::string& name, const bmcl::SharedBytes& hash) {
            },
        };
    }

    const char* name() const override
    {
        return "FakeEventHandler";
    }

    TestConfig* _testCfg;
};

class PhotonStream : public caf::event_based_actor {
public:
    PhotonStream(caf::actor_config& cfg, StreamConfig* streamCfg)
        : caf::event_based_actor(cfg)
        , _streamCfg(streamCfg)
    {
        Photon_Init();
        _current = *PhotonExc_GetMsg();
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SendDataAtom, const bmcl::SharedBytes& data) {
                if (_streamCfg->shouldRecievePacket()) {
                    for (const uint8_t* it = data.data(); it < data.view().end(); it += _streamCfg->chunkSize) {
                        std::size_t size = std::min<std::size_t>(_streamCfg->chunkSize, data.view().end() - it);
                        PhotonExc_AcceptInput(it, size);
                    }
                }
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
                if (_streamCfg->shouldDeliverPacket()) {
                    auto data = bmcl::SharedBytes::create(_current.data, _current.size);
                    send(_dest, RecvDataAtom::value, data);
                }

                PhotonExc_PrepareNextMsg();
                _current = *PhotonExc_GetMsg();
                delayed_send(this, std::chrono::milliseconds(10), RepeatStreamAtom::value);
            },
        };
    }

    void on_exit() override
    {
        _dest = caf::actor();
    }

    PhotonExcMsg _current;
    caf::actor _dest;
    StreamConfig* _streamCfg;
};

void FwtTest::run()
{
    _stream = _system->spawn<PhotonStream>(&_streamCfg);
    _handler = _system->spawn<FakeEventHandler>(&_testCfg);
    _gc = _system->spawn<decode::GroundControl>(_stream, _handler);

    caf::anon_send(_stream, SetStreamDestAtom::value, _gc);
    caf::anon_send(_gc, StartAtom::value);
    caf::anon_send(_stream, StartAtom::value);
    _system->await_all_actors_done();
}

void FwtTest::stop()
{
    caf::anon_send_exit(_stream, caf::exit_reason::user_shutdown);
    caf::anon_send_exit(_gc, caf::exit_reason::user_shutdown);
    caf::anon_send_exit(_handler, caf::exit_reason::user_shutdown);
}

void FwtTest::SetUp()
{
    _testCfg = TestConfig();
    _streamCfg = StreamConfig();
    _testCfg.parent = this;
    _system.reset(new caf::actor_system(_cfg));
}

void FwtTest::TearDown()
{
    _system.reset();
}

TEST_F(FwtTest, noLostPacketsByteByByte)
{
    _testCfg.exitOnProjectUpdate = true;
    run();
    EXPECT_EQ(0, _testCfg.errorCount);
    EXPECT_TRUE(_testCfg.projectUpdated);
}

TEST_F(FwtTest, noLostPacketsFourBytes)
{
    _streamCfg.chunkSize = 4;
    _testCfg.exitOnProjectUpdate = true;
    run();
    EXPECT_EQ(0, _testCfg.errorCount);
    EXPECT_TRUE(_testCfg.projectUpdated);
}

TEST_F(FwtTest, noLostPacketsNineBytes)
{
    _streamCfg.chunkSize = 9;
    _testCfg.exitOnProjectUpdate = true;
    run();
    EXPECT_EQ(0, _testCfg.errorCount);
    EXPECT_TRUE(_testCfg.projectUpdated);
}

TEST_F(FwtTest, looseFiftyPercentPackets)
{
    _streamCfg.chunkSize = 9;
    _streamCfg.setDeliveryProbability(0.5);
    _streamCfg.setRecieveProbability(0.5);
    _testCfg.exitOnProjectUpdate = true;
    run();
    EXPECT_TRUE(_testCfg.projectUpdated);
}
