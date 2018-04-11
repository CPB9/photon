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
#include "photon/model/TmMsgDecoder.h"
#include "photon/model/FieldsNode.h"
#include "photon/model/CoderState.h"
#include "photon/model/Node.h"
#include "photon/model/NodeViewUpdater.h"
#include "photon/model/ValueInfoCache.h"

#include <bmcl/MemReader.h>

#include <deque>

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

class StatusesNode : public NodeWithNamedChildren {
public:
    StatusesNode(bmcl::OptionPtr<Node> parent = bmcl::None)
        : NodeWithNamedChildren(parent)
    {
    }

    bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) override
    {
        for (const Rc<ComponentParamsNode>& child : _nodes) {
            if (child->fieldName() == name) {
                return child.get();
            }
        }
        return bmcl::None;
    }

    void collectUpdates(NodeViewUpdater* dest) override
    {
        collectUpdatesGeneric(_nodes, dest);
    }

    std::size_t numChildren() const override
    {
        return _nodes.size();
    }

    bmcl::Option<std::size_t> childIndex(const Node* node) const override
    {
        return childIndexGeneric(_nodes, node);
    }

    bmcl::OptionPtr<Node> childAt(std::size_t idx) override
    {
        return childAtGeneric(_nodes, idx);
    }

    bmcl::StringView fieldName() const override
    {
        return "~";
    }

    void addParamNode(ComponentParamsNode* node)
    {
        _nodes.emplace_back(node);
    }

private:
    std::vector<Rc<ComponentParamsNode>> _nodes;
};

class EventsNode : public Node {
public:
    EventsNode(bmcl::OptionPtr<Node> parent = bmcl::None)
        : Node(parent)
        , _lastUpdateSize(0)
    {
    }

    void collectUpdates(NodeViewUpdater* dest) override
    {
        if (_lastUpdateSize < _nodes.size()) {
            std::size_t delta = _nodes.size() - _lastUpdateSize;
            NodeViewVec vec;
            vec.reserve(delta);
            for (std::size_t i = _lastUpdateSize; i < _nodes.size(); i++) {
                EventNode* n = _nodes[i].get();
                vec.emplace_back(new NodeView(n, n->lastUpdateTime()));
            }
            dest->addExtendUpdate(std::move(vec), OnboardTime::now(), this); //FIXME: time
        }
        _lastUpdateSize = _nodes.size();
    }

    std::size_t numChildren() const override
    {
        return _nodes.size();
    }

    bmcl::Option<std::size_t> childIndex(const Node* node) const override
    {
        return childIndexGeneric(_nodes, node);
    }

    bmcl::OptionPtr<Node> childAt(std::size_t idx) override
    {
        return childAtGeneric(_nodes, idx);
    }

    bmcl::StringView fieldName() const override
    {
        return "~";
    }

    void addEvent(EventNode* node)
    {
        node->setParent(this);
        _nodes.emplace_back(node);
    }

    void addEvent(Rc<EventNode>&& node)
    {
        node->setParent(this);
        _nodes.push_back(std::move(node));
    }

private:
    std::size_t _lastUpdateSize;
    std::vector<Rc<EventNode>> _nodes;
};

TmModel::TmModel(const decode::Device* dev, const ValueInfoCache* cache)
    : _device(dev)
    , _statuses(new StatusesNode)
    , _events(new EventsNode)
{
    for (const decode::Ast* ast : dev->modules()) {
        if (ast->component().isNone()) {
            continue;
        }

        const decode::Component* comp = ast->component().unwrap();

        if (!comp->hasParams()) {
            continue;
        }

        Rc<ComponentParamsNode> node = new ComponentParamsNode(comp, cache, _statuses.get());
        _statuses->addParamNode(node.get());

        std::size_t compNum = comp->number();
        for (const decode::StatusMsg* msg : comp->statusesRange()) {
            std::size_t msgNum = msg->number();
            uint64_t num = (uint64_t(compNum) << 32) | uint64_t(msgNum);
            _decoders.emplace(num, StatusMsgDecoder(msg, node.get()));
        }
        for (const decode::EventMsg* msg : comp->eventsRange()) {
            std::size_t msgNum = msg->number();
            uint64_t num = (uint64_t(compNum) << 32) | uint64_t(msgNum);
            _decoders.emplace(num, EventMsgDecoder(msg, cache));
        }
    }
}

bool TmModel::acceptTmMsg(CoderState* ctx, uint32_t compNum, uint32_t msgNum, bmcl::MemReader* src)
{
    auto it = _decoders.find((uint64_t(compNum) << 32) | uint64_t(msgNum));
    if (it == _decoders.end()) {
        ctx->setError("Invalid component id or tm msg id");
        return false;
    }

    if (it->second.isFirst()) {
        if (!it->second.unwrapFirst().decode(ctx, src)) {
            return false;
        }
    } else {
        bmcl::Option<Rc<EventNode>> eventNode = it->second.unwrapSecond().decode(ctx, src);
        if (eventNode.isNone()) {
            return false;
        }
        _events->addEvent(std::move(eventNode.unwrap()));
    }
    return true;
}

TmModel::~TmModel()
{
}

Node* TmModel::statusesNode()
{
    return _statuses.get();
}

Node* TmModel::eventsNode()
{
    return _events.get();
}
}
