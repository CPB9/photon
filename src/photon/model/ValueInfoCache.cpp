/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/ValueInfoCache.h"
#include "decode/ast/Type.h"
#include "decode/parser/Package.h"
#include "decode/ast/AstVisitor.h"
#include "decode/ast/Component.h"
#include "decode/core/StringBuilder.h"
#include "decode/core/Foreach.h"

#include <bmcl/Logging.h>

namespace photon {

// vector<string> when relocating copies strings instead of moving for some reason
// using allocated const char* instead
static void updateIndexes(ValueInfoCache::StringVecType* dest, std::size_t newSize)
{
    dest->reserve(newSize);
    for (std::size_t i = dest->size(); i < newSize; i++) {
        char* buf = (char*)alloca(sizeof(std::size_t) * 4);
        int stringSize = std::sprintf(buf, "%zu", i);
        assert(stringSize > 0);
        char* allocatedBuf  = new char[stringSize];
        std::memcpy(allocatedBuf, buf, stringSize);
        dest->push_back(bmcl::StringView(allocatedBuf, stringSize));
    }
}

static void buildNamedTypeName(const decode::NamedType* type, decode::StringBuilder* dest)
{
    dest->append(type->moduleName());
    dest->append("::");
    dest->append(type->name());
}

static bool buildTypeName(const decode::Type* type, decode::StringBuilder* dest)
{
    switch (type->typeKind()) {
        case decode::TypeKind::Builtin: {
        const decode::BuiltinType* builtin = type->asBuiltin();
        switch (builtin->builtinTypeKind()) {
        case decode::BuiltinTypeKind::USize:
            dest->append("usize");
            return true;
        case decode::BuiltinTypeKind::ISize:
            dest->append("isize");
            return true;
        case decode::BuiltinTypeKind::Varuint:
            dest->append("varuint");
            return true;
        case decode::BuiltinTypeKind::Varint:
            dest->append( "varint");
            return true;
        case decode::BuiltinTypeKind::U8:
            dest->append( "u8");
            return true;
        case decode::BuiltinTypeKind::I8:
            dest->append( "i8");
            return true;
        case decode::BuiltinTypeKind::U16:
            dest->append( "u16");
            return true;
        case decode::BuiltinTypeKind::I16:
            dest->append( "i16");
            return true;
        case decode::BuiltinTypeKind::U32:
            dest->append( "u32");
            return true;
        case decode::BuiltinTypeKind::I32:
            dest->append( "i32");
            return true;
        case decode::BuiltinTypeKind::U64:
            dest->append( "u64");
            return true;
        case decode::BuiltinTypeKind::I64:
            dest->append( "i64");
            return true;
        case decode::BuiltinTypeKind::F32:
            dest->append( "f32");
            return true;
        case decode::BuiltinTypeKind::F64:
            dest->append( "f64");
            return true;
        case decode::BuiltinTypeKind::Bool:
            dest->append( "bool");
            return true;
        case decode::BuiltinTypeKind::Void:
            dest->append("void");
            return true;
        case decode::BuiltinTypeKind::Char:
            dest->append("char");
            return true;
        }
        assert(false);
        break;
    }
    case decode::TypeKind::Reference: {
        const decode::ReferenceType* ref = type->asReference();
        if (ref->referenceKind() == decode::ReferenceKind::Pointer) {
            if (ref->isMutable()) {
                dest->append("*mut ");
            } else {
                dest->append("*const ");
            }
        } else {
            if (ref->isMutable()) {
                dest->append("&mut ");
            } else {
                dest->append('&');
            }
        }
        buildTypeName(ref->pointee(), dest);
        return true;
    }
    case decode::TypeKind::Array: {
        const decode::ArrayType* array = type->asArray();
        dest->append('[');
        buildTypeName(array->elementType(), dest);
        dest->append("; ");
        dest->appendNumericValue(array->elementCount());
        dest->append(']');
        return true;
    }
    case decode::TypeKind::DynArray: {
        const decode::DynArrayType* dynArray = type->asDynArray();
        dest->append("&[");
        buildTypeName(dynArray->elementType(), dest);
        dest->append("; ");
        dest->appendNumericValue(dynArray->maxSize());
        dest->append(']');
        return true;
    }
    case decode::TypeKind::Function: {
        const decode::FunctionType* func = type->asFunction();
        dest->append("&Fn(");
        foreachList(func->argumentsRange(), [dest](const decode::Field* arg){
            dest->append(arg->name());
            dest->append(": ");
            buildTypeName(arg->type(), dest);
        }, [dest](const decode::Field*){
            dest->append(", ");
        });
        bmcl::OptionPtr<const decode::Type> rv = func->returnValue();
        if (rv.isSome()) {
            dest->append(") -> ");
            buildTypeName(rv.unwrap(), dest);
        } else {
            dest->append(')');
        }
        return true;
    }
    case decode::TypeKind::Enum:
        buildNamedTypeName(type->asEnum(), dest);
        return true;
    case decode::TypeKind::Struct:
        buildNamedTypeName(type->asStruct(), dest);
        return true;
    case decode::TypeKind::Variant:
        buildNamedTypeName(type->asVariant(), dest);
        return true;
    case decode::TypeKind::Imported:
        buildNamedTypeName(type->asImported(), dest);
        return true;
    case decode::TypeKind::Alias:
        buildNamedTypeName(type->asAlias(), dest);
        return true;
    case decode::TypeKind::Generic:
        return false;
    case decode::TypeKind::GenericInstantiation: {
        const decode::GenericInstantiationType* t = type->asGenericInstantiation();
        dest->append(t->moduleName());
        dest->append("::");
        dest->append(t->genericName());
        dest->append("<");
        foreachList(t->substitutedTypesRange(), [dest](const decode::Type* type) {
            buildTypeName(type, dest);
        }, [dest](const decode::Type* type) {
            dest->append(", ");
        });
        dest->append(">");
        return false;
    }
    case decode::TypeKind::GenericParameter:
        assert(false);
        return false;
    }
    assert(false);
}

class CacheCollector : public decode::ConstAstVisitor<CacheCollector> {
public:
    void collectNames(const decode::Package* package,
                      ValueInfoCache::TypeMapType* typeNameMap,
                      ValueInfoCache::TmMsgMapType* tmMsgNameMap,
                      ValueInfoCache::StringVecType* strIndexes)
    {
        _typeNameMap = typeNameMap;
        _tmMsgNameMap = tmMsgNameMap;
        _strIndexes = strIndexes;
        for (const decode::Ast* ast : package->modules()) {
            for (const decode::Type* type : ast->typesRange()) {
                traverseType(type);
            }
            if (ast->component().isNone()) {
                continue;
            }
            const decode::Component* comp = ast->component().unwrap();
            for (const decode::StatusMsg* msg : comp->statusesRange()) {
                addTmMsg(comp, msg);
            }
            for (const decode::EventMsg* msg : comp->eventsRange()) {
                addTmMsg(comp, msg);
            }
        }
    }

    void addTmMsg(const decode::Component* comp, const decode::TmMsg* msg)
    {
        std::string name;
        name.reserve(comp->name().size() + 2 + msg->name().size());
        name.append(comp->name().begin(), comp->name().end());
        name.append("::", 2);
        name.append(msg->name().begin(), msg->name().end());
        _tmMsgNameMap->emplace(msg, std::move(name));
    }

    bool visitArrayType(const decode::ArrayType* type)
    {
        updateIndexes(_strIndexes, type->elementCount());
        return true;
    }

    bool visitDynArrayType(const decode::DynArrayType* type)
    {
        updateIndexes(_strIndexes, type->maxSize());
        return true;
    }

    bool visitTupleVariantField(const decode::TupleVariantField* field)
    {
        updateIndexes(_strIndexes, field->typesRange().size());
        return true;
    }

    bool visitType(const decode::Type* type)
    {
        bool rv = true;
        auto pair = _typeNameMap->emplace(type, std::string());
        if (pair.second) {
            decode::StringBuilder b;
            rv = buildTypeName(type, &b);
            pair.first->second = b.view().toStdString();
            if (type->isGenericInstantiation()) {
                _typeNameMap->emplace(type->asGenericInstantiation()->instantiatedType(), pair.first->second);
            }
        }
        return rv;
    }

private:
    ValueInfoCache::TypeMapType* _typeNameMap;
    ValueInfoCache::TmMsgMapType* _tmMsgNameMap;
    ValueInfoCache::StringVecType* _strIndexes;
};

ValueInfoCache::ValueInfoCache(const decode::Package* package)
{
    CacheCollector b;
    updateIndexes(&_arrayIndexes, 64); //HACK: limit maximum number of commands
    b.collectNames(package, &_names, &_tmMsgs, &_arrayIndexes);

}

ValueInfoCache::~ValueInfoCache()
{
    for (bmcl::StringView view : _arrayIndexes) {
        delete [] view.data();
    }
}

bmcl::StringView ValueInfoCache::arrayIndex(std::size_t idx) const
{
    assert(idx < _arrayIndexes.size());
    return _arrayIndexes[idx];
}

bmcl::StringView ValueInfoCache::nameForType(const decode::Type* type) const
{
    auto it = _names.find(type);
    if (it == _names.end()) {
        return bmcl::StringView("UNITIALIZED");
    }
    return it->second;
}

bmcl::StringView ValueInfoCache::nameForTmMsg(const decode::TmMsg* msg) const
{
    auto it = _tmMsgs.find(msg);
    if (it == _tmMsgs.end()) {
        return bmcl::StringView("UNITIALIZED");
    }
    return it->second;
}
}
