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
#include "decode/ast/Type.h"
#include "photon/model/TmMsgDecoder.h"
#include "photon/model/FieldsNode.h"
#include "photon/model/CoderState.h"
#include "photon/model/Node.h"
#include "photon/model/NodeViewUpdater.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/ValueNode.h"

#include <bmcl/MemReader.h>

#include <deque>

namespace photon {

class TmStatsNode : public Node {
public:
    using NodeType = NumericValueNode<uint64_t>;

    explicit TmStatsNode(bmcl::OptionPtr<Node> parent = bmcl::None)
        : Node(parent)
        , _totalMsgsRecieved(OnboardTime::now(), 0)
        , _lastUpdate(OnboardTime::now())
    {
    }

    ~TmStatsNode()
    {
    }

    void incTotal(OnboardTime now)
    {
        _totalMsgsRecieved.setValue(now, _totalMsgsRecieved.value() + 1);
    }

    void setLastUpdateTime(OnboardTime time)
    {
        _lastUpdate = time;
    }

    void addNode(NodeType* node)
    {
        _nodes.emplace_back(node);
    }

    void collectUpdates(NodeViewUpdater* dest) override
    {
        if (_totalMsgsRecieved.hasChanged()) {
            dest->addValueUpdate(Value::makeUnsigned(_totalMsgsRecieved.value()),
                                 _totalMsgsRecieved.lastOnboardUpdateTime(),
                                 this);
            _totalMsgsRecieved.updateState();
        }
        collectUpdatesGeneric(_nodes, dest);
    }

    std::size_t numChildren() const override
    {
        return _nodes.size();
    }

    bmcl::OptionPtr<Node> childAt(std::size_t idx) override
    {
        return childAtGeneric(_nodes, idx);
    }

    bmcl::Option<std::size_t> childIndex(const Node* node) const override
    {
        return childIndexGeneric(_nodes, node);
    }

    bmcl::StringView fieldName() const override
    {
        return "messages";
    }

    Value value() const override
    {
        return Value::makeUnsigned(_totalMsgsRecieved.value());
    }

    ValueKind valueKind() const override
    {
        return ValueKind::Unsigned;
    }

private:
    std::vector<Rc<NodeType>> _nodes;
    ValuePair<uint64_t> _totalMsgsRecieved;
    OnboardTime _lastUpdate;
};

class ComponentVarsNode : public FieldsNode {
public:
    ComponentVarsNode(const decode::Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
        : FieldsNode(comp->varsRange(), cache, parent)
        , _comp(comp)
    {
    }

    ~ComponentVarsNode()
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
    StatusesNode(const decode::Device* dev, bmcl::OptionPtr<Node> parent = bmcl::None)
        : NodeWithNamedChildren(parent)
        , _dev(dev)
    {
    }

    bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) override
    {
        for (const Rc<ComponentVarsNode>& child : _nodes) {
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
        return _dev->name();
    }

    void addVarsNode(ComponentVarsNode* node)
    {
        _nodes.emplace_back(node);
    }

private:
    std::vector<Rc<ComponentVarsNode>> _nodes;
    Rc<const decode::Device> _dev;
};

class EventsNode : public Node {
public:
    explicit EventsNode(bmcl::OptionPtr<Node> parent = bmcl::None)
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
    , _statuses(new StatusesNode(dev))
    , _events(new EventsNode)
    , _statistics(new TmStatsNode)
{
    for (const decode::Ast* ast : dev->modules()) {
        if (ast->component().isNone()) {
            continue;
        }

        const decode::Component* comp = ast->component().unwrap();

        if (!comp->hasVars()) {
            continue;
        }

        Rc<ComponentVarsNode> node = new ComponentVarsNode(comp, cache, _statuses.get());
        _statuses->addVarsNode(node.get());

        Rc<decode::BuiltinType> u64Type = new decode::BuiltinType(decode::BuiltinTypeKind::U64);

        std::size_t compNum = comp->number();
        for (const decode::StatusMsg* msg : comp->statusesRange()) {
            std::size_t msgNum = msg->number();
            uint64_t num = (uint64_t(compNum) << 32) | uint64_t(msgNum);

            Rc<NumericValueNode<uint64_t>> statsNode = new NumericValueNode<uint64_t>(u64Type.get(), cache, _statistics.get());
            statsNode->setFieldName(cache->nameForTmMsg(msg));
            _statistics->addNode(statsNode.get());
            _decoders.emplace(std::piecewise_construct,
                              std::forward_as_tuple(num),
                              std::forward_as_tuple(msg, node.get(), statsNode.get()));
        }
        for (const decode::EventMsg* msg : comp->eventsRange()) {
            std::size_t msgNum = msg->number();
            uint64_t num = (uint64_t(compNum) << 32) | uint64_t(msgNum);
            Rc<NumericValueNode<uint64_t>> statsNode = new NumericValueNode<uint64_t>(u64Type.get(), cache, _statistics.get());
            statsNode->setFieldName(cache->nameForTmMsg(msg));
            _statistics->addNode(statsNode.get());
            _decoders.emplace(std::piecewise_construct,
                              std::forward_as_tuple(num),
                              std::forward_as_tuple(msg, cache, statsNode.get()));
        }
    }
}

bool TmModel::acceptTmMsg(CoderState* ctx, uint32_t compNum, uint32_t msgNum, bmcl::MemReader* src)
{
    auto it = _decoders.find((uint64_t(compNum) << 32) | uint64_t(msgNum));
    if (it == _decoders.end()) {
        ctx->setError("Invalid component id or tm msg id: " + std::to_string(compNum) + " " + std::to_string(msgNum));
        return false;
    }

    MsgState& state = it->second;

    if (state.decoder.isFirst()) {
        if (!state.decoder.unwrapFirst().decode(ctx, src)) {
            return false;
        }
    } else {
        bmcl::Option<Rc<EventNode>> eventNode = state.decoder.unwrapSecond().decode(ctx, src);
        if (eventNode.isNone()) {
            return false;
        }
        _events->addEvent(std::move(eventNode.unwrap()));
    }

    auto now = OnboardTime::now();
    state.statNode->incRawValue(now);
    _statistics->incTotal(now);
    _statistics->setLastUpdateTime(now);
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

Node* TmModel::statisticsNode()
{
    return _statistics.get();
}
}
