#include "UiTest.h"

#include <decode/ui/QNodeModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/ui/FirmwareWidget.h>
#include <decode/ui/FirmwareStatusWidget.h>
#include <decode/model/Model.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/model/NodeView.h>
#include <decode/model/NodeViewUpdater.h>
#include <decode/parser/Project.h>
#include <decode/core/Diagnostics.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>

#include <chrono>
#include <unordered_set>

using namespace decode;

UiActor::UiActor(caf::actor_config& cfg, int& argc, char** argv)
    : caf::event_based_actor(cfg)
    , _argc(argc)
    , _argv(argv)
{
}

UiActor::~UiActor()
{
}

caf::behavior UiActor::make_behavior()
{
    return caf::behavior{
        [this](StartEventLoopAtom) {
            _app = bmcl::makeUnique<QApplication>(_argc, _argv);
            _statusWidget = bmcl::makeUnique<FirmwareStatusWidget>();
            _widget = bmcl::makeUnique<FirmwareWidget>();
            delayed_send(this, std::chrono::milliseconds(1), RepeatEventLoopAtom::value);
        },
        [this](RepeatEventLoopAtom) {
            _app->processEvents();
            delayed_send(this, std::chrono::milliseconds(1), RepeatEventLoopAtom::value);
        },
        [this](SetProjectAtom, const Rc<const Project>&, const Rc<const Device>&) {
            _widget->show();
        },
        [this](SetTmViewAtom, const Rc<NodeView>& tmView) {
            _widget->setRootTmNode(tmView.get());
        },
        [this](UpdateTmViewAtom, const Rc<NodeViewUpdater>& updater) {
            _widget->applyTmUpdates(updater.get());
        },
        [this](FirmwareErrorEventAtom, const std::string& msg) {
            _statusWidget->firmwareError(msg);
        },
        [this](FirmwareDownloadStartedEventAtom) {
            _statusWidget->resize(640, 300);
            _statusWidget->move(_app->desktop()->screen()->rect().center() - _statusWidget->rect().center());
            _statusWidget->show();
        },
        [this](FirmwareDownloadFinishedEventAtom) {
            _statusWidget->close();
        },
        [this](FirmwareStartCmdSentEventAtom) {
            _statusWidget->beginFirmwareStartCommand();
        },
        [this](FirmwareStartCmdPassedEventAtom) {
            _statusWidget->endFirmwareStartCommand();
        },
        [this](FirmwareProgressEventAtom, std::size_t size) {
            _statusWidget->firmwareDownloadProgress(size);
        },
        [this](FirmwareSizeRecievedEventAtom, std::size_t size) {
            _statusWidget->beginFirmwareDownload(size);
        },
        [this](FirmwareHashDownloadedEventAtom, const std::string& name, const bmcl::SharedBytes& hash) {
            _statusWidget->endHashDownload(name, hash.view());
        },
        [this](StartAtom, std::chrono::milliseconds delay, const caf::actor& act) {
            delayed_send(act, delay, StartAtom::value);
        },
    };
}

const char* UiActor::name() const
{
    return "UiTestActor";
}
