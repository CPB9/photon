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
#include "decode/core/HashMap.h"

#include <bmcl/Fwd.h>

#include <vector>

namespace decode {
class StatusMsg;
}

namespace bmcl { class MemReader; }

namespace photon {

class FieldsNode;
class Statuses;
class ValueNode;
class DecoderAction;
class DecoderCtx;

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

    bool decode(const DecoderCtx& ctx, bmcl::MemReader* src);

private:
    std::vector<ChainElement> _chain;
};

class StatusDecoder : public RefCountable {
public:
    using Pointer = Rc<StatusDecoder>;
    using ConstPointer = Rc<const StatusDecoder>;

    template <typename R>
    StatusDecoder(R statusRange, FieldsNode* node)
    {
        for (const auto& it : statusRange) {
            _decoders.emplace(std::piecewise_construct,
                              std::forward_as_tuple(it->number()),
                              std::forward_as_tuple(it, node));
        }
    }

    ~StatusDecoder();

    bool decode(const DecoderCtx& ctx, uint32_t msgId, bmcl::Bytes payload);

private:
    decode::HashMap<uint32_t, StatusMsgDecoder> _decoders;
};
}
