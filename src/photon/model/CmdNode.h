/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/model/FieldsNode.h"
#include "photon/model/ValueNode.h"
#include "photon/model/ValueInfoCache.h"

#include <bmcl/Fwd.h>

namespace decode {
class Function;
class Component;
}

namespace photon {

class ValueInfoCache;

class CmdNode : public FieldsNode {
public:
    using Pointer = Rc<CmdNode>;
    using ConstPointer = Rc<const CmdNode>;

    CmdNode(const decode::Component* comp, const decode::Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs = true);
    ~CmdNode();

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const;
    std::size_t numChildren() const override;
    bool canHaveChildren() const override;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;
    bmcl::StringView shortDescription() const override;

    Rc<CmdNode> clone(bmcl::OptionPtr<Node> parent) const;

    const decode::Function* function() const
    {
        return _func.get();
    }

    const ValueInfoCache* cache() const
    {
        return _cache.get();
    }


private:

    Rc<const decode::Component> _comp;
    Rc<const decode::Function> _func;
    Rc<const ValueInfoCache> _cache;

    bool _expandArgs;
};

template <typename T, typename B = Node>
class GenericContainerNode : public B {
public:
    template <typename... A>
    GenericContainerNode(A&&... args)
        : B(std::forward<A>(args)...)
    {
    }

    ~GenericContainerNode()
    {
    }

    std::size_t numChildren() const override
    {
        return _nodes.size();
    }

    bmcl::Option<std::size_t> childIndex(const Node* node) const override
    {
        return Node::childIndexGeneric(_nodes, node);
    }

    bmcl::OptionPtr<Node> childAt(std::size_t idx) override
    {
        return Node::childAtGeneric(_nodes, idx);
    }

    typename decode::RcVec<T>::ConstRange nodes() const
    {
        return _nodes;
    }

    typename decode::RcVec<T>::Range nodes()
    {
        return _nodes;
    }

protected:
    decode::RcVec<T> _nodes;
};

extern template class GenericContainerNode<CmdNode, Node>;

class ScriptNode : public GenericContainerNode<CmdNode, Node> {
public:
    ScriptNode(bmcl::OptionPtr<Node> parent);
    ~ScriptNode();

    static Rc<ScriptNode> withAllCmds(const decode::Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs);

    void addCmdNode(CmdNode* node);

    bool encode(CoderState* ctx, bmcl::Buffer* dest) const;

    bmcl::StringView fieldName() const override;

    void moveNode(std::size_t i1, std::size_t i2);

    void clear();

    void removeCmd(const CmdNode* cmd);

private:
    bmcl::StringView _fieldName;
};

extern template class GenericContainerNode<ValueNode, Node>;

class ScriptResultNode : public GenericContainerNode<ValueNode, Node> {
public:
    ScriptResultNode(bmcl::OptionPtr<Node> parent);
    ~ScriptResultNode();

    static Rc<ScriptResultNode> fromScriptNode(const ScriptNode* node, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    bool decode(CoderState* ctx, bmcl::MemReader* src);

    bmcl::StringView fieldName() const override;
};
}
