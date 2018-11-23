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
#include "photon/model/ValueKind.h"
#include "photon/model/OnboardTime.h"

#include <bmcl/Fwd.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/Option.h>
#include <bmcl/StringView.h>

#include <algorithm>

namespace decode {
class StringBuilder;
}

namespace photon {

class Value;
class NodeView;
class NodeViewUpdater;

class Node : public RefCountable {
public:
    using Pointer = Rc<Node>;
    using ConstPointer = Rc<const Node>;

    explicit Node(bmcl::OptionPtr<Node> parent);
    ~Node();

    void setParent(Node* node);
    bool hasParent() const;
    bmcl::OptionPtr<const Node> parent() const;
    bmcl::OptionPtr<Node> parent();

    virtual void collectUpdates(NodeViewUpdater* dest);

    virtual bool canHaveChildren() const;
    virtual std::size_t numChildren() const;
    virtual bmcl::Option<std::size_t> canBeResized() const;
    virtual bmcl::Option<std::size_t> childIndex(const Node* node) const;
    virtual bmcl::OptionPtr<Node> childAt(std::size_t idx);
    virtual bmcl::StringView fieldName() const;
    virtual bmcl::StringView typeName() const;
    virtual bmcl::StringView shortDescription() const;
    virtual bool resizeNode(std::size_t size);
    virtual Value value() const;
    virtual ValueKind valueKind() const;
    virtual bool canSetValue() const;
    virtual bool setValue(const Value& value);
    virtual bool isDefault() const;
    virtual bool isInRange() const;
    virtual bmcl::Option<OnboardTime> lastUpdateTime() const;
    virtual bmcl::Option<std::vector<Value>> possibleValues() const;
    virtual void stringify(decode::StringBuilder* dest) const;

    bmcl::Option<std::size_t> indexInParent() const;

protected:
    template <typename C>
    static bmcl::Option<std::size_t> childIndexGeneric(const C& cont, const Node* node)
    {
        auto it = std::find(cont.cbegin(), cont.cend(), node);
        if (it == cont.cend()) {
            return bmcl::None;
        }
        return std::distance(cont.cbegin(), it);
    }

    template <typename C>
    static bmcl::OptionPtr<Node> childAtGeneric(const C& cont, std::size_t idx)
    {
        if (idx >= cont.size()) {
            return bmcl::None;
        }
        return cont[idx].get();
    }

    template <typename C>
    static void collectUpdatesGeneric(const C& cont, NodeViewUpdater* dest)
    {
        for (const auto& node : cont) {
            node->collectUpdates(dest);
        }
    }


private:
    bmcl::OptionPtr<Node> _parent; // not owned
};
}
