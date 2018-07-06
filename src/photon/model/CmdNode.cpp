/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/CmdNode.h"
#include "decode/core/Try.h"
#include "decode/ast/Component.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/Type.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/ValueNode.h"

#include <bmcl/Buffer.h>

namespace photon {

CmdNode::CmdNode(const decode::Component* comp, const decode::Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
    : FieldsNode(func->type()->argumentsRange(), cache, parent)
    , _comp(comp)
    , _func(func)
    , _cache(cache)
    , _expandArgs(expandArgs)
{
}

CmdNode::~CmdNode()
{
}

bool CmdNode::encode(CoderState* ctx, bmcl::Buffer* dest) const
{
    dest->writeVarUint(_comp->number());
    auto it = std::find(_comp->cmdsBegin(), _comp->cmdsEnd(), _func.get());
    if (it == _comp->cmdsEnd()) {
        //TODO: report error
        return false;
    }

    dest->writeVarUint(std::distance(_comp->cmdsBegin(), it));
    return encodeFields(ctx, dest);
}

std::size_t CmdNode::numChildren() const
{
    if (_expandArgs)
        return _nodes.size();
    return 0;
}

bool CmdNode::canHaveChildren() const
{
    return _expandArgs;
}

Rc<CmdNode> CmdNode::clone(bmcl::OptionPtr<Node> parent) const
{
    return new CmdNode(_comp.get(), _func.get(), _cache.get(), parent);
}

bmcl::StringView CmdNode::typeName() const
{
    return _cache->nameForType(_func->type());
}

bmcl::StringView CmdNode::fieldName() const
{
    return _func->name();
}

bmcl::StringView CmdNode::shortDescription() const
{
    return _func->shortDescription();
}

ScriptNode::ScriptNode(bmcl::OptionPtr<Node> parent)
    : GenericContainerNode<CmdNode, Node>(parent)
{
}

ScriptNode::~ScriptNode()
{
}

Rc<ScriptNode> ScriptNode::withAllCmds(const decode::Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
{
    ScriptNode* self = new ScriptNode(parent);
    self->_fieldName = comp->moduleName();
    for (const decode::Function* f : comp->cmdsRange()) {
        self->_nodes.emplace_back(new CmdNode(comp, f, cache, self, expandArgs));
    }
    return self;
}

bmcl::StringView ScriptNode::fieldName() const
{
    return _fieldName;
}

void ScriptNode::addCmdNode(CmdNode* node)
{
    _nodes.emplace_back(node);
}

bool ScriptNode::encode(CoderState* ctx, bmcl::Buffer* dest) const
{
    for (const CmdNode* node : decode::RcVec<CmdNode>::ConstRange(_nodes)) {
        TRY(node->encode(ctx, dest));
    }
    return true;
}

void ScriptNode::removeCmd(const CmdNode* cmd)
{
    auto it = std::find(_nodes.begin(), _nodes.end(), cmd);
    if (it != _nodes.end()) {
        _nodes.erase(it);
    }
}

void ScriptNode::moveNode(std::size_t i1, std::size_t i2)
{
    std::swap(_nodes[i1], _nodes[i2]);
}

void ScriptNode::clear()
{
    _nodes.clear();
}

ScriptResultNode::ScriptResultNode(bmcl::OptionPtr<Node> parent)
    : GenericContainerNode<ValueNode, Node>(parent)
{
}

ScriptResultNode::~ScriptResultNode()
{
}

Rc<ScriptResultNode> ScriptResultNode::fromScriptNode(const ScriptNode* node, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    Rc<ScriptResultNode> resultNode = new ScriptResultNode(parent);
    std::size_t n = 0;
    for (const CmdNode* cmdNode : node->nodes()) {
        const decode::Function* func = cmdNode->function();
        auto rv = func->type()->returnValue();
        if (rv.isSome()) {
            Rc<ValueNode> valueNode = ValueNode::fromType(rv.unwrap(), cache, resultNode.get());
            valueNode->setFieldName(cache->arrayIndex(n));
            n++;
            resultNode->_nodes.emplace_back(valueNode);
        }
    }
    return resultNode;
}

bool ScriptResultNode::decode(CoderState* ctx, bmcl::MemReader* src)
{
    for (const Rc<ValueNode>& node : _nodes) {
        if (!node->decode(ctx, src)) {
            return false;
        }
    }
    return true;
}

bmcl::StringView ScriptResultNode::fieldName() const
{
    return "~";
}

template class GenericContainerNode<CmdNode, Node>;
template class GenericContainerNode<ValueNode, Node>;
}
