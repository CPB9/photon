/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/NodeViewUpdater.h"
#include "photon/model/NodeViewStore.h"
#include "photon/model/Node.h"

namespace photon {

NodeViewUpdater::NodeViewUpdater(const Node* owner)
    : _owner(owner)
{
}

NodeViewUpdater::~NodeViewUpdater()
{
}

void NodeViewUpdater::addValueUpdate(Value&& value, Node* parent)
{
    _updates.emplace_back(std::move(value), parent);
}

void NodeViewUpdater::addShrinkUpdate(std::size_t size, Node* parent)
{
    _updates.emplace_back(size, parent);
}

void NodeViewUpdater::addExtendUpdate(NodeViewVec&& vec, Node* parent)
{
    _updates.emplace_back(std::move(vec), parent);
}

void NodeViewUpdater::apply(NodeViewStore* dest)
{
    for (NodeViewUpdate& update : _updates) {
        dest->apply(&update);
    }
}

const std::vector<NodeViewUpdate>& NodeViewUpdater::updates() const
{
    return _updates;
}
}
