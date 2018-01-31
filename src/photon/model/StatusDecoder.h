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
class CoderState;

struct ChainElement {
    ChainElement(std::size_t index, DecoderAction* action, ValueNode* node);
    ChainElement(const ChainElement& other);
    ~ChainElement();

    std::size_t nodeIndex;
    Rc<DecoderAction> action;
    Rc<ValueNode> node;
};

class TmDecoder : public RefCountable {
public:
    virtual bool decode(CoderState* ctx, bmcl::MemReader* src) = 0;
};

class StatusMsgDecoder : public TmDecoder {
public:

    StatusMsgDecoder(const decode::StatusMsg* msg, FieldsNode* node);
    ~StatusMsgDecoder();

    bool decode(CoderState* ctx, bmcl::MemReader* src) override;

private:
    std::vector<ChainElement> _chain;
};
}
