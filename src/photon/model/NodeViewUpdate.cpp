/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/model/NodeViewUpdate.h"
#include "photon/model/Node.h"

namespace photon {

NodeViewUpdate::NodeViewUpdate(Value&& value, Node* parent)
    : NodeViewUpdateBase(ValueUpdate(std::move(value), parent->isDefault(), parent->isInRange()))
    , _id(uintptr_t(parent))
{
}

NodeViewUpdate::NodeViewUpdate(NodeViewVec&& vec, Node* parent)
    : NodeViewUpdateBase(vec)
    , _id(uintptr_t(parent))
{
}

NodeViewUpdate::NodeViewUpdate(std::size_t size, Node* parent)
    : NodeViewUpdateBase(size)
    , _id(uintptr_t(parent))
{
}

NodeViewUpdate::~NodeViewUpdate()
{
}

uintptr_t NodeViewUpdate::id() const
{
    return _id;
}
}
