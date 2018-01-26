/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/ValueNode.h"
#include "photon/model/Value.h"
#include "decode/ast/Field.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/core/RangeAttr.h"
#include "decode/ast/Type.h"
#include "decode/ast/Component.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/NodeViewUpdate.h"
#include "photon/model/NodeView.h"
#include "photon/model/NodeViewUpdater.h"
#include "photon/model/DecoderCtx.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>

namespace bmcl {
#ifdef BMCL_LITTLE_ENDIAN
template <>
inline float htole(float value)
{
    return value;
}

template <>
inline float letoh(float value)
{
    return value;
}

template <>
inline double htole(double value)
{
    return value;
}

template <>
inline double letoh(double value)
{
    return value;
}
#else
# error TODO: implement for big endian
#endif
template <>
inline char htole(char value)
{
    return value;
}

template <>
inline char letoh(char value)
{
    return value;
}
}

namespace photon {

template <typename T>
inline bool updateOptionalValuePair(bmcl::Option<ValuePair<T>>* value, OnboardTime time, T newValue)
{
    if (value->isSome()) {
        value->unwrap().setValue(time, newValue);
        return true;
    }
    value->emplace(time, newValue);
    return true;
}

ValueNode::ValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : Node(parent)
    , _cache(cache)
{
}

ValueNode::~ValueNode()
{
}

bmcl::StringView ValueNode::typeName() const
{
    return _cache->nameForType(type());
}

bmcl::StringView ValueNode::fieldName() const
{
    return _fieldName;
}

bmcl::StringView ValueNode::shortDescription() const
{
    return _shortDesc;
}

void ValueNode::setFieldName(bmcl::StringView name)
{
    _fieldName = name;
}

void ValueNode::setShortDesc(bmcl::StringView desc)
{
    _shortDesc = desc;
}

static Rc<BuiltinValueNode> builtinNodeFromType(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    switch (type->builtinTypeKind()) {
    case decode::BuiltinTypeKind::USize:
        //TODO: check target ptr size
        return new NumericValueNode<std::uint64_t>(type, cache, parent);
    case decode::BuiltinTypeKind::ISize:
        //TODO: check target ptr size
        return new NumericValueNode<std::int64_t>(type, cache, parent);
    case decode::BuiltinTypeKind::Varint:
        return new VarintValueNode(type, cache, parent);
    case decode::BuiltinTypeKind::Varuint:
        return new VaruintValueNode(type, cache, parent);
    case decode::BuiltinTypeKind::U8:
        return new NumericValueNode<std::uint8_t>(type, cache, parent);
    case decode::BuiltinTypeKind::I8:
        return new NumericValueNode<std::int8_t>(type, cache, parent);
    case decode::BuiltinTypeKind::U16:
        return new NumericValueNode<std::uint16_t>(type, cache, parent);
    case decode::BuiltinTypeKind::I16:
        return new NumericValueNode<std::int16_t>(type, cache, parent);
    case decode::BuiltinTypeKind::U32:
        return new NumericValueNode<std::uint32_t>(type, cache, parent);
    case decode::BuiltinTypeKind::I32:
        return new NumericValueNode<std::int32_t>(type, cache, parent);
    case decode::BuiltinTypeKind::U64:
        return new NumericValueNode<std::uint64_t>(type, cache, parent);
    case decode::BuiltinTypeKind::I64:
        return new NumericValueNode<std::int64_t>(type, cache, parent);
    case decode::BuiltinTypeKind::F32:
        return new NumericValueNode<float>(type, cache, parent);
    case decode::BuiltinTypeKind::F64:
        return new NumericValueNode<double>(type, cache, parent);
    case decode::BuiltinTypeKind::Bool:
        //TODO: make bool value
        return new NumericValueNode<std::uint8_t>(type, cache, parent);
    case decode::BuiltinTypeKind::Void:
        assert(false);
        return nullptr;
    case decode::BuiltinTypeKind::Char:
        return new NumericValueNode<char>(type, cache, parent);
    }
    assert(false);
    return nullptr;
}

static Rc<ValueNode> createNodefromType(const decode::Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    switch (type->typeKind()) {
    case decode::TypeKind::Builtin:
        return builtinNodeFromType(type->asBuiltin(), cache, parent);
    case decode::TypeKind::Reference:
        return new ReferenceValueNode(type->asReference(), cache, parent);
    case decode::TypeKind::Array:
        return new ArrayValueNode(type->asArray(), cache, parent);
    case decode::TypeKind::DynArray: {
        const decode::DynArrayType* dynArray = type->asDynArray();
        if (dynArray->elementType()->isBuiltin()) {
            if (dynArray->elementType()->asBuiltin()->builtinTypeKind() == decode::BuiltinTypeKind::Char) {
                return new StringValueNode(dynArray, cache, parent);
            }
        }
        return new DynArrayValueNode(dynArray, cache, parent);
    }
    case decode::TypeKind::Function:
        return new FunctionValueNode(type->asFunction(), cache, parent);
    case decode::TypeKind::Enum:
        return new EnumValueNode(type->asEnum(), cache, parent);
    case decode::TypeKind::Struct:
        return new StructValueNode(type->asStruct(), cache, parent);
    case decode::TypeKind::Variant:
        return new VariantValueNode(type->asVariant(), cache, parent);
    case decode::TypeKind::Imported:
        return createNodefromType(type->asImported()->link(), cache, parent);
    case decode::TypeKind::Alias:
        return createNodefromType(type->asAlias()->alias(), cache, parent);
    case decode::TypeKind::Generic:
        assert(false);
        return nullptr;
    case decode::TypeKind::GenericInstantiation:
        return createNodefromType(type->asGenericInstantiation()->instantiatedType(), cache, parent);
    case decode::TypeKind::GenericParameter:
        assert(false);
        return nullptr;
    }
    assert(false);
    return nullptr;
}

Rc<ValueNode> ValueNode::fromType(const decode::Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    assert(cache);
    return createNodefromType(type, cache, parent);
}

Rc<ValueNode> ValueNode::fromField(const decode::Field* field, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    Rc<ValueNode> node = ValueNode::fromType(field->type(), cache, parent);
    node->setShortDesc(field->shortDescription());
    node->setFieldName(field->name());
    if (field->type()->isBuiltin()) {
        static_cast<BuiltinValueNode*>(node.get())->setRangeAttribute(field->rangeAttribute().data());
    }
    return node;
}

ContainerValueNode::ContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ValueNode(cache, parent)
{
}

ContainerValueNode::~ContainerValueNode()
{
    for (const Rc<ValueNode>& v : _values) {
        v->setParent(nullptr);
    }
}

bool ContainerValueNode::isContainerValue() const
{
    return true;
}

bool ContainerValueNode::isInitialized() const
{
    for (const Rc<ValueNode>& v : _values) {
        if (!v->isInitialized()) {
            return false;
        }
    }
    return true;
}

bool ContainerValueNode::encode(bmcl::MemWriter* dest) const
{
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->encode(dest));
    }
    return true;
}

bool ContainerValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    for (std::size_t i = 0; i < _values.size(); i++) {
        TRY(_values[i]->decode(ctx, src));
    }
    return true;
}

const std::vector<Rc<ValueNode>> ContainerValueNode::values()
{
    return _values;
}

std::size_t ContainerValueNode::numChildren() const
{
    return _values.size();
}

bmcl::Option<std::size_t> ContainerValueNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_values, node);
}

bmcl::OptionPtr<Node> ContainerValueNode::childAt(std::size_t idx)
{
    return childAtGeneric(_values, idx);
}

ValueNode* ContainerValueNode::nodeAt(std::size_t index)
{
    return _values[index].get();
}

const ValueNode* ContainerValueNode::nodeAt(std::size_t index) const
{
    return _values[index].get();
}

ArrayValueNode::ArrayValueNode(const decode::ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _changedSinceUpdate(false)
{
    std::size_t count = type->elementCount();
    _values.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        Rc<ValueNode> node = ValueNode::fromType(type->elementType(), _cache.get(), this);
        node->setFieldName(_cache->arrayIndex(i));
        _values.push_back(node);
    }
}

ArrayValueNode::~ArrayValueNode()
{
}

void ArrayValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (const Rc<ValueNode>& node : _values) {
        node->collectUpdates(dest);
    }
    _changedSinceUpdate = false;
}

bool ArrayValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    _changedSinceUpdate = true;
    return ContainerValueNode::decode(ctx, src);
}

const decode::Type* ArrayValueNode::type() const
{
    return _type.get();
}

DynArrayValueNode::DynArrayValueNode(const decode::DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _lastResizeTime(OnboardTime::now())
    , _minSizeSinceUpdate(0)
    , _lastUpdateSize(0)
{
}

DynArrayValueNode::~DynArrayValueNode()
{
}

void DynArrayValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (std::size_t i = 0; i < _minSizeSinceUpdate; i++) {
        _values[i]->collectUpdates(dest);
    }
    if (_minSizeSinceUpdate < _lastUpdateSize) {
        //shrink
        dest->addShrinkUpdate(_minSizeSinceUpdate, _lastResizeTime, this);
    }
    if (_values.size() > _minSizeSinceUpdate) {
        //extend
        NodeViewVec vec;
        vec.reserve(_values.size() - _minSizeSinceUpdate);
        for (std::size_t i = _minSizeSinceUpdate; i < _values.size(); i++) {
            vec.emplace_back(new NodeView(_values[i].get(), _lastResizeTime));
        }
        dest->addExtendUpdate(std::move(vec), _lastResizeTime, this);
    }

    _minSizeSinceUpdate = _values.size();
    _lastUpdateSize = _minSizeSinceUpdate;
}

bool DynArrayValueNode::encode(bmcl::MemWriter* dest) const
{
    if (!dest->writeVarUint(_values.size())) {
        //TODO: report error
        return false;
    }
    return ContainerValueNode::encode(dest);
}

bool DynArrayValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    uint64_t size;
    if (!src->readVarUint(&size)) {
        //TODO: report error
        return false;
    }
    if (size > _type->maxSize()) {
        //TODO: report error
        return false;
    }
    resizeDynArray(ctx.dataTimeOfOrigin(), size);
    return ContainerValueNode::decode(ctx, src);
}

const decode::Type* DynArrayValueNode::type() const
{
    return _type.get();
}

std::size_t DynArrayValueNode::maxSize() const
{
    return _type->maxSize();
}

bmcl::Option<std::size_t> DynArrayValueNode::canBeResized() const
{
    return bmcl::Option<std::size_t>(_type->maxSize());
}

bool DynArrayValueNode::resizeNode(std::size_t size)
{
    if (size > _type->maxSize()) {
        return false;
    }
    resizeDynArray(OnboardTime::now(), size);
    return true;
}

void DynArrayValueNode::resizeDynArray(OnboardTime time, std::size_t size)
{
    _lastResizeTime = time;
    _minSizeSinceUpdate = std::min(_minSizeSinceUpdate, size);
    std::size_t currentSize = _values.size();
    if (size > currentSize) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - currentSize); i++) {
            Rc<ValueNode> node = ValueNode::fromType(_type->elementType(), _cache.get(), this);
            node->setFieldName(_cache->arrayIndex(currentSize + i));
            _values.push_back(node);
        }
    } else if (size < currentSize) {
        _values.resize(size);
    }
    assert(values().size() == size);
}

StructValueNode::StructValueNode(const decode::StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
    , _changedSinceUpdate(false)
{
    _values.reserve(type->fieldsRange().size());
    for (const decode::Field* field : type->fieldsRange()) {
        Rc<ValueNode> node = ValueNode::fromField(field, _cache.get(), this);
        _values.push_back(node);
    }
}

StructValueNode::~StructValueNode()
{
}

void StructValueNode::collectUpdates(NodeViewUpdater* dest)
{
    for (const Rc<ValueNode>& node : _values) {
        node->collectUpdates(dest);
    }
    _changedSinceUpdate = false;
}

bool StructValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    _changedSinceUpdate = true;
    return ContainerValueNode::decode(ctx, src);
}

const decode::Type* StructValueNode::type() const
{
    return _type.get();
}

bmcl::OptionPtr<ValueNode> StructValueNode::nodeWithName(bmcl::StringView name)
{
    for (const Rc<ValueNode>& node : _values) {
        if (node->fieldName() == name) {
            return node.get();
        }
    }
    return bmcl::None;
}

VariantValueNode::VariantValueNode(const decode::VariantType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ContainerValueNode(cache, parent)
    , _type(type)
{
}

VariantValueNode::~VariantValueNode()
{
}

Value VariantValueNode::value() const
{
    if (_currentId.isSome()) {
        return Value::makeStringView(_type->fieldsBegin()[_currentId.unwrap().value()]->name());
    }
    return Value::makeUninitialized();
}

void VariantValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_currentId.isNone()) {
        return;
    }

    if (!_currentId.unwrap().hasChanged()) {
        ContainerValueNode::collectUpdates(dest);
        return;
    }

    OnboardTime t = _currentId.unwrap().lastOnboardUpdateTime();
    if (_currentId.unwrap().hasValueChanged()) {
        dest->addValueUpdate(value(), t, this);
    } else {
        dest->addTimeUpdate(t, this);
    }
    dest->addShrinkUpdate(std::size_t(0), t, this);
    if (!_values.empty()) {
        NodeViewVec vec;
        vec.reserve(_values.size());
        for (const Rc<ValueNode>& node : _values) {
            vec.emplace_back(new NodeView(node.get(), t));
        }
        dest->addExtendUpdate(std::move(vec), t, this);
    }
    _currentId.unwrap().updateState();
}

bool VariantValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isNone()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarInt(_currentId.unwrap().value()));
    return ContainerValueNode::encode(dest);
}

bool VariantValueNode::canSetValue() const
{
    return true;
}

void VariantValueNode::selectId(OnboardTime time, std::int64_t id)
{
    updateOptionalValuePair(&_currentId, time, id);
    if (!_currentId.unwrap().hasValueChanged()) {
        return;
    }
    //TODO: do not resize if type doesn't change
    const decode::VariantField* field = _type->fieldsBegin()[id];
    switch (field->variantFieldKind()) {
    case decode::VariantFieldKind::Constant:
        _values.resize(0);
        break;
    case decode::VariantFieldKind::Tuple: {
        const decode::TupleVariantField* tField = static_cast<const decode::TupleVariantField*>(field);
        std::size_t currentSize = _values.size();
        std::size_t size = tField->typesRange().size();
        if (size < currentSize) {
            _values.resize(size);
        } else if (size > currentSize) {
            _values.resize(size);
        }
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(tField->typesBegin()[i], _cache.get(), this);
            _values[i]->setFieldName(_cache->arrayIndex(i));
        }
        break;
    }
    case decode::VariantFieldKind::Struct: {
        const decode::StructVariantField* sField = static_cast<const decode::StructVariantField*>(field);
        std::size_t currentSize = _values.size();
        std::size_t size = sField->fieldsRange().size();
        if (size < currentSize) {
            _values.resize(size);
        } else if (size > currentSize) {
            _values.resize(size);
        }
        for (std::size_t i = 0; i < size; i++) {
            const decode::Field* field = sField->fieldsBegin()[i];
            _values[i] = ValueNode::fromField(field, _cache.get(), this);
        }
        break;
    }
    default:
        assert(false);
    }
}

bool VariantValueNode::selectEnum(OnboardTime time, bmcl::StringView name)
{
    std::size_t i = 0;
    for (const decode::VariantField* field : _type->fieldsRange()) {
        if (field->name() == name) {
            selectId(time, i);
            return true;
        }
        i++;
    }
    return false;
}

ValueKind VariantValueNode::valueKind() const
{
    return ValueKind::StringView;
}

bool VariantValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    int64_t id;
    TRY(src->readVarInt(&id));
    if (id < 0) {
        //TODO: report error
        return false;
    }
    if (uint64_t(id) >= _type->fieldsRange().size()) {
        //TODO: report error
        return false;
    }
    selectId(ctx.dataTimeOfOrigin(), id);
    TRY(ContainerValueNode::decode(ctx, src));
    return true;
}

bool VariantValueNode::setValue(const Value& value)
{
    switch (value.kind()) {
    case ValueKind::String:
        return selectEnum(OnboardTime::now(), value.asString());
    case ValueKind::StringView:
        return selectEnum(OnboardTime::now(), value.asStringView());
    default:
        return false;
    }
    return false;
}

bmcl::Option<std::vector<Value>> VariantValueNode::possibleValues() const
{
    std::vector<Value> values;
    values.reserve(_type->fieldsRange().size());
    for (const decode::VariantField* field : _type->fieldsRange()) {
        values.emplace_back(Value::makeStringView(field->name()));
    }
    return std::move(values);
}

const decode::Type* VariantValueNode::type() const
{
    return _type.get();
}

NonContainerValueNode::NonContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : ValueNode(cache, parent)
{
}

bool NonContainerValueNode::isContainerValue() const
{
    return false;
}

bool NonContainerValueNode::canHaveChildren() const
{
    return false;
}

bool NonContainerValueNode::canSetValue() const
{
    return true;
}

StringValueNode::StringValueNode(const decode::DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
    , _lastUpdateTime(OnboardTime::now())
    , _hasChanged(false)
{
    assert(type->elementType()->isBuiltin());
    assert(type->elementType()->asBuiltin()->builtinTypeKind() == decode::BuiltinTypeKind::Char);
}

StringValueNode::~StringValueNode()
{
}

const decode::Type* StringValueNode::type() const
{
    return _type.get();
}

void StringValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if ( _value.isSome() && _hasChanged) {
        dest->addValueUpdate(value(), _lastUpdateTime, this);
        _hasChanged = false;
    }
}

bool StringValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    if (!dest->writeVarUint(_value->size())) {
        //TODO: report error
        return false;
    }
    if (dest->sizeLeft() < _value->size()) {
        //TODO: report error
        return false;
    }
    dest->write(_value->data(), _value->size());
    return true;
}

bool StringValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    uint64_t size;
    if (!src->readVarUint(&size)) {
        //TODO: report error
        return false;
    }
    if (src->sizeLeft() < size) {
        //TODO: report error
        return false;
    }
    if (_value.isNone()) {
        _value.emplace();
    }
    _hasChanged = true;
    _lastUpdateTime = ctx.dataTimeOfOrigin();
    _value->assign((const char*)src->current(), size);
    src->skip(size);
    return true;
}

bool StringValueNode::isInitialized() const
{
    return _value.isSome();
}

Value StringValueNode::value() const
{
    if (_value.isSome()) {
        return Value::makeString(_value.unwrap());
    }
    return Value::makeUninitialized();
}

ValueKind StringValueNode::valueKind() const
{
    return ValueKind::String;
}

bool StringValueNode::setValue(const Value& value)
{
    if (value.kind() == ValueKind::String) {
        if (value.asString().size() > _type->maxSize()) {
            //TODO: report error
            return false;
        }
        _lastUpdateTime = OnboardTime::now();
        _hasChanged = true;
        _value.emplace(value.asString());
        return true;
    } else if (value.kind() == ValueKind::StringView) {
        if (value.asStringView().size() > _type->maxSize()) {
            //TODO: report error
            return false;
        }
        _lastUpdateTime = OnboardTime::now();
        _hasChanged = true;
        _value.emplace(value.asStringView().toStdString());
        return true;
    }
    return false;
}

AddressValueNode::AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
{
}

AddressValueNode::~AddressValueNode()
{
}

void AddressValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_address.isNone()) {
        return;
    }

    if (!_address.unwrap().hasChanged()) {
        return;
    }

    if (_address.unwrap().hasValueChanged()) {
        dest->addValueUpdate(Value::makeUnsigned(_address.unwrap().value()), _address.unwrap().lastOnboardUpdateTime(), this);
    } else {
        dest->addTimeUpdate(_address.unwrap().lastOnboardUpdateTime(), this);
    }

    _address.unwrap().updateState();
}

bool AddressValueNode::encode(bmcl::MemWriter* dest) const
{
    //TODO: get target word size
    if (_address.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < 8) {
        return false;
    }
    dest->writeUint64Le(_address.unwrap().value());
    return true;
}

bool AddressValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        //TODO: report error
        return false;
    }
    uint64_t value = src->readUint64Le();
    updateOptionalValuePair(&_address, ctx.dataTimeOfOrigin(), value);
    return true;
}

bool AddressValueNode::isInitialized() const
{
    return _address.isSome();
}

Value AddressValueNode::value() const
{
    if (_address.isSome()) {
        return Value::makeUnsigned(_address.unwrap().value());
    }
    return Value::makeUninitialized();
}

ValueKind AddressValueNode::valueKind() const
{
    return ValueKind::Unsigned;
}

bool AddressValueNode::setValue(const Value& value)
{
    if (value.isA(ValueKind::Unsigned)) {
        //TODO: check word size
        _address.emplace(OnboardTime::now(), value.asUnsigned());
        return true;
    }
    return false;
}

ReferenceValueNode::ReferenceValueNode(const decode::ReferenceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : AddressValueNode(cache, parent)
    , _type(type)
{
}

ReferenceValueNode::~ReferenceValueNode()
{
}

const decode::Type* ReferenceValueNode::type() const
{
    return _type.get();
}

FunctionValueNode::FunctionValueNode(const decode::FunctionType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : AddressValueNode(cache, parent)
    , _type(type)
{
}

FunctionValueNode::~FunctionValueNode()
{
}

const decode::Type* FunctionValueNode::type() const
{
    return _type.get();
}

EnumValueNode::EnumValueNode(const decode::EnumType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

EnumValueNode::~EnumValueNode()
{
}
void EnumValueNode::collectUpdates(NodeViewUpdater* dest)
{
    if (_currentId.isNone()) {
        return;
    }

    if (!_currentId.unwrap().hasChanged()) {
        return;
    }

    if (_currentId.unwrap().hasValueChanged()) {
        dest->addValueUpdate(value(), _currentId.unwrap().lastOnboardUpdateTime(), this);
    } else {
        dest->addTimeUpdate(_currentId.unwrap().lastOnboardUpdateTime(), this);
    }

    _currentId.unwrap().updateState();
}

bool EnumValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isSome()) {
        TRY(dest->writeVarInt(_currentId.unwrap().value()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    int64_t value;
    if (!src->readVarInt(&value)) {
        //TODO: report error
        return false;
    }
    auto it = _type->constantsRange().findIf([value](const decode::EnumConstant* c) {
        return c->value() == value;
    });
    if (it == _type->constantsEnd()) {
        //TODO: report error
        return false;
    }
    updateOptionalValuePair(&_currentId, ctx.dataTimeOfOrigin(), value);
    return true;
}

bmcl::Option<std::vector<Value>> EnumValueNode::possibleValues() const
{
    std::vector<Value> values;
    values.reserve(_type->constantsRange().size());
    for (const decode::EnumConstant* c : _type->constantsRange()) {
        values.emplace_back(Value::makeStringView(c->name()));
    }
    return std::move(values);
}

Value EnumValueNode::value() const
{
    if (_currentId.isSome()) {
        auto it = _type->constantsRange().findIf([this](const decode::EnumConstant* c) {
            return c->value() == _currentId.unwrap().value();
        });
        assert(it != _type->constantsEnd());
        return Value::makeStringView(it->name());
    }
    return Value::makeUninitialized();
}

ValueKind EnumValueNode::valueKind() const
{
    return ValueKind::StringView;
}

bool EnumValueNode::selectEnum(OnboardTime time, bmcl::StringView value)
{
    for (const decode::EnumConstant* c : _type->constantsRange()) {
        if (c->name() == value) {
            _currentId.emplace(time, c->value());
            return true;
        }
    }
    return false;
}

bool EnumValueNode::setValue(const Value& value)
{
    switch (value.kind()) {
    case ValueKind::String:
        return selectEnum(OnboardTime::now(), value.asString());
    case ValueKind::StringView:
        return selectEnum(OnboardTime::now(), value.asStringView());
    default:
        return false;
    }
    return false;
}

bool EnumValueNode::isInitialized() const
{
    return _currentId.isSome();
}

const decode::Type* EnumValueNode::type() const
{
    return _type.get();
}

BuiltinValueNode::BuiltinValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NonContainerValueNode(cache, parent)
    , _type(type)
{
}

BuiltinValueNode::~BuiltinValueNode()
{
}

void BuiltinValueNode::setRangeAttribute(const decode::RangeAttr* attr)
{
    _rangeAttr.reset(attr);
}

Rc<BuiltinValueNode> BuiltinValueNode::fromType(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    assert(cache);
    return builtinNodeFromType(type, cache, parent);
}

const decode::Type* BuiltinValueNode::type() const
{
    return _type.get();
}

template <typename T>
NumericValueNode<T>::NumericValueNode(const decode::BuiltinType* type,const ValueInfoCache* cache,  bmcl::OptionPtr<Node> parent)
    : BuiltinValueNode(type, cache, parent)
{
}

template <typename T>
NumericValueNode<T>::~NumericValueNode()
{
}

template <typename T>
void NumericValueNode<T>::collectUpdates(NodeViewUpdater* dest)
{
    if (_value.isNone()) {
        return;
    }

    if (!_value.unwrap().hasChanged()) {
        return;
    }

    OnboardTime t = _value.unwrap().lastOnboardUpdateTime();

    if (_value.unwrap().hasValueChanged()) {
        if (std::is_floating_point<T>::value) {
            dest->addValueUpdate(Value::makeDouble(_value.unwrap().value()), t, this);
        } else if (std::is_signed<T>::value) {
            dest->addValueUpdate(Value::makeSigned(_value.unwrap().value()), t, this);
        } else {
            dest->addValueUpdate(Value::makeUnsigned(_value.unwrap().value()), t, this);
        }
    } else {
        dest->addTimeUpdate(t, this);
    }

    _value.unwrap().updateState();
}

template <typename T>
bool NumericValueNode<T>::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    dest->writeType<T>(bmcl::htole<T>(_value.unwrap().value()));
    return true;
}

template <typename T>
bool NumericValueNode<T>::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    T value = bmcl::letoh<T>(src->readType<T>());
    updateOptionalValuePair(&_value, ctx.dataTimeOfOrigin(), value);
    return true;
}

template <typename T>
Value NumericValueNode<T>::value() const
{
    if (_value.isNone()) {
        return Value::makeUninitialized();
    }
    if (std::is_floating_point<T>::value) {
        return Value::makeDouble(_value.unwrap().value());
    } else if (std::is_signed<T>::value) {
        return Value::makeSigned(_value.unwrap().value());
    } else {
        return Value::makeUnsigned(_value.unwrap().value());
    }
}

template <typename T>
bool NumericValueNode<T>::isInitialized() const
{
    return _value.isSome();
}

template <typename T>
ValueKind NumericValueNode<T>::valueKind() const
{
    if (std::is_floating_point<T>::value) {
        return ValueKind::Double;
    } else if (std::is_signed<T>::value) {
        return ValueKind::Signed;
    }
    return ValueKind::Unsigned;
}

template <typename T>
bool NumericValueNode<T>::emplace(OnboardTime time, intmax_t value)
{
    if (std::is_unsigned<T>()) {
        if (value < 0) {
            return false;
        }
        if (uintmax_t(value) >= std::numeric_limits<T>::min() && uintmax_t(value) <= std::numeric_limits<T>::max()) {
            _value.emplace(time, value);
            return true;
        }
        return false;
    } else {
        if (value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max()) {
            _value.emplace(time, value);
            return true;
        }
        return false;
    }
}

template <typename T>
bool NumericValueNode<T>::emplace(OnboardTime time, uintmax_t value)
{
    if (value <= std::numeric_limits<T>::max()) {
        _value.emplace(time, value);
        return true;
    }
    return false;
}

template <typename T>
bool NumericValueNode<T>::emplace(OnboardTime time, double value)
{
    if (value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max()) {
        _value.emplace(time, value);
        return true;
    }
    return false;
}

template <typename T>
bool NumericValueNode<T>::setValue(const Value& value)
{
    if (value.isA(ValueKind::Double)) {
        return emplace(OnboardTime::now(), value.asDouble());
    } else if (value.isA(ValueKind::Signed)) {
        return emplace(OnboardTime::now(), value.asSigned());
    } else if (value.isA(ValueKind::Unsigned)) {
        return emplace(OnboardTime::now(), value.asUnsigned());
    }
    return false;
}

template <typename T>
bmcl::Option<T> NumericValueNode<T>::rawValue() const
{
    if (_value.isSome()) {
        return _value.unwrap().value();
    }
    return bmcl::None;
}

template <typename T>
void NumericValueNode<T>::setRawValue(T value)
{
    updateOptionalValuePair(&_value, OnboardTime::now(), value);
}

template <typename T>
bool NumericValueNode<T>::isDefault() const
{
    if (_value.isNone() || !_rangeAttr) {
        return false;
    }
    return _rangeAttr->valueIsDefault(_value.unwrap().value());
}

template <typename T>
bool NumericValueNode<T>::isInRange() const
{
    if (_value.isNone()) {
        return false;
    }
    if (!_rangeAttr) {
        return true;
    }
    return _rangeAttr->valueIsInRange(_value.unwrap().value());
}

template class NumericValueNode<std::uint8_t>;
template class NumericValueNode<std::int8_t>;
template class NumericValueNode<std::uint16_t>;
template class NumericValueNode<std::int16_t>;
template class NumericValueNode<std::uint32_t>;
template class NumericValueNode<std::int32_t>;
template class NumericValueNode<std::uint64_t>;
template class NumericValueNode<std::int64_t>;
template class NumericValueNode<float>;
template class NumericValueNode<double>;

VarintValueNode::VarintValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NumericValueNode<int64_t>(type, cache, parent)
{
}

VarintValueNode::~VarintValueNode()
{
}

bool VarintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarInt(_value.unwrap().value());
}

bool VarintValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    int64_t value;
    //TODO: report error
    TRY(src->readVarInt(&value));
    updateOptionalValuePair(&_value, ctx.dataTimeOfOrigin(), value);
    return true;
}

VaruintValueNode::VaruintValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NumericValueNode<uint64_t>(type, cache, parent)
{
}

VaruintValueNode::~VaruintValueNode()
{
}

bool VaruintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarUint(_value.unwrap().value());
}

bool VaruintValueNode::decode(const DecoderCtx& ctx, bmcl::MemReader* src)
{
    uint64_t value;
    //TODO: report error
    TRY(src->readVarUint(&value));
    updateOptionalValuePair(&_value, ctx.dataTimeOfOrigin(), value);
    return true;
}
}
