#include "UiTest.h"

#include <photon/ui/QNodeModel.h>
#include <photon/ui/QNodeViewModel.h>
#include <photon/ui/QCmdModel.h>
#include <photon/ui/FirmwareWidget.h>
#include <photon/ui/FirmwareStatusWidget.h>
#include <photon/groundcontrol/AllowUnsafeMessageType.h>
#include <photon/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <photon/model/NodeView.h>
#include <photon/model/ValueInfoCache.h>
#include <photon/model/NodeViewUpdater.h>
#include <decode/parser/Project.h>
#include <decode/core/Diagnostics.h>
#include <photon/model/CmdModel.h>
#include <photon/groundcontrol/TmParamUpdate.h>
#include <photon/groundcontrol/Packet.h>
#include <photon/groundcontrol/ProjectUpdate.h>

#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QWidget>

#include <chrono>
#include <unordered_set>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Value);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(std::vector<photon::Value>);

using namespace photon;

UiActor::UiActor(caf::actor_config& cfg, uint64_t srcAddress, uint64_t destAddress, caf::actor stream, int& argc, char** argv)
    : caf::event_based_actor(cfg)
    , _argc(argc)
    , _argv(argv)
    , _stream(stream)
    , _widgetShown(false)
    , _param2Value(0)
{
    _gc = spawn<GroundControl>(srcAddress, destAddress, _stream, this);
    send(_stream, SetStreamDestAtom::value, _gc);
}

UiActor::~UiActor()
{
}

caf::behavior testSubActor(caf::event_based_actor* self)
{
    return caf::behavior{
        [](const Value& value, const std::string& path) {
            BMCL_DEBUG() << path << ": " << value.asUnsigned();
        }
    };
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
        [this](RepeatParam2Atom) {
            std::vector<Value> args = {Value::makeUnsigned(_param2Value)};
            _param2Value++;
            request(_gc, caf::infinite, SendCustomCommandAtom::value, "test", "setParam2", std::move(args)).then([](const PacketResponse& resp) {
                (void)resp;
            });
            delayed_send(this, std::chrono::milliseconds(500), RepeatParam2Atom::value);
        },
        [this](UpdateTmViewAtom, const Rc<NodeViewUpdater>& updater) {
            _widget->applyTmUpdates(updater.get());
        },
        [this](UpdateTmParams, const TmParamUpdate& update) {
            (void)update;
            switch (update.kind()) {
            case TmParamKind::None:
                return;
            case TmParamKind::Position: {
                const Position& latLon = update.as<Position>();
                (void)latLon;
                return;
            }
            case TmParamKind::Orientation: {
                const Orientation& orientation = update.as<Orientation>();
                (void)orientation;
                return;
            }
            case TmParamKind::RoutesInfo: {
                BMCL_DEBUG() << "accepted routes info";
                return;
            }
            }
        },
        [this](LogAtom, const std::string& msg) {
            BMCL_DEBUG() << msg;
        },
        [this](SetProjectAtom, const ProjectUpdate& update) {
            _widgetShown = true;
            _widget->resize(800, 600);
            _widget->showMaximized();
            Rc<CmdModel> cmdNode = new CmdModel(update.device.get(), update.cache.get(), bmcl::None);
            _widget->setRootCmdNode(update.cache.get(), cmdNode.get());
            _testSub = spawn(testSubActor);
            request(_gc, caf::infinite, SubscribeTmAtom::value, std::string("test.param2"), _testSub);
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
            bmcl::Rc<Node> emptyNode = new Node(bmcl::None);
            std::unique_ptr<QNodeViewModel> paramViewModel = bmcl::makeUnique<QNodeViewModel>(new NodeView(emptyNode.get()));
            _widget = bmcl::makeUnique<FirmwareWidget>(std::move(paramViewModel));

            QObject::connect(_app.get(), &QApplication::lastWindowClosed, _app.get(), [this]() {
                BMCL_DEBUG() << "quitting";
                quit();
            });

            QObject::connect(_widget.get(), &FirmwareWidget::unreliablePacketQueued, _widget.get(), [this](const PacketRequest& packet) {
                send(_gc, SendUnreliablePacketAtom::value, packet);
            });

            QObject::connect(_widget.get(), &FirmwareWidget::reliablePacketQueued, _widget.get(), [this](const PacketRequest& packet) {
                request(_gc, caf::infinite, SendReliablePacketAtom::value, packet).then([this](const PacketResponse& response) {
                    _widget->acceptPacketResponse(response);
                });
            });

            delayed_send(_stream, std::chrono::milliseconds(100), StartAtom::value);
            delayed_send(_gc, std::chrono::milliseconds(200), StartAtom::value);
            delayed_send(this, std::chrono::milliseconds(10), RepeatEventLoopAtom::value);
            delayed_send(this, std::chrono::milliseconds(500), RepeatParam2Atom::value);
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
