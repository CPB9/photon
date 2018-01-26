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
#include "photon/model/DecoderCtx.h"
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
{
}

TmState::~TmState()
{
}

void TmState::on_exit()
{
    destroy(_handler);
}

caf::behavior TmState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            _model = new TmModel(update->device(), update->cache());
            initTmNodes();
            Rc<NodeView> view = new NodeView(_model.get());
            send(_handler, SetTmViewAtom::value, view);
            _updateCount++;
            delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value, _updateCount);
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
            (void)isEnabled;
        },
    };
}

bool TmState::subscribeTm(const std::string& path, const caf::actor& dest)
{
    auto rv = findNode(_model.get(), path);
    if (rv.isErr()) {
        return false;
    }
    Node* node = rv.unwrap().get();
    ValueNode* valueNode = dynamic_cast<ValueNode*>(node);
    if (!valueNode) {
        return false;
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
    Rc<NodeViewUpdater> updater = new NodeViewUpdater(_model.get());
    _model->collectUpdates(updater.get());
    if (_features.hasPosition) {
        Position pos;
        updateParam(_latNode, &pos.latLon.latitude);
        updateParam(_lonNode, &pos.latLon.longitude);
        updateParam(_altNode, &pos.altitude);
        send(_handler, UpdateTmParams::value, TmParamUpdate(pos));
    }
    if (_features.hasVelocity) {
        Velocity3 vel;
        updateParam(_velXNode, &vel.x);
        updateParam(_velYNode, &vel.y);
        updateParam(_velZNode, &vel.z);
        send(_handler, UpdateTmParams::value, TmParamUpdate(vel));
    }
    if (_features.hasOrientation) {
        Orientation orientation;
        updateParam(_headingNode, &orientation.heading);
        updateParam(_pitchNode, &orientation.pitch);
        updateParam(_rollNode, &orientation.roll);
        send(_handler, UpdateTmParams::value, TmParamUpdate(orientation));
    }
    send(_handler, UpdateTmViewAtom::value, updater);
    for (const NamedSub& sub : _namedSubs) {
        send(sub.actor, sub.node->value(), sub.path);
    }
    _updateCount++;
    delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value, _updateCount);
}

template <typename T>
void TmState::updateParam(const Rc<NumericValueNode<T>>& src, T* dest, T defaultValue)
{
    if (src) {
        auto value = src->rawValue();
        if (value.isSome()) {
            *dest = value.unwrap();
        } else {
            *dest = defaultValue;
        }
    }
}

template <typename T>
void TmState::initTypedNode(const char* name, Rc<T>* dest)
{
    auto node = findTypedNode<T>(_model.get(), name);
    if (node.isOk()) {
        *dest = node.unwrap();
    }
}

void TmState::initTmNodes()
{
    initTypedNode("nav.latLon.latitude", &_latNode);
    initTypedNode("nav.latLon.longitude", &_lonNode);
    initTypedNode("nav.altitude", &_altNode);
    _features.hasPosition = _latNode || _lonNode || _altNode;
    initTypedNode("nav.velocity.x", &_velXNode);
    initTypedNode("nav.velocity.y", &_velYNode);
    initTypedNode("nav.velocity.z", &_velZNode);
    _features.hasVelocity = _velXNode || _velYNode || _velZNode;
    initTypedNode("nav.orientation.heading", &_headingNode);
    initTypedNode("nav.orientation.pitch", &_pitchNode);
    initTypedNode("nav.orientation.roll", &_rollNode);
    _features.hasOrientation = _headingNode || _pitchNode || _rollNode;
}

void TmState::acceptData(const PacketHeader& header, bmcl::Bytes packet)
{
    if (!_model) {
        return;
    }

    DecoderCtx ctx(header.tickTime);

    bmcl::MemReader src(packet);
    while (src.sizeLeft() != 0) {
        if (src.sizeLeft() < 2) {
            //TODO: report error
            return;
        }

        uint16_t msgSize = src.readUint16Le();
        if (src.sizeLeft() < msgSize) {
            //TODO: report error
            return;
        }

        bmcl::MemReader msg(src.current(), msgSize);
        src.skip(msgSize);

        uint64_t compNum;
        if (!msg.readVarUint(&compNum)) {
            //TODO: report error
            return;
        }
        if (compNum > std::numeric_limits<uint32_t>::max()) {
            //TODO: report error
            return;
        }

        uint64_t msgNum;
        if (!msg.readVarUint(&msgNum)) {
            //TODO: report error
            return;
        }
        if (msgNum > std::numeric_limits<uint32_t>::max()) {
            //TODO: report error
            return;
        }

        bmcl::Bytes view(msg.current(), msg.sizeLeft());
        NumberedSub sub(compNum, msgNum);
        auto it = _numberedSubs.find(sub);
        if (it != _numberedSubs.end()) {
            bmcl::SharedBytes data = bmcl::SharedBytes::create(view);
            for (const caf::actor& actor : it->second) {
                send(actor, sub, data);
            }
        }

        _model->acceptTmMsg(ctx, compNum, msgNum, view);
    }
}
}
