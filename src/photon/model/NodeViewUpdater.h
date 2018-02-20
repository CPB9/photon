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
#include "photon/model/NodeViewUpdate.h"
#include "photon/model/OnboardTime.h"

namespace photon {

class NodeViewStore;

class NodeViewUpdater : public RefCountable {
public:
    using Pointer = Rc<NodeViewUpdater>;
    using ConstPointer = Rc<const NodeViewUpdater>;

    NodeViewUpdater(const Node* owner);
    ~NodeViewUpdater();

    void addTimeUpdate(OnboardTime time, Node* parent);
    void addValueUpdate(Value&& value, OnboardTime time, Node* parent);
    void addShrinkUpdate(std::size_t size, OnboardTime time, Node* parent);
    void addExtendUpdate(NodeViewVec&& vec, OnboardTime time, Node* parent);
    void apply(NodeViewStore* dest);

    const std::vector<NodeViewUpdate>& updates() const;
    std::vector<NodeViewUpdate>& updates();

private:
    std::vector<NodeViewUpdate> _updates;
    Rc<const Node> _owner;
};
}
