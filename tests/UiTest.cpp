#include "UiTest.h"

#include <decode/ui/QNodeModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/FirmwareWidget.h>
#include <decode/ui/FirmwareStatusWidget.h>
#include <decode/groundcontrol/AllowUnsafeMessageType.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/model/NodeView.h>
#include <decode/model/ValueInfoCache.h>
#include <decode/model/NodeViewUpdater.h>
#include <decode/parser/Project.h>
#include <decode/core/Diagnostics.h>
#include <decode/model/CmdModel.h>
#include <decode/groundcontrol/TmParamUpdate.h>
#include <decode/groundcontrol/Packet.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <unordered_set>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);

using namespace decode;

UiActor::UiActor(caf::actor_config& cfg, caf::actor stream, int& argc, char** argv)
    : caf::event_based_actor(cfg)
    , _argc(argc)
    , _argv(argv)
    , _stream(stream)
    , _widgetShown(false)
{
    _gc = spawn<decode::GroundControl>(_stream, this);
    send(_stream, SetStreamDestAtom::value, _gc);
}

UiActor::~UiActor()
{
}

caf::behavior UiActor::make_behavior()
{
    return caf::behavior{
        [this](RepeatEventLoopAtom) {
            _app->sync();
            if (_widgetShown && !_widget->isVisible()) {
                BMCL_DEBUG() << "quitting";
                _app.reset();
                quit();
                return;
            }
            delayed_send<caf::message_priority::high>(this, std::chrono::milliseconds(10), RepeatEventLoopAtom::value);
        },
        [this](UpdateTmViewAtom, const Rc<NodeViewUpdater>& updater) {
            _widget->applyTmUpdates(updater.get());
        },
        [this](UpdateTmParams, const TmParamUpdate& update) {
            (void)update;
            switch (update.kind()) {
            case TmParamKind::None:
                return;
            case TmParamKind::LatLon: {
                const LatLon& latLon = update.as<LatLon>();
                (void)latLon;
                return;
            }
            case TmParamKind::Orientation: {
                const Orientation& orientation = update.as<Orientation>();
                (void)orientation;
                return;
            }
            }
        },
        [this](LogAtom, const std::string& msg) {
            BMCL_DEBUG() << msg;
        },
        [this](SetProjectAtom, const Project::ConstPointer& proj, const Device::ConstPointer& dev) {
            _widgetShown = true;
            _widget->showMaximized();
            Rc<CmdModel> cmdNode = new CmdModel(dev.get(), new ValueInfoCache(proj->package()), bmcl::None);
            _widget->setRootCmdNode(cmdNode.get());
        },
        [this](SetTmViewAtom, const Rc<NodeView>& tmView) {
            _widget->setRootTmNode(tmView.get());
        },
        [this](FirmwareErrorEventAtom, const std::string& msg) {
            _statusWidget->firmwareError(msg);
        },
        [this](ExchangeErrorEventAtom, const std::string& msg) {
            BMCL_CRITICAL() << "exc: " + msg;
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
        [this](FirmwareProgressEventAtom, std::size_t size, std::size_t totalSize) {
            _statusWidget->firmwareDownloadProgress(size);
        },
        [this](FirmwareSizeRecievedEventAtom, std::size_t size) {
            _statusWidget->beginFirmwareDownload(size);
        },
        [this](FirmwareHashDownloadedEventAtom, const std::string& name, const bmcl::SharedBytes& hash) {
            _statusWidget->endHashDownload(name, hash.view());
        },
        [this](StartAtom) {
            _app = bmcl::makeUnique<QApplication>(_argc, _argv);
            _statusWidget = bmcl::makeUnique<FirmwareStatusWidget>();
            _widget = bmcl::makeUnique<FirmwareWidget>();

            QObject::connect(_app.get(), &QApplication::lastWindowClosed, _app.get(), [this]() {
                BMCL_DEBUG() << "quitting";
                quit();
            });

            QObject::connect(_widget.get(), &FirmwareWidget::unreliablePacketQueued, _widget.get(), [this](const PacketRequest& packet) {
                send(_gc, SendUnreliablePacketAtom::value, packet);
            });

            delayed_send(_stream, std::chrono::milliseconds(100), decode::StartAtom::value);
            delayed_send(_gc, std::chrono::milliseconds(200), decode::StartAtom::value);
            delayed_send(this, std::chrono::milliseconds(10), RepeatEventLoopAtom::value);
            //send(_gc, EnableLoggindAtom::value, true);
        },
    };
}

const char* UiActor::name() const
{
    return "UiTestActor";
}

void UiActor::on_exit()
{
    send_exit(_gc, caf::exit_reason::user_shutdown);
    send_exit(_stream, caf::exit_reason::user_shutdown);
    destroy(_gc);
    destroy(_stream);
}
