/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/core/Rc.h"
#include "photon/model/FieldsNode.h"
#include "decode/core/HashMap.h"

#include <bmcl/Fwd.h>

#include <vector>

namespace decode {
class StatusMsg;
class EventMsg;
}

namespace bmcl { class MemReader; }

namespace photon {

class FieldsNode;
class Statuses;
class ValueNode;
class DecoderAction;
class CoderState;
class ValueInfoCache;
class EventNode;
class Value;

struct ChainElement {
    ChainElement(std::size_t index, DecoderAction* action, ValueNode* node);
    ChainElement(const ChainElement& other);
    ~ChainElement();

    std::size_t nodeIndex;
    Rc<DecoderAction> action;
    Rc<ValueNode> node;
};

class StatusMsgDecoder {
public:

    StatusMsgDecoder(const decode::StatusMsg* msg, FieldsNode* node);
    ~StatusMsgDecoder();

    bool decode(CoderState* ctx, bmcl::MemReader* src);

private:
    std::vector<ChainElement> _chain;
};

class EventNode : public FieldsNode {
public:
    EventNode(const decode::EventMsg* msg, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent = bmcl::None);
    ~EventNode();

    bool decode(CoderState* ctx, bmcl::MemReader* src);
    bmcl::StringView fieldName() const override;
    Value value() const override;
    ValueKind valueKind() const override;

private:
    void updateValue();

    Rc<const decode::EventMsg> _msg;
    bmcl::StringView _name;
    std::string _value;
};

class EventMsgDecoder {
public:

    EventMsgDecoder(const decode::EventMsg* msg, const ValueInfoCache* cache);
    ~EventMsgDecoder();

    bmcl::Option<Rc<EventNode>> decode(CoderState* ctx, bmcl::MemReader* src);

private:
    Rc<const decode::EventMsg> _msg;
    Rc<const ValueInfoCache> _cache;
};
}
