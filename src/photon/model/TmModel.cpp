/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/TmModel.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Project.h"
#include "decode/ast/Component.h"
#include "decode/ast/Ast.h"
#include "photon/model/StatusDecoder.h"
#include "photon/model/FieldsNode.h"

#include <bmcl/MemReader.h>

namespace photon {

class ComponentParamsNode : public FieldsNode {
public:
    ComponentParamsNode(const decode::Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
        : FieldsNode(comp->paramsRange(), cache, parent)
        , _comp(comp)
    {
    }

    ~ComponentParamsNode()
    {
    }

    bmcl::StringView fieldName() const override
    {
        return _comp->name();
    }

    bmcl::StringView shortDescription() const override
    {
        return _comp->moduleInfo()->shortDescription();
    }

private:
    Rc<const decode::Component> _comp;
};

TmModel::TmModel(const decode::Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NodeWithNamedChildren(parent)
    , _device(dev)
{
    for (const decode::Ast* ast : dev->modules()) {
        if (ast->component().isNone()) {
            continue;
        }

        const decode::Component* it = ast->component().unwrap();

        if (!it->hasParams()) {
            continue;
        }

        Rc<ComponentParamsNode> node = new ComponentParamsNode(it, cache, this);
        _nodes.emplace_back(node);

        if (!it->hasStatuses()) {
            continue;
        }
        std::size_t compNum = it->number();
        for (const decode::StatusMsg* msg : it->statusesRange()) {
            std::size_t msgNum = msg->number();
            uint64_t num = (uint64_t(compNum) << 32) | uint64_t(msgNum);
            _decoders.emplace(num, new StatusMsgDecoder(msg, node.get()));
        }
    }
}

TmModel::~TmModel()
{
}

bmcl::OptionPtr<Node> TmModel::nodeWithName(bmcl::StringView name)
{
    auto it = std::find_if(_nodes.begin(), _nodes.end(), [name](const Rc<ComponentParamsNode>& node) {
        return node->fieldName() == name;
    });
    if (it == _nodes.end()) {
        return bmcl::None;
    }
    return it->get();
}

void TmModel::collectUpdates(NodeViewUpdater* dest)
{
    collectUpdatesGeneric(_nodes, dest);
}

void  TmModel::acceptTmMsg(const DecoderCtx& ctx, uint32_t compNum, uint32_t msgNum, bmcl::Bytes payload)
{
    auto it = _decoders.find((uint64_t(compNum) << 32) | uint64_t(msgNum));
    if (it == _decoders.end()) {
        //TODO: report error
        return;
    }

    bmcl::MemReader src(payload);
    if (!it->second->decode(ctx, &src)) {
        //TODO: report error
        return;
    }
}

std::size_t TmModel::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> TmModel::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> TmModel::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView TmModel::fieldName() const
{
    return "~";
}
}
