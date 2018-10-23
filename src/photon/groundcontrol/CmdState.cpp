#include "photon/groundcontrol/CmdState.h"
#include "photon/groundcontrol/Atoms.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/parser/Package.h"
#include "decode/parser/Project.h"
#include "photon/model/CmdModel.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/Encoder.h"
#include "photon/model/Decoder.h"
#include "photon/model/Value.h"
#include "photon/model/CmdNode.h"
#include "photon/model/OnboardTime.h"
#include "photon/model/CoderState.h"
#include "photon/groundcontrol/GcInterface.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/GcCmd.h"
#include "photon/groundcontrol/TmParamUpdate.h"
#include "photon/groundcontrol/ProjectUpdate.h"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/Result.h>

#include <caf/sec.hpp>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::TmParamUpdate);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::GcCmd);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Value);

#define CHECK_CMD_INTERFACE(iface)                              \
    if (_ifaces->iface().isNone()) {                    \
        return caf::sec::invalid_argument;              \
    }

namespace photon {

CmdState::CmdState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _exc(exchange)
    , _handler(handler)
{
}

CmdState::~CmdState()
{
}

static CmdState::EncodeResult encodePacket(CmdState::EncodeHandler handler)
{
    uint8_t tmp[1024];
    Encoder writer(tmp, sizeof(tmp));
    if (!handler(&writer)) {
        return writer.errorMsgOr("unknown error").toStdString();
    }
    return PacketRequest(writer.writenData(), StreamType::Cmd);
}

struct RouteUploadState {
    RouteUploadState()
        : currentIndex(0)
        , routeIndex(0)
    {
    }

    caf::actor exchange;
    Rc<const WaypointGcInterface> iface;
    caf::response_promise promise;
    Route route;
    std::size_t currentIndex;
    std::size_t routeIndex;
};

using SendNextPointCmdAtom          = caf::atom_constant<caf::atom("sendnxtp")>;
using SendStartRouteCmdAtom         = caf::atom_constant<caf::atom("sendstrt")>;
using SendClearRouteCmdAtom         = caf::atom_constant<caf::atom("sendclrt")>;
using SendEndRouteCmdAtom           = caf::atom_constant<caf::atom("sendenrt")>;
using SendSetPointActiveCmdAtom     = caf::atom_constant<caf::atom("sendspac")>;
using SendSetRouteInvertedCmdAtom   = caf::atom_constant<caf::atom("sendsrti")>;
using SendSetRouteClosedCmdAtom     = caf::atom_constant<caf::atom("sendsrtc")>;

template <typename T>
class GcActorBase : public caf::event_based_actor {
public:
    GcActorBase(caf::actor_config& cfg,
                caf::actor exchange,
                const T* iface,
                const caf::response_promise& promise,
                const std::string& name)
        : caf::event_based_actor(cfg)
        , _name(name)
        , _exchange(exchange)
        , _iface(iface)
        , _promise(promise)
    {
        monitor(exchange);
        set_down_handler([this](caf::down_msg& dm) {
            quit();
        });
    }

    void on_exit() override
    {
        if (_promise.pending())
            _promise.deliver(caf::sec::request_receiver_down);
        destroy(_exchange);
    }

    const char* name() const override
    {
        return _name.c_str();
    }

protected:
    template <typename R>
    void action(R* actor, bool (R::* encode)(Encoder* dest) const, void (R::* next)(const PacketResponse& resp))
    {
        auto rv = encodePacket(std::bind(encode, actor, std::placeholders::_1));
        if (rv.isErr()) {
            BMCL_CRITICAL() << "error encoding packet";
            _promise.deliver(caf::sec::invalid_argument);
            quit();
            return;
        }
        request(_exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([=](const PacketResponse& resp) {
            if (resp.type == ReceiptType::Ok) {
                (actor->*next)(resp);
                return;
            }
            BMCL_CRITICAL() << "could not deliver packet";
            _promise.deliver(caf::sec::invalid_argument);
            quit();
        },
        [=](const caf::error& err) {
            BMCL_CRITICAL() << err.code();
            _promise.deliver(caf::sec::invalid_argument);
            quit();
        });
    }

    std::string _name;
    caf::actor _exchange;
    Rc<const T> _iface;
    caf::response_promise _promise;
};

using NavActorBase = GcActorBase<WaypointGcInterface>;

class RouteUploadActor : public NavActorBase {
public:
    RouteUploadActor(caf::actor_config& cfg,
                     caf::actor exchange,
                     const WaypointGcInterface* iface,
                     const caf::response_promise& promise,
                     UploadRouteGcCmd&& route)
        : NavActorBase(cfg, exchange, iface, promise, "RouteUploadActor")
        , _route(std::move(route))
        , _currentIndex(0)
    {
    }

    bool encodeRoutePoint(Encoder* dest) const
    {
        return _iface->encodeSetRoutePointCmd(_route.id, _currentIndex, _route.waypoints[_currentIndex], dest);
    }

    bool encodeBeginRoute(Encoder* dest) const
    {
        return _iface->encodeBeginRouteCmd(_route.id, _route.waypoints.size(), dest);
    }

    bool encodeClearRoute(Encoder* dest) const
    {
        return _iface->encodeClearRouteCmd(_route.id, dest);
    }

    bool encodeEndRoute(Encoder* dest) const
    {
        return _iface->encodeEndRouteCmd(_route.id, dest);
    }

    void sendFirstPoint(const PacketResponse&)
    {
        send(this, SendNextPointCmdAtom::value);
    }

    void sendClearRoute(const PacketResponse&)
    {
        send(this, SendClearRouteCmdAtom::value);
    }

    void sendNextPoint(const PacketResponse&)
    {
        _currentIndex++;
        send(this, SendNextPointCmdAtom::value);
    }

    void sendSetActivePoint(const PacketResponse&)
    {
        send(this, SendSetPointActiveCmdAtom::value);
    }

    void sendSetRouteClosed(const PacketResponse&)
    {
        send(this, SendSetRouteClosedCmdAtom::value);
    }

    void sendSetRouteInverted(const PacketResponse&)
    {
        send(this, SendSetRouteInvertedCmdAtom::value);
    }

    bool encodeSetActivePoint(Encoder* dest) const
    {
        return _iface->encodeSetRouteActivePointCmd(_route.id, _route.activePoint, dest);
    }

    bool encodeSetRouteClosed(Encoder* dest) const
    {
        return _iface->encodeSetRouteClosedCmd(_route.id, _route.isClosed, dest);
    }

    bool encodeSetRouteInverted(Encoder* dest) const
    {
        return _iface->encodeSetRouteInvertedCmd(_route.id, _route.isInverted, dest);
    }

    void endUpload(const PacketResponse&)
    {
        _promise.deliver(caf::unit);
        quit();
    }

    caf::behavior make_behavior() override
    {
        send(this, SendStartRouteCmdAtom::value);
        return caf::behavior{
            [this](SendNextPointCmdAtom) {
                if (_currentIndex >= _route.waypoints.size()) {
                    send(this, SendEndRouteCmdAtom::value);
                    return;
                }
                action(this, &RouteUploadActor::encodeRoutePoint, &RouteUploadActor::sendNextPoint);
            },
            [this](SendStartRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeBeginRoute, &RouteUploadActor::sendFirstPoint);
            },
            [this](SendEndRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeEndRoute, &RouteUploadActor::sendSetActivePoint);
            },
            [this](SendSetPointActiveCmdAtom) {
                action(this, &RouteUploadActor::encodeSetActivePoint, &RouteUploadActor::sendSetRouteClosed);
            },
            [this](SendSetRouteClosedCmdAtom) {
                action(this, &RouteUploadActor::encodeSetRouteClosed, &RouteUploadActor::sendSetRouteInverted);
            },
            [this](SendSetRouteInvertedCmdAtom) {
                action(this, &RouteUploadActor::encodeSetRouteInverted, &RouteUploadActor::endUpload);
            },
        };
    }

private:
    UploadRouteGcCmd _route;
    std::size_t _currentIndex;
};

template <typename T>
class OneActionActor : public GcActorBase<T> {
public:
    OneActionActor(caf::actor_config& cfg,
                   caf::actor exchange,
                   const T* iface,
                   const caf::response_promise& promise,
                   const std::string& name)
        : GcActorBase<T>(cfg, exchange, iface, promise, name)
    {
    }

    virtual bool encode(Encoder* dest) const = 0;

    virtual void end(const PacketResponse&)
    {
        this->_promise.deliver(caf::unit);
        this->quit();
    }

    caf::behavior make_behavior() override
    {
        this->send(this, StartAtom::value);
        return caf::behavior{
            [this](StartAtom) {
                this->action(this, &OneActionActor<T>::encode, &OneActionActor<T>::end);
            },
        };
    }
};

using OneActionNavActor = OneActionActor<WaypointGcInterface>;

class SetActiveRouteActor : public OneActionNavActor {
public:
    SetActiveRouteActor(caf::actor_config& cfg,
                        caf::actor exchange,
                        const WaypointGcInterface* iface,
                        const caf::response_promise& promise,
                        SetActiveRouteGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise, "SetActiveRouteActor")
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetActiveRouteCmd(_cmd.id, dest);
    }

private:
    SetActiveRouteGcCmd _cmd;
};

class SetRouteActivePointActor : public OneActionNavActor {
public:
    SetRouteActivePointActor(caf::actor_config& cfg,
                             caf::actor exchange,
                             const WaypointGcInterface* iface,
                             const caf::response_promise& promise,
                             SetRouteActivePointGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise, "SetRouteActivePointActor")
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteActivePointCmd(_cmd.id, _cmd.index, dest);
    }

private:
    SetRouteActivePointGcCmd _cmd;
};

class SetRouteInvertedActor : public OneActionNavActor {
public:
    SetRouteInvertedActor(caf::actor_config& cfg,
                          caf::actor exchange,
                          const WaypointGcInterface* iface,
                          const caf::response_promise& promise,
                          SetRouteInvertedGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise, "SetRouteInvertedActor")
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteInvertedCmd(_cmd.id, _cmd.isInverted, dest);
    }

private:
    SetRouteInvertedGcCmd _cmd;
};

class SetRouteClosedActor : public OneActionNavActor {
public:
    SetRouteClosedActor(caf::actor_config& cfg,
                        caf::actor exchange,
                        const WaypointGcInterface* iface,
                        const caf::response_promise& promise,
                        SetRouteClosedGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise, "SetRouteClosedActor")
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteClosedCmd(_cmd.id, _cmd.isClosed, dest);
    }

private:
    SetRouteClosedGcCmd _cmd;
};

class DownloadRouteInfoActor : public OneActionNavActor {
public:
    DownloadRouteInfoActor(caf::actor_config& cfg,
                           caf::actor exchange,
                           const WaypointGcInterface* iface,
                           const caf::response_promise& promise,
                           caf::actor handler)
        : OneActionNavActor(cfg, exchange, iface, promise, "DownloadRouteInfoActor")
        , _handler(handler)
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeGetRoutesInfoCmd(dest);
    }

    void end(const PacketResponse& resp) override
    {
        Decoder dec(resp.payload.view());
        AllRoutesInfo info;
        if (!_iface->decodeGetRoutesInfoResponse(&dec, &info)) {
            _promise.deliver(caf::sec::invalid_argument);
        } else {
            send(_handler, UpdateTmParams::value, TmParamUpdate(std::move(info)));
            _promise.deliver(caf::unit);
        }
        quit();
    }

private:
    caf::actor _handler;
};

class AddClientActor : public OneActionActor<UdpGcInterface> {
public:
    AddClientActor(caf::actor_config& cfg,
                   caf::actor exchange,
                   const UdpGcInterface* iface,
                   const caf::response_promise& promise,
                   AddClientGcCmd&& cmd)
        : OneActionActor<UdpGcInterface>(cfg, exchange, iface, promise, "AddClientActor")
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return this->_iface->encodeAddClient(_cmd.id, _cmd.address, dest);
    }

    void end(const PacketResponse& resp) override
    {
        (void)resp;
        this->_promise.deliver(caf::unit);
        this->quit();
    }

private:
    AddClientGcCmd _cmd;
};

using SendGetInfoAtom       = caf::atom_constant<caf::atom("sendgein")>;
using SendGetNextPointAtom  = caf::atom_constant<caf::atom("sendgenp")>;

class DownloadRouteActor : public NavActorBase {
public:
    DownloadRouteActor(caf::actor_config& cfg,
                       caf::actor exchange,
                       const WaypointGcInterface* iface,
                       const caf::response_promise& promise,
                       DownloadRouteGcCmd&& cmd,
                       caf::actor handler)
        : NavActorBase(cfg, exchange, iface, promise, "DownloadRouteActor")
        , _currentIndex(0)
        , _handler(handler)
    {
        _route.id = cmd.id;
    }

    bool encodeGetInfo(Encoder* dest) const
    {
        return _iface->encodeGetRouteInfoCmd(_route.id, dest);
    }

    bool encodeGetRoutePoint(Encoder* dest) const
    {
        return _iface->encodeGetRoutePointCmd(_route.id, _currentIndex, dest);
    }

    void unpackInfoAndSendGetPoint(const PacketResponse& resp)
    {
        Decoder dec(resp.payload.view());
        if (!_iface->decodeGetRouteInfoResponse(&dec, &_route.route.info)) {
            _promise.deliver(caf::sec::invalid_argument);
            return;
        }
        send(this, SendGetNextPointAtom::value);
    }

    void unpackPointAndSendGetPoint(const PacketResponse& resp)
    {
        Decoder dec(resp.payload.view());
        Waypoint point;
        if (!_iface->decodeGetRoutePointResponse(&dec, &point)) {
            _promise.deliver(caf::sec::invalid_argument);
            return;
        }
        _route.route.waypoints.push_back(point);
        _currentIndex++;
        send(this, SendGetNextPointAtom::value);
    }

    void endDownload()
    {
        _promise.deliver(caf::unit);
        send(_handler, UpdateTmParams::value, TmParamUpdate(std::move(_route)));
        quit();
    }

    caf::behavior make_behavior() override
    {
        send(this, SendGetInfoAtom::value);
        return caf::behavior{
            [this](SendGetNextPointAtom) {
                if (_currentIndex >= _route.route.info.size) {
                    endDownload();
                    return;
                }
                action(this, &DownloadRouteActor::encodeGetRoutePoint, &DownloadRouteActor::unpackPointAndSendGetPoint);
            },
            [this](SendGetInfoAtom) {
                action(this, &DownloadRouteActor::encodeGetInfo, &DownloadRouteActor::unpackInfoAndSendGetPoint);
            },
        };
    }

private:
    RouteTmParam _route;
    std::size_t _currentIndex;
    caf::actor _handler;
};

using SendBeginFileAtom     = caf::atom_constant<caf::atom("sendbef")>;
using SendNextFileChunkAtom = caf::atom_constant<caf::atom("sendnfc")>;
using SendEndFileAtom       = caf::atom_constant<caf::atom("sendenf")>;

class UploadFileActor : public GcActorBase<FileGcInterface> {
public:
    UploadFileActor(caf::actor_config& cfg,
                    caf::actor exchange,
                    const FileGcInterface* iface,
                    const caf::response_promise& promise,
                    UploadFileGcCmd&& cmd)
        : GcActorBase<FileGcInterface>(cfg, exchange, iface, promise, "UploadFileActor")
        , _reader(cmd.reader)
        , _id(cmd.id)
    {
    }

    bool encodeBeginFile(Encoder* dest) const
    {
        return _iface->encodeBeginFile(_id, _reader->size(), dest);
    }

    bool encodeEndFile(Encoder* dest) const
    {
        return _iface->encodeEndFile(_id, _reader->size(), dest);
    }

    bool encodeNextChunk(Encoder* dest) const
    {
        std::size_t offset = _reader->offset();
        bmcl::Bytes chunk = _reader->readNext(_iface->maxFileChunkSize());
        return _iface->encodeWriteFile(_id, offset, chunk, dest);
    }

    void sendNextChunk(const PacketResponse&)
    {
        send(this, SendNextFileChunkAtom::value);
    }

    void endUpload(const PacketResponse&)
    {
        _promise.deliver(caf::unit);
        quit();
    }

    const char* name() const override
    {
        return "FileUploadActor";
    }

    caf::behavior make_behavior() override
    {
        send(this, SendBeginFileAtom::value);
        return caf::behavior{
            [this](SendBeginFileAtom) {
                action(this, &UploadFileActor::encodeBeginFile, &UploadFileActor::sendNextChunk);
            },
            [this](SendNextFileChunkAtom) {
                if (_reader->hasData()) {
                    action(this, &UploadFileActor::encodeNextChunk, &UploadFileActor::sendNextChunk);
                    return;
                }
                send(this, SendEndFileAtom::value);
            },
            [this](SendEndFileAtom) {
                action(this, &UploadFileActor::encodeEndFile, &UploadFileActor::endUpload);
            },
        };
    }

private:
    Rc<decode::DataReader> _reader;
    std::uintmax_t _id;
};

caf::behavior CmdState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            _valueInfoCache = update->cache();
            _model = new CmdModel(update->device(), update->cache(), bmcl::None);
            _proj = update->project();
            _dev = update->device();
            _ifaces = new AllGcInterfaces(update->device(), update->cache());
            if (!_ifaces->errors().empty()) {
                BMCL_CRITICAL() << _ifaces->errors();
            }
//             assert(_ifaces->waypointInterface().isSome());
//             UploadRouteGcCmd rt;
//             Waypoint wp;
//             wp.position = Position{1,2,3};
//             wp.speed.emplace(7);
//             wp.action = SnakeWaypointAction{};
//             rt.waypoints.push_back(wp);
//             rt.waypoints.push_back(wp);
//             rt.id = 0;
//             rt.isClosed = true;
//             rt.isInverted = true;
//             rt.activePoint.emplace(2);
//             send(this, SendGcCommandAtom::value, GcCmd(rt));
//             SetActiveRouteGcCmd cmd;
//             cmd.id.emplace(0);
//             send(this, SendGcCommandAtom::value, GcCmd(cmd));
//             SetRouteActivePointGcCmd cmd2;
//             cmd2.id = 0;
//             cmd2.index.emplace(2);
//             send(this, SendGcCommandAtom::value, GcCmd(cmd2));
//             SetRouteInvertedGcCmd cmd3;
//             cmd3.id = 0;
//             cmd3.isInverted = true;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd3));
//             SetRouteClosedGcCmd cmd4;
//             cmd4.id = 0;
//             cmd4.isClosed = true;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd4));
//             DownloadRouteInfoGcCmd cmd5;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd5));
//             DownloadRouteGcCmd cmd6;
//             cmd6.id = 0;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd6));
//             UploadFileGcCmd cmd7;
//             cmd7.id = 12;
//             cmd7.reader = new MemDataReader(std::vector<uint8_t>(3000, 4));
//             send(this, SendGcCommandAtom::value, GcCmd(cmd7));
        },
        [this](SendCustomCommandAtom, const std::string& compName, const std::string& cmdName, const std::vector<Value>& args) {
            return sendCustomCmd(compName, cmdName, args);
        },
        [this](SendGcCommandAtom, GcCmd& cmd) -> caf::result<void> {
            //FIXME: check per command
            if (_proj.isNull()) {
                return caf::sec::invalid_argument;
            }

            caf::response_promise promise = make_response_promise();
            switch (cmd.kind()) {
            case GcCmdKind::None:
                return caf::sec::invalid_argument;
            case GcCmdKind::UploadRoute:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<RouteUploadActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<UploadRouteGcCmd>()));
                return promise;
            }
            case GcCmdKind::SetActiveRoute:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<SetActiveRouteActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetActiveRouteGcCmd>()));
                return promise;
            }
            case GcCmdKind::SetRouteActivePoint:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<SetRouteActivePointActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteActivePointGcCmd>()));
                return promise;
            }
            case GcCmdKind::SetRouteInverted:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<SetRouteInvertedActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteInvertedGcCmd>()));
                return promise;
            }
            case GcCmdKind::SetRouteClosed:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<SetRouteClosedActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteClosedGcCmd>()));
                return promise;
            }
            case GcCmdKind::DownloadRouteInfo:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<DownloadRouteInfoActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, _handler);
                return promise;
            }
            case GcCmdKind::DownloadRoute:
            {
                CHECK_CMD_INTERFACE(waypointInterface);
                spawn<DownloadRouteActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<DownloadRouteGcCmd>()), _handler);
                return promise;
            }
            case GcCmdKind::UploadFile:
            {
                CHECK_CMD_INTERFACE(fileInterface);
                spawn<UploadFileActor>(_exc, _ifaces->fileInterface().unwrap(), promise, std::move(cmd.as<UploadFileGcCmd>()));
                return promise;
            }
            case GcCmdKind::GroupCreate:
            case GcCmdKind::GroupRemove:
            case GcCmdKind::GroupAttach:
            case GcCmdKind::GroupDetach:
                return caf::sec::invalid_argument;
            case GcCmdKind::AddClient:
            {
                CHECK_CMD_INTERFACE(udpInterface);
                spawn<AddClientActor>(_exc, _ifaces->udpInterface().unwrap(), promise, std::move(cmd.as<AddClientGcCmd>()));
                return promise;
            }
            };
            return caf::sec::invalid_argument;
        },
    };
}

caf::result<PacketResponse> CmdState::sendCustomCmd(bmcl::StringView compName, bmcl::StringView cmdName, const std::vector<Value>& args)
{
    if (_proj.isNull()) {
        return caf::error();
    }
    auto mod = _proj->package()->moduleWithName(compName);
    if (mod.isNone()) {
        return caf::error();
    }

    auto comp = mod.unwrap()->component();
    if (comp.isNone()) {
        return caf::error();
    }

    auto cmd = comp.unwrap()->cmdWithName(cmdName);
    if (cmd.isNone()) {
        return caf::error();
    }

    Rc<CmdNode> cmdNode = new CmdNode(comp.unwrap(), cmd.unwrap(), _valueInfoCache.get(), bmcl::None);
    if (!cmdNode->setValues(args)) {
        return caf::error();
    }

    bmcl::Buffer dest;
    dest.reserve(1024);
    CoderState ctx(OnboardTime::now());
    if (cmdNode->encode(&ctx, &dest)) {
        PacketRequest req(dest, StreamType::Cmd);
        caf::response_promise promise = make_response_promise();
        request(_exc, caf::infinite, SendReliablePacketAtom::value, std::move(req)).then([promise](const PacketResponse& response) mutable {
            promise.deliver(response);
        },
        [promise](const caf::error& err) mutable {
            promise.deliver(err);
        });
        return promise;
    }
    return caf::error();
}

void CmdState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}

const char* CmdState::name() const
{
    return "CmdStateActor";
}
}
