#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/Scheduler.h"
#include "photon/fwt/Fwt.Component.h"

#include <thread>

using namespace decode;

class TestSink : public Sender {
public:
    void sendPacket(Client* self, bmcl::Bytes packet) override
    {
        packets.emplace_back(packet);
    }

    std::vector<bmcl::Buffer> packets;
};

class TestScheduler : public Scheduler {
public:
    void scheduleAction(StateAction action, std::chrono::milliseconds) override
    {
        actions.push_back(action);
    }

    std::vector<StateAction> actions;
};

class TestFwtState : public FwtState {
public:
    TestFwtState(Scheduler* sched)
        : FwtState(sched)
        , packageRecieved(false)
    {
    }

protected:
    void updatePackage(bmcl::Bytes) override
    {
        packageRecieved = true;
    }

public:
    bool packageRecieved;
};

uint8_t temp[1024];

int main()
{
    PhotonFwt_Init();
    Rc<TestScheduler> _sched = new TestScheduler;
    Rc<TestSink> _sink = new TestSink;
    Rc<TestFwtState> _state = new TestFwtState(_sched.get());
    _state->start(_sink.get());

    while (true) {
        auto packets = std::move(_sink->packets);
        for (const bmcl::Buffer& packet : packets) {
            PhotonReader cmd;
            PhotonReader_Init(&cmd, packet.data(), packet.size());
            auto cmdRv = PhotonFwt_AcceptCmd(&cmd);
        }

        PhotonWriter dest;
        PhotonWriter_Init(&dest, temp, sizeof(temp));
        auto genRv = PhotonFwt_GenAnswer(&dest);

        if (genRv != PhotonError_Ok && genRv != PhotonError_NoDataAvailable) {
            abort();
        }

        _state->acceptData(_sink.get(), bmcl::Bytes(dest.start, dest.current - dest.start));

        if (_state->packageRecieved) {
            break;
        }

        auto actions = std::move(_sched->actions);
        for (const StateAction& action : actions) {
            action();
        }
    }
}
