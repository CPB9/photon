/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/model/Value.h"
#include "photon/model/NodeView.h"
#include "photon/model/OnboardTime.h"

#include <bmcl/Variant.h>

namespace photon {

class Model;
class Node;
class NodeViewUpdate;
class NodeView;
class NodeViewStore;

enum class NodeViewUpdateKind {
    None,
    Value,
    Extend,
    Shrink,
};

struct IndexAndNodeView {
    std::size_t index;
    Rc<NodeView> child;
};

struct ValueUpdate {
    ValueUpdate(Value&& value, bool isDefault, bool isInRange)
        : value(std::move(value))
        , isDefault(isDefault)
        , isInRange(isInRange)
    {
    }
    Value value;
    bool isDefault;
    bool isInRange;
};

using NodeViewUpdateBase =
    bmcl::Variant<NodeViewUpdateKind, NodeViewUpdateKind::None,
        bmcl::VariantElementDesc<NodeViewUpdateKind, ValueUpdate, NodeViewUpdateKind::Value>,
        bmcl::VariantElementDesc<NodeViewUpdateKind, NodeViewVec, NodeViewUpdateKind::Extend>,
        bmcl::VariantElementDesc<NodeViewUpdateKind, std::size_t, NodeViewUpdateKind::Shrink>
    >;

class NodeViewUpdate : public NodeViewUpdateBase {
public:
    NodeViewUpdate(OnboardTime time, Node* parent);
    NodeViewUpdate(Value&& value, OnboardTime time, Node* parent);
    NodeViewUpdate(NodeViewVec&& vec, OnboardTime time, Node* parent);
    NodeViewUpdate(std::size_t size, OnboardTime time, Node* parent);
    ~NodeViewUpdate();

    uintptr_t id() const;
    OnboardTime time() const;

private:
    OnboardTime _time;
    uintptr_t _id;
};
}
