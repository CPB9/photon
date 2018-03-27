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
#include "decode/core/HashMap.h"

namespace photon {

class NodeView;
class NodeViewUpdate;

class NodeViewStore {
public:
    NodeViewStore(NodeView* view);
    virtual ~NodeViewStore();

    void setRoot(NodeView* view);
    bool apply(NodeViewUpdate* update);

    virtual void beginExtend(NodeView* view, std::size_t extendSize);
    virtual void endExtend();

    virtual void beginShrink(NodeView* view, std::size_t newSize);
    virtual void endShrink();

    virtual void handleValueUpdate(NodeView* view);

private:
    void registerNodes(NodeView* view);
    void unregisterNodes(NodeView* view);

    Rc<NodeView> _root;
    decode::HashMap<uintptr_t, Rc<NodeView>> _map;
};
}
