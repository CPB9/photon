/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/ui/QNodeViewModel.h"
#include "photon/model/NodeView.h"
#include "photon/model/NodeViewUpdater.h"

namespace photon {

QNodeViewStore::QNodeViewStore(NodeView* view, QNodeViewModel* parent)
    : NodeViewStore(view)
    , _parent(parent)
{
}

QNodeViewStore::~QNodeViewStore()
{
}

void QNodeViewStore::beginExtend(NodeView* view, std::size_t extendSize)
{
    _lastIndex = _parent->indexFromNode(view, 0);
    _first = view->numChildren();
    _last = _first + extendSize;
    _parent->beginInsertRows(_lastIndex, _first, _last);
}

void QNodeViewStore::endExtend()
{
    _parent->endInsertRows();
}

void QNodeViewStore::beginShrink(NodeView* view, std::size_t newSize)
{
    _lastIndex = _parent->indexFromNode(view, 0);
    _last = view->numChildren();
    _first = newSize;
    _parent->beginRemoveRows(_lastIndex, _first, _last);
}

void QNodeViewStore::endShrink()
{
    _parent->endRemoveRows();
}

void QNodeViewStore::handleValueUpdate(NodeView* view)
{
    //TODO: implement updates
}

QNodeViewModel::QNodeViewModel(NodeView* node)
    : QModelBase<NodeView>(node)
    , _store(node, this)
{

}

QNodeViewModel::~QNodeViewModel()
{
}

void QNodeViewModel::applyUpdates(NodeViewUpdater* updater)
{
    updater->apply(&_store);
}

void QNodeViewModel::setRoot(NodeView* node)
{
    beginResetModel();
    _root = node;
    _store.setRoot(node);
    endResetModel();
}
}
