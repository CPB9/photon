#include <decode/core/Rc.h>
#include <decode/parser/Project.h>
#include <photon/groundcontrol/Exchange.h>
#include <photon/groundcontrol/GroundControl.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <decode/model/NodeViewUpdater.h>

#include "photon/core/Core.Component.h"
#include "photon/exc/Exc.Component.h"

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <caf/send.hpp>
#include <caf/actor_system.hpp>
#include <caf/actor_system_config.hpp>

using namespace photon;

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

class PhotonStream : public caf::event_based_actor {
public:
    explicit PhotonStream(caf::actor_config& cfg)
        : caf::event_based_actor(cfg)
    {
        Photon_Init();
        _current = *PhotonExc_GetMsg();
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SendDataAtom, const bmcl::SharedBytes& data) {
                PhotonExc_AcceptInput(data.data(), data.size());
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
                auto data = bmcl::SharedBytes::create(_current.data, _current.size);
                send(_dest, RecvDataAtom::value, data);
                PhotonExc_PrepareNextMsg();
                _current = *PhotonExc_GetMsg();
                send(this, RepeatStreamAtom::value);
            },
        };
    }

    void on_exit() override
    {
        destroy(_dest);
    }

    PhotonExcMsg _current;
    caf::actor _dest;
};

class FakeHandler : public caf::event_based_actor {
public:
    explicit FakeHandler(caf::actor_config& cfg)
        : caf::event_based_actor(cfg)
    {
    }

    caf::behavior make_behavior() override
    {
        return caf::behavior{
            [this](SetProjectAtom, const ProjectUpdate&) {
            },
            [this](FirmwareErrorEventAtom, const std::string& msg) {
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
            [this](UpdateTmViewAtom, const Rc<NodeViewUpdater>& updater) {
            },
            [this](SetTmViewAtom, const Rc<NodeView>& tmView) {
            },
        };
    }

    const char* name() const override
    {
        return "FakeHandler";
    }
};

int main(int argc, char** argv)
{
    caf::actor_system_config cfg;
    caf::actor_system system(cfg);
    caf::actor stream = system.spawn<PhotonStream>();

    caf::actor handler = system.spawn<FakeHandler>();
    caf::actor gc = system.spawn<decode::GroundControl>(1, 2, stream, handler);

    caf::anon_send(stream, decode::SetStreamDestAtom::value, gc);
    caf::anon_send(gc, decode::StartAtom::value);
    caf::anon_send(stream, decode::StartAtom::value);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    caf::anon_send_exit(stream, caf::exit_reason::user_shutdown);
    caf::anon_send_exit(gc, caf::exit_reason::user_shutdown);
    caf::anon_send_exit(handler, caf::exit_reason::user_shutdown);

    system.await_all_actors_done();
    return 0;
}

