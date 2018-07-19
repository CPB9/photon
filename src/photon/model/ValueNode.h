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
#include "photon/model/Node.h"
#include "photon/model/ValueInfoCache.h"
#include "photon/model/OnboardTime.h"

#include <bmcl/Option.h>
#include <bmcl/OptionPtr.h>

#include <string>

namespace decode {
class ArrayType;
class DynArrayType;
class StructType;
class FunctionType;
class BuiltinType;
class AliasType;
class ImportedType;
class VariantType;
class EnumType;
class ReferenceType;
class RangeAttr;
class Field;
class Function;
}

namespace bmcl { class MemReader; class Buffer; }

namespace photon {

class CoderState;
class ValueInfoCache;
class NodeViewUpdater;

class ValueNode : public Node {
public:
    using Pointer = Rc<ValueNode>;
    using ConstPointer = Rc<const ValueNode>;

    ~ValueNode();

    static Rc<ValueNode> fromType(const decode::Type* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    static Rc<ValueNode> fromField(const decode::Field* field, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    virtual bool encode(CoderState* ctx, bmcl::Buffer* dest) const = 0;
    virtual bool decode(CoderState* ctx, bmcl::MemReader* src) = 0;

    virtual bool isContainerValue() const = 0;
    virtual bool isInitialized() const = 0;
    virtual const decode::Type* type() const = 0;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;
    bmcl::StringView shortDescription() const override;

    void setFieldName(bmcl::StringView name);
    void setShortDesc(bmcl::StringView desc);

    const ValueInfoCache* cache() const
    {
        return _cache.get();
    }

protected:

    explicit ValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const ValueInfoCache> _cache;

private:
    bmcl::StringView _fieldName;
    bmcl::StringView _shortDesc;
};

class ContainerValueNode : public ValueNode {
public:
    using Pointer = Rc<ContainerValueNode>;
    using ConstPointer = Rc<const ContainerValueNode>;

    ~ContainerValueNode();

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;

    bool isContainerValue() const override;
    bool isInitialized() const override;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;

    const ValueNode* nodeAt(std::size_t index) const;
    ValueNode* nodeAt(std::size_t index);

    const std::vector<Rc<ValueNode>>& values();

protected:
    explicit ContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    std::vector<Rc<ValueNode>> _values;
};

class ArrayValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<ArrayValueNode>;
    using ConstPointer = Rc<const ArrayValueNode>;

    ArrayValueNode(const decode::ArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ArrayValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    const decode::Type* type() const override;
    void stringify(decode::StringBuilder* dest) const override;

private:
    Rc<const decode::ArrayType> _type;
    bool _changedSinceUpdate;
};

class DynArrayValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<DynArrayValueNode>;
    using ConstPointer = Rc<const DynArrayValueNode>;

    DynArrayValueNode(const decode::DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~DynArrayValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    const decode::Type* type() const override;

    std::size_t maxSize() const;
    bmcl::Option<std::size_t> canBeResized() const override;
    bool resizeNode(std::size_t size) override;
    void resizeDynArray(OnboardTime time, std::size_t size);

private:
    Rc<const decode::DynArrayType> _type;
    OnboardTime _lastResizeTime;
    std::size_t _minSizeSinceUpdate;
    std::size_t _lastUpdateSize;
};

class StructValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<StructValueNode>;
    using ConstPointer = Rc<const StructValueNode>;

    StructValueNode(const decode::StructType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~StructValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    const decode::Type* type() const override;
    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

private:
    Rc<const decode::StructType> _type;
    bool _changedSinceUpdate;
};

template <typename T>
class ValuePair {
public:
    ValuePair(OnboardTime time, T value)
        : _updateTime(time)
        , _current(value)
        , _hasChanged(true)
    {
    }

    void updateState()
    {
        _hasChanged = false;
    }

    bool hasChanged() const
    {
        return _hasChanged;
    }

    bool hasValueChanged() const
    {
        return true; //HACK
    }

    void setValue(OnboardTime time, T value)
    {
        _updateTime = time;
        _current = value;
        _hasChanged = true;
    }

    T value() const
    {
        return _current;
    }

    OnboardTime lastOnboardUpdateTime() const
    {
        return _updateTime;
    }

private:
    OnboardTime _updateTime;
    T _current;
    bool _hasChanged;
};

class VariantValueNode : public ContainerValueNode {
public:
    using Pointer = Rc<VariantValueNode>;
    using ConstPointer = Rc<const VariantValueNode>;

    VariantValueNode(const decode::VariantType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VariantValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    ValueKind valueKind() const override;
    bool canSetValue() const override;
    const decode::Type* type() const override;
    Value value() const override;
    bool setValue(const Value& value) override;
    bmcl::Option<std::vector<Value>> possibleValues() const override;

private:
    void selectId(OnboardTime time, std::int64_t id);
    bool selectEnum(OnboardTime time, bmcl::StringView name);

    Rc<const decode::VariantType> _type;
    bmcl::Option<ValuePair<std::int64_t>> _currentId;
};

class NonContainerValueNode : public ValueNode {
public:
    using Pointer = Rc<NonContainerValueNode>;
    using ConstPointer = Rc<const NonContainerValueNode>;

    bool isContainerValue() const override;
    bool canHaveChildren() const override;

    bool canSetValue() const override;

protected:
    NonContainerValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
};

class StringValueNode : public NonContainerValueNode {
public:
    StringValueNode(const decode::DynArrayType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~StringValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    bool isInitialized() const override;
    Value value() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

    const decode::Type* type() const override;

private:
    Rc<const decode::DynArrayType> _type;
    bmcl::Option<std::string> _value;
    OnboardTime _lastUpdateTime;
    bool _hasChanged;
};

class AddressValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<AddressValueNode>;
    using ConstPointer = Rc<const AddressValueNode>;

    AddressValueNode(const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~AddressValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    bool isInitialized() const override;
    Value value() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

protected:
    bmcl::Option<ValuePair<uint64_t>> _address;
};

class ReferenceValueNode : public AddressValueNode {
public:
    using Pointer = Rc<ReferenceValueNode>;
    using ConstPointer = Rc<const ReferenceValueNode>;

    ReferenceValueNode(const decode::ReferenceType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~ReferenceValueNode();
    const decode::Type* type() const override;

private:
    Rc<const decode::ReferenceType> _type;
};

class FunctionValueNode : public AddressValueNode {
public:
    using Pointer = Rc<FunctionValueNode>;
    using ConstPointer = Rc<const FunctionValueNode>;

    FunctionValueNode(const decode::FunctionType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FunctionValueNode();
    const decode::Type* type() const override;

private:
    Rc<const decode::FunctionType> _type;
};

class EnumValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<EnumValueNode>;
    using ConstPointer = Rc<const EnumValueNode>;

    EnumValueNode(const decode::EnumType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~EnumValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    Value value() const override;
    ValueKind valueKind() const override;

    bool setValue(const Value& value) override;

    bool isInitialized() const override;
    const decode::Type* type() const override;
    bmcl::Option<std::vector<Value>> possibleValues() const override;

    //TODO: implement enum editing
    //ValueKind valueKind() const override;
    //bool setValue(const Value& value) override;

private:
    bool selectEnum(OnboardTime time, bmcl::StringView value);

    Rc<const decode::EnumType> _type;
    bmcl::Option<ValuePair<int64_t>> _currentId;
};

class BuiltinValueNode : public NonContainerValueNode {
public:
    using Pointer = Rc<BuiltinValueNode>;
    using ConstPointer = Rc<const BuiltinValueNode>;

    ~BuiltinValueNode();

    static Rc<BuiltinValueNode> fromType(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    const decode::Type* type() const override;

    void setRangeAttribute(const decode::RangeAttr* attr);

protected:
    BuiltinValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    Rc<const decode::RangeAttr> _rangeAttr;
    Rc<const decode::BuiltinType> _type;
};

template <typename T>
class NumericValueNode : public BuiltinValueNode {
public:
    using Pointer = Rc<NumericValueNode<T>>;
    using ConstPointer = Rc<const NumericValueNode<T>>;

    NumericValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~NumericValueNode();

    void collectUpdates(NodeViewUpdater* dest) override;

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
    void stringify(decode::StringBuilder* dest) const override;

    Value value() const override;

    bool isInitialized() const override;

    ValueKind valueKind() const override;
    bool setValue(const Value& value) override;

    bool isDefault() const override;
    bool isInRange() const override;

    bmcl::Option<T> rawValue() const;
    void setRawValue(T value, OnboardTime time = OnboardTime::now());
    void incRawValue(OnboardTime time = OnboardTime::now());

protected:
    bool emplace(OnboardTime time, intmax_t value);
    bool emplace(OnboardTime time, uintmax_t value);
    bool emplace(OnboardTime time, double value);

    bmcl::Option<ValuePair<T>> _value;
};

extern template class NumericValueNode<std::uint8_t>;
extern template class NumericValueNode<std::int8_t>;
extern template class NumericValueNode<std::uint16_t>;
extern template class NumericValueNode<std::int16_t>;
extern template class NumericValueNode<std::uint32_t>;
extern template class NumericValueNode<std::int32_t>;
extern template class NumericValueNode<std::uint64_t>;
extern template class NumericValueNode<std::int64_t>;
extern template class NumericValueNode<float>;
extern template class NumericValueNode<double>;

class VarintValueNode : public NumericValueNode<std::int64_t> {
public:
    using Pointer = Rc<VarintValueNode>;
    using ConstPointer = Rc<const VarintValueNode>;

    VarintValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VarintValueNode();

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
};

class VaruintValueNode : public NumericValueNode<std::uint64_t> {
public:
    using Pointer = Rc<VaruintValueNode>;
    using ConstPointer = Rc<const VaruintValueNode>;

    VaruintValueNode(const decode::BuiltinType* type, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~VaruintValueNode();

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const override;
    bool decode(CoderState* ctx, bmcl::MemReader* src) override;
};
}
