/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/FieldsNode.h"
#include "decode/ast/Component.h"
#include "photon/model/ValueNode.h"
#include "photon/model/Value.h"
#include "decode/core/StringBuilder.h"
#include "decode/core/Foreach.h"
#include "decode/ast/Field.h"

namespace photon {

FieldsNode::FieldsNode(decode::FieldVec::ConstRange params, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NodeWithNamedChildren(parent)
{
    for (const decode::Field* field : params) {
        if (field->rangeAttribute().isSome()) {
        }
        Rc<ValueNode> node = ValueNode::fromField(field, cache, this);
        _nameToNodeMap.emplace(field->name(), node);
        _nodes.emplace_back(node);
    }
}

FieldsNode::~FieldsNode()
{
}

bmcl::OptionPtr<ValueNode> FieldsNode::valueNodeWithName(bmcl::StringView name)
{
    auto it = _nameToNodeMap.find(name);
    if (it == _nameToNodeMap.end()) {
        return bmcl::None;
    }
    return it->second.get();
}

bmcl::OptionPtr<Node> FieldsNode::nodeWithName(bmcl::StringView name)
{
    auto it = _nameToNodeMap.find(name);
    if (it == _nameToNodeMap.end()) {
        return bmcl::None;
    }
    return it->second.get();
}

std::size_t FieldsNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> FieldsNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> FieldsNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bool FieldsNode::encodeFields(CoderState* ctx, bmcl::Buffer* dest) const
{
    for (std::size_t i = 0; i < _nodes.size(); i++) {
        const ValueNode* node = _nodes[i].get();
        if (!node->encode(ctx, dest)) {
            return false;
        }
    }
    return true;
}

bool FieldsNode::decodeFields(CoderState* ctx, bmcl::MemReader* src)
{
    for (std::size_t i = 0; i < _nodes.size(); i++) {
        ValueNode* node = _nodes[i].get();
        if (!node->decode(ctx, src)) {
            return false;
        }
    }
    return true;
}

void FieldsNode::collectUpdates(NodeViewUpdater* dest)
{
    return collectUpdatesGeneric(_nodes, dest);
}

bool FieldsNode::setValues(bmcl::ArrayView<Value> values)
{
    if (values.size() != _nodes.size()) {
        return false;
    }

    for (std::size_t i = 0; i < _nodes.size(); i++) {
        if (!_nodes[i]->setValue(values[i])) {
            return false;
        }
    }
    return true;
}

void FieldsNode::stringify(decode::StringBuilder* dest) const
{
    dest->append("{");
    decode::foreachList(_nodes, [dest](const Rc<ValueNode>& node) {
        dest->append(node->fieldName());
        dest->append(": ");
        node->stringify(dest);
    }, [dest](const Rc<ValueNode>& node) {
        dest->append(", ");
    });
    dest->append("}");
}

decode::RcVec<ValueNode>::ConstRange FieldsNode::valueNodes() const
{
    return _nodes;
}

decode::RcVec<ValueNode>::Range FieldsNode::valueNodes()
{
    return _nodes;
}
}
