/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/TmState.h"
#include "photon/groundcontrol/Atoms.h"

#include "decode/ast/Type.h"
#include "decode/parser/Project.h"
#include "photon/model/TmModel.h"
#include "photon/model/NodeView.h"
#include "photon/model/NodeViewUpdater.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/ValueNode.h"
#include "photon/model/FindNode.h"
#include "photon/model/CoderState.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "photon/groundcontrol/TmParamUpdate.h"
#include "photon/groundcontrol/ProjectUpdate.h"

#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::NodeView::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::NodeViewUpdater::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::TmParamUpdate);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Value);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::NumberedSub);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

#define TM_LOG(msg)         \
    if (_isLoggingEnabled) { \
        logMsg(msg);         \
    }

namespace photon {

TmState::NamedSub::NamedSub(const ValueNode* node, const std::string& path,const caf::actor& dest)
    : node(node)
    , path(path)
    , actor(dest)
{
}

TmState::NamedSub::~NamedSub()
{
}

TmState::TmState(caf::actor_config& cfg, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _handler(handler)
    , _isLoggingEnabled(false)
{
}

TmState::~TmState()
{
}

void TmState::logMsg(std::string&& msg)
{
    send(_handler, LogAtom::value, std::move(msg));
}

void TmState::on_exit()
{
    destroy(_handler);
}

caf::behavior TmState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            if (_project == update->project() && _dev == update->device())
                return;
            _project = update->project();
            _dev = update->device();

            _model = new TmModel(update->device(), update->cache());
            Rc<NodeView> statusView = new NodeView(_model->statusesNode());
            Rc<NodeView> eventView = new NodeView(_model->eventsNode());
            Rc<NodeView> statsView = new NodeView(_model->statisticsNode());
            send(_handler, SetTmViewAtom::value, statusView, eventView, statsView);
            _updateCount++;

            for (NamedSub& sub : _namedSubs) {
                auto rv = findNode(_model->statusesNode(), sub.path);
                if (rv.isErr()) {
                    sub.actor = caf::actor(); //TODO: remove
                    continue;
                }
                Node* node = rv.unwrap().get();
                ValueNode* valueNode = dynamic_cast<ValueNode*>(node);
                if (!valueNode) {
                    sub.actor = caf::actor(); //TODO: remove
                    continue;
                }
                sub.node = valueNode;
            }
        },
        [this](RecvPacketPayloadAtom, const PacketHeader& header, const bmcl::SharedBytes& data) {
            acceptData(header, data.view());
        },
        [this](PushTmUpdatesAtom, uint64_t count) {
            if (count != _updateCount) {
                return;
            }
            pushTmUpdates();
        },
        [this](SubscribeNumberedTmAtom, const NumberedSub& sub, const caf::actor& dest) {
            return subscribeTm(sub, dest);
        },
        [this](SubscribeNamedTmAtom, const std::string& path, const caf::actor& dest) {
            return subscribeTm(path, dest);
        },
        [this](StartAtom) {
            (void)this;
        },
        [this](StopAtom) {
            (void)this;
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            (void)this;
            _isLoggingEnabled = isEnabled;
        },
    };
}

bool TmState::subscribeTm(const std::string& path, const caf::actor& dest)
{
    if (_model.isNull())
        return false;

    auto rv = findNode(_model->statusesNode(), path);
    if (rv.isErr()) {
        return false;
    }
    Node* node = rv.unwrap().get();
    ValueNode* valueNode = dynamic_cast<ValueNode*>(node);
    if (!valueNode) {
        return false;
    }
    auto it = std::find_if(_namedSubs.begin(), _namedSubs.end(), [path](const NamedSub& sub) {return sub.path == path; });
    if (it != _namedSubs.end())
    {
        _namedSubs.erase(it);
    }
    _namedSubs.emplace_back(valueNode, path, dest);
    return true;
}

bool TmState::subscribeTm(const NumberedSub& sub, const caf::actor& dest)
{
    auto pair = _numberedSubs.emplace(sub, std::vector<caf::actor>());
    pair.first->second.emplace_back(dest);
    return true;
}

void TmState::pushTmUpdates()
{
    Rc<NodeViewUpdater> statusUpdater = new NodeViewUpdater(_model->statusesNode());
    _model->statusesNode()->collectUpdates(statusUpdater.get());

    Rc<NodeViewUpdater> eventUpdater = new NodeViewUpdater(_model->eventsNode());
    _model->eventsNode()->collectUpdates(eventUpdater.get());

    Rc<NodeViewUpdater> statisticsUpdater = new NodeViewUpdater(_model->statisticsNode());
    _model->statisticsNode()->collectUpdates(statisticsUpdater.get());
    send(_handler, UpdateTmViewAtom::value, statusUpdater, eventUpdater, statisticsUpdater);
    for (const NamedSub& sub : _namedSubs) {
        send(sub.actor, sub.node->value(), sub.path);
    }
    _updateCount++;
}

template <typename T>
void TmState::initTypedNode(const char* name, Rc<T>* dest)
{
    auto node = findTypedNode<T>(_model.get(), name);
    if (node.isOk()) {
        *dest = node.unwrap();
    }
}

void TmState::reportError(std::string&& msg)
{
    send(_handler, ExchangeErrorEventAtom::value, std::move(msg));
}

void TmState::acceptData(const PacketHeader& header, bmcl::Bytes packet)
{
    if (_model.isNull()) {
        reportError("recieved tm msg while model uninitialized");
        return;
    }

    if (packet.isEmpty()) {
        reportError("recieved empty tm packet");
        return;
    }

    CoderState ctx(header.tickTime);

    bmcl::MemReader src(packet);
    while (src.sizeLeft() != 0) {
        if (src.sizeLeft() < 2) {
            reportError("recieved tm packet with stray data");
            return;
        }

        uint64_t compNum;
        if (!src.readVarUint(&compNum)) {
            reportError("failed to read tm msg component number");
            return;
        }
        if (compNum > std::numeric_limits<uint32_t>::max()) {
            reportError("tm msg component number too big");
            return;
        }

        uint64_t msgNum;
        if (!src.readVarUint(&msgNum)) {
            reportError("failed to read tm msg message number");
            return;
        }
        if (msgNum > std::numeric_limits<uint32_t>::max()) {
            reportError("tm msg message number too big");
            return;
        }

        const uint8_t* begin = src.current();

        TM_LOG("parsing tm msg: " + std::to_string(compNum) + " " + std::to_string(msgNum));

        if (!_model->acceptTmMsg(&ctx, compNum, msgNum, &src)) {
            reportError("failed to parse tm message: " + ctx.error());
            return;
        }

        bmcl::Bytes view(begin, src.current());
        NumberedSub sub(compNum, msgNum);
        auto it = _numberedSubs.find(sub);
        if (it != _numberedSubs.end()) {
            bmcl::SharedBytes data = bmcl::SharedBytes::create(view);
            for (const caf::actor& actor : it->second) {
                send(actor, sub, data);
            }
        }

    }
    pushTmUpdates();
}
}
