/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/model/Node.h"
#include "decode/core/HashMap.h"
#include "photon/model/TmMsgDecoder.h"

#include <bmcl/Either.h>
#include <bmcl/Fwd.h>

#include <vector>

namespace decode {
class Device;
}

namespace photon {

class CoderState;
class ValueInfoCache;
class FieldsNode;
class StatusMsgDecoder;
class NodeViewUpdater;
template <typename T>
class NumericValueNode;
class ComponentVarsNode;
class StatusesNode;
class EventsNode;
class TmStatsNode;

class TmModel : public RefCountable {
public:
    struct MsgState {
        MsgState(const decode::StatusMsg* msg, FieldsNode* fieldsNode, NumericValueNode<uint64_t>* statsNode)
            : decoder(bmcl::InPlaceFirst, msg, fieldsNode)
            , statNode(statsNode)
        {
        }

        MsgState(const decode::EventMsg* msg, const ValueInfoCache* cache, NumericValueNode<uint64_t>* statsNode)
            : decoder(bmcl::InPlaceSecond, msg, cache)
            , statNode(statsNode)
        {
        }

        bmcl::Either<StatusMsgDecoder, EventMsgDecoder> decoder;
        Rc<NumericValueNode<uint64_t>> statNode;
    };

    using Pointer = Rc<TmModel>;
    using ConstPointer = Rc<const TmModel>;

    TmModel(const decode::Device* dev, const ValueInfoCache* cache);
    ~TmModel();

    bool acceptTmMsg(CoderState* ctx, uint32_t compNum, uint32_t msgNum, bmcl::MemReader* payload);

    Node* statusesNode();
    Node* eventsNode();
    Node* statisticsNode();

private:
    decode::HashMap<uint64_t, MsgState> _decoders;
    Rc<const decode::Device> _device;
    Rc<StatusesNode> _statuses;
    Rc<EventsNode> _events;
    Rc<TmStatsNode> _statistics;
};
}
