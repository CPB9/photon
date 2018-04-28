#pragma once

#include <decode/core/Rc.h>
#include <photon/groundcontrol/GroundControl.h>
#include <photon/groundcontrol/Atoms.h>
#include <photon/groundcontrol/Exchange.h>

#include <bmcl/String.h>

#include <QApplication>

#include <caf/send.hpp>
#include <caf/actor_system.hpp>
#include <caf/actor_system_config.hpp>
#include <caf/event_based_actor.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>

namespace photon {
class FirmwareWidget;
class FirmwareStatusWidget;
}

namespace photongen {
class Validator;
}

using RepeatEventLoopAtom = caf::atom_constant<caf::atom("reploop")>;
using RepeatParam2Atom    = caf::atom_constant<caf::atom("reppar2")>;

class UiActor : public caf::event_based_actor {
public:
    UiActor(caf::actor_config& cfg, uint64_t srcAddress, uint64_t destAddress, caf::actor stream, int& argc, char** argv);
    ~UiActor();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    std::unique_ptr<QApplication> _app;
    std::unique_ptr<photon::FirmwareWidget> _widget;
    std::unique_ptr<photon::FirmwareStatusWidget> _statusWidget;
    int _argc;
    char** _argv;
    caf::actor _stream;
    caf::actor _gc;
    caf::actor _testSub;
    bool _widgetShown;
    uint16_t _param2Value;
    photon::Rc<photongen::Validator> _validator;

};

template <typename S, typename... A>
int runUiTest(int argc, char** argv, uint64_t srcAddress, uint64_t destAddress, A&&... args)
{
    caf::actor_system_config cfg;
    caf::actor_system system(cfg);
    caf::actor stream = system.spawn<S>(std::forward<A>(args)...);
    caf::actor uiActor = system.spawn<UiActor, caf::detached>(srcAddress, destAddress, stream, argc, argv);

    caf::anon_send(uiActor, photon::StartAtom::value);
    system.await_all_actors_done();
    return 0;
}
