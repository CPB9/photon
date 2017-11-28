/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/StatusDecoder.h"
#include "decode/core/Try.h"
#include "decode/ast/Component.h"
#include "decode/ast/Type.h"
#include "photon/model/FieldsNode.h"
#include "decode/ast/Field.h"
#include "photon/model/ValueNode.h"

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
    virtual bool execute(ValueNode* node, bmcl::MemReader* src) = 0;

    void setNext(DecoderAction* next)
    {
        _next.reset(next);
    }

protected:
    Rc<DecoderAction> _next;
};

class DecodeNodeAction : public DecoderAction {
public:
    bool execute(ValueNode* node, bmcl::MemReader* src) override
    {
        return node->decode(src);
    }
};

class DescentAction : public DecoderAction {
public:
    DescentAction(std::size_t fieldIndex)
        : _fieldIndex(fieldIndex)
    {
    }

    bool execute(ValueNode* node, bmcl::MemReader* src) override
    {
        (void)src;
        assert(node->type()->isStruct());
        ContainerValueNode* cnode = static_cast<ContainerValueNode*>(node);
        ValueNode* child = cnode->nodeAt(_fieldIndex);
        return _next->execute(child, src);
    }

private:
    std::size_t _fieldIndex;
};

class DecodeDynArrayPartsAction : public DecoderAction {
public:
    bool execute(ValueNode* node, bmcl::MemReader* src) override
    {
        uint64_t dynArraySize;
        if (!src->readVarUint(&dynArraySize)) {
            //TODO: report error
            return false;
        }
        DynArrayValueNode* cnode = static_cast<DynArrayValueNode*>(node);
        if (dynArraySize > cnode->maxSize()) {
            //TODO: report error
            return false;
        }
        //TODO: add size check
        cnode->resizeDynArray(dynArraySize);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < dynArraySize; i++) {
            TRY(_next->execute(values[i].get(), src));
        }
        return true;
    }
};

class DecodeArrayPartsAction : public DecoderAction {
public:
    bool execute(ValueNode* node, bmcl::MemReader* src) override
    {
        //FIXME: implement range check
        ArrayValueNode* cnode = static_cast<ArrayValueNode*>(node);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < values.size(); i++) {
            TRY(_next->execute(values[i].get(), src));
        }
        return true;
    }
};

static Rc<DecoderAction> createMsgDecoder(const decode::StatusRegexp* part, const decode::Type* lastType)
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
        if (!firstAction) {
            firstAction = newAction;
        }
        if (previousAction) {
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
    for (const decode::StatusRegexp* part : msg->partsRange()) {
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

bool StatusMsgDecoder::decode(bmcl::MemReader* src)
{
    for (const ChainElement& elem : _chain) {
        if (!elem.action->execute(elem.node.get(), src)) {
            return false;
        }
    }
    return true;
}

StatusDecoder::~StatusDecoder()
{
}

bool StatusDecoder::decode(uint32_t msgId, bmcl::Bytes payload)
{
    auto it = _decoders.find(msgId);
    if (it == _decoders.end()) {
        //TODO: report error
        return false;
    }
    bmcl::MemReader src(payload);
    if (!it->second.decode(&src)) {
        //TODO: report error
        return false;
    }
    return true;
}
}
