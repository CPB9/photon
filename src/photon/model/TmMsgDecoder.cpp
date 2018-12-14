/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/TmMsgDecoder.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/core/StringBuilder.h"
#include "decode/ast/Component.h"
#include "decode/ast/Type.h"
#include "photon/model/FieldsNode.h"
#include "decode/ast/Field.h"
#include "photon/model/ValueNode.h"
#include "photon/model/CoderState.h"
#include "photon/model/Value.h"

#include <bmcl/Bytes.h>
#include <bmcl/MemReader.h>

namespace photon {

ChainElement::ChainElement(std::size_t index, DecoderAction* action, ValueNode* node)
    : nodeIndex(index)
    , action(action)
    , node(node)
{
}

ChainElement::ChainElement(const ChainElement& other)
    : nodeIndex(other.nodeIndex)
    , action(other.action)
    , node(other.node)
{
}

ChainElement::~ChainElement()
{
}

class DecoderAction : public RefCountable {
public:
    virtual bool execute(CoderState* ctx, ValueNode* node, bmcl::MemReader* src) = 0;

    void setNext(DecoderAction* next)
    {
        _next.reset(next);
    }

protected:
    Rc<DecoderAction> _next;
};

class DecodeNodeAction : public DecoderAction {
public:
    bool execute(CoderState* ctx, ValueNode* node, bmcl::MemReader* src) override
    {
        return node->decode(ctx, src);
    }
};

class DescentAction : public DecoderAction {
public:
    explicit DescentAction(std::size_t fieldIndex)
        : _fieldIndex(fieldIndex)
    {
    }

    bool execute(CoderState* ctx, ValueNode* node, bmcl::MemReader* src) override
    {
        (void)src;
        assert(node->type()->isStruct());
        ContainerValueNode* cnode = static_cast<ContainerValueNode*>(node);
        ValueNode* child = cnode->nodeAt(_fieldIndex);
        return _next->execute(ctx, child, src);
    }

private:
    std::size_t _fieldIndex;
};

class DecodeDynArrayPartsAction : public DecoderAction {
public:
    bool execute(CoderState* ctx, ValueNode* node, bmcl::MemReader* src) override
    {
        uint64_t dynArraySize;
        if (!src->readVarUint(&dynArraySize)) {
            ctx->setError("failed to read dynArray size");
            return false;
        }
        DynArrayValueNode* cnode = static_cast<DynArrayValueNode*>(node);
        if (dynArraySize > cnode->maxSize()) {
            ctx->setError("invalid dynArray size");
            return false;
        }
        //TODO: add size check
        cnode->resizeDynArray(ctx->dataTimeOfOrigin(), dynArraySize);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < dynArraySize; i++) {
            TRY(_next->execute(ctx, values[i].get(), src));
        }
        return true;
    }
};

class DecodeArrayPartsAction : public DecoderAction {
public:
    bool execute(CoderState* ctx, ValueNode* node, bmcl::MemReader* src) override
    {
        //FIXME: implement range check
        ArrayValueNode* cnode = static_cast<ArrayValueNode*>(node);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < values.size(); i++) {
            TRY(_next->execute(ctx, values[i].get(), src));
        }
        return true;
    }
};

static Rc<DecoderAction> createMsgDecoder(const decode::VarRegexp* part, const decode::Type* lastType)
{
    assert(part->hasAccessors());
    if (part->accessorsRange().size() == 1) {
        return new DecodeNodeAction;
    }

    auto it = part->accessorsBegin() + 1;
    auto end = part->accessorsEnd();

    Rc<DecoderAction> firstAction;
    Rc<DecoderAction> previousAction;

    auto updateAction = [&](DecoderAction* newAction) {
        if (firstAction.isNull()) {
            firstAction = newAction;
        }
        if (!previousAction.isNull()) {
            previousAction->setNext(newAction);
        }
        previousAction = newAction;
    };

    while (it != end) {
        const decode::Accessor* acc = *it;
        if (acc->accessorKind() == decode::AccessorKind::Field) {
            auto facc = acc->asFieldAccessor();
            assert(lastType->isStruct());
            bmcl::Option<std::size_t> index = lastType->asStruct()->indexOfField(facc->field());
            assert(index.isSome());
            lastType = facc->field()->type();
            DescentAction* newAction = new DescentAction(index.unwrap());
            updateAction(newAction);
        } else if (acc->accessorKind() == decode::AccessorKind::Subscript) {
            auto sacc = acc->asSubscriptAccessor();
            const decode::Type* type = sacc->type();
            if (type->isArray()) {
                updateAction(new DecodeArrayPartsAction);
                lastType = type->asArray()->elementType();
            } else if (type->isDynArray()) {
                updateAction(new DecodeDynArrayPartsAction);
                lastType = type->asDynArray()->elementType();
            } else {
                assert(false);
            }
        } else {
            assert(false);
        }
        it++;
    }

    previousAction->setNext(new DecodeNodeAction);
    return firstAction;
}

StatusMsgDecoder::StatusMsgDecoder(const decode::StatusMsg* msg, FieldsNode* node)
{
    for (const decode::VarRegexp* part : msg->partsRange()) {
        if (!part->hasAccessors()) {
            continue;
        }
        assert(part->accessorsBegin()->accessorKind() == decode::AccessorKind::Field);
        auto facc = part->accessorsBegin()->asFieldAccessor();
        auto op = node->valueNodeWithName(facc->field()->name());
        assert(op.isSome());
        assert(node->hasParent());
        auto i = node->childIndex(op.unwrap());
        std::size_t index = i.unwrap();
        Rc<DecoderAction> decoder = createMsgDecoder(part, facc->field()->type());
        _chain.emplace_back(index, decoder.get(), op.unwrap());
    }
}

StatusMsgDecoder::~StatusMsgDecoder()
{
}

bool StatusMsgDecoder::decode(CoderState* ctx, bmcl::MemReader* src)
{
    for (const ChainElement& elem : _chain) {
        if (!elem.action->execute(ctx, elem.node.get(), src)) {
            return false;
        }
    }
    return true;
}

EventMsgDecoder::EventMsgDecoder(const decode::EventMsg* msg, const ValueInfoCache* cache)
    : _msg(msg)
    , _cache(cache)
{
}

EventMsgDecoder::~EventMsgDecoder()
{
}

bmcl::Option<Rc<EventNode>> EventMsgDecoder::decode(CoderState* ctx, bmcl::MemReader* src)
{
    Rc<EventNode> node = new EventNode(_msg.get(), _cache.get());
    if (!node->decode(ctx, src)) {
        return bmcl::None;
    }
    return std::move(node);
}

EventNode::EventNode(const decode::EventMsg* msg, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : FieldsNode(msg->partsRange(), cache, parent)
    , _msg(msg)
{
    _name = cache->nameForTmMsg(msg);
    _value = "???";
}

EventNode::~EventNode()
{
}

bool EventNode::decode(CoderState* ctx, bmcl::MemReader* src)
{
    bool rv = decodeFields(ctx, src);
    updateValue();
    return rv;
}

bmcl::StringView EventNode::fieldName() const
{
    return _name;
}

Value EventNode::value() const
{
    return Value::makeStringView(_value);
}

ValueKind EventNode::valueKind() const
{
    return ValueKind::StringView;
}

void EventNode::updateValue()
{
    if (_nodes.empty()) {
        _value.clear();
        return;
    }
    decode::StringBuilder builder;
    stringify(&builder);
    _value = builder.toStdString();
}
}
