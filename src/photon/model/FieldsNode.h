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
#include "decode/core/Hash.h"
#include "decode/core/HashMap.h"
#include "decode/parser/Containers.h"
#include "photon/model/Node.h"
#include "photon/model/NodeWithNamedChildren.h"

#include <bmcl/StringView.h>
#include <bmcl/Fwd.h>

#include <cstdint>

namespace photon {

class ValueNode;
class CoderState;
class ValueInfoCache;
class NodeViewUpdater;

class FieldsNode : public NodeWithNamedChildren {
public:
    using Pointer = Rc<FieldsNode>;
    using ConstPointer = Rc<const FieldsNode>;

    FieldsNode(decode::FieldVec::ConstRange, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FieldsNode();

    bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) override;
    bmcl::OptionPtr<ValueNode> valueNodeWithName(bmcl::StringView name);

    bool encodeFields(CoderState* ctx, bmcl::Buffer* dest) const;

    void collectUpdates(NodeViewUpdater* dest) override;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bool setValues(bmcl::ArrayView<Value> values);

public:
    decode::HashMap<bmcl::StringView, Rc<ValueNode>> _nameToNodeMap; //TODO: remove
    std::vector<Rc<ValueNode>> _nodes;
};

}
