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
#include "photon/model/NodeViewStore.h"
#include "photon/model/NodeView.h"
#include "photon/ui/QModelBase.h"

#include <bmcl/Fwd.h>

#include <QAbstractItemModel>

namespace photon {

class NodeView;
class NodeViewUpdater;
class QNodeViewModel;

class QNodeViewStore : public NodeViewStore {
public:
    QNodeViewStore(NodeView* view, QNodeViewModel* parent);
    ~QNodeViewStore();

    void beginExtend(NodeView* view, std::size_t extendSize) override;
    void endExtend() override;

    void beginShrink(NodeView* view, std::size_t newSize) override;
    void endShrink() override;

    void handleValueUpdate(NodeView* view) override;

private:
    QNodeViewModel* _parent;
    QModelIndex _lastIndex;
    int _first;
    int _last;
};

class QNodeViewModel : public QModelBase<NodeView> {
    Q_OBJECT
public:
    QNodeViewModel(NodeView* node);
    ~QNodeViewModel();

    void applyUpdates(NodeViewUpdater* updater);
    void setRoot(NodeView* node);

private:
    friend class QNodeViewStore;

    QNodeViewStore _store;
};
}

