/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/ui/QCmdModel.h"
#include "photon/model/CmdNode.h"

#include <bmcl/OptionPtr.h>
#include <bmcl/Logging.h>

#include <QMimeData>

namespace photon {

QCmdModel::QCmdModel(ScriptNode* node)
    : QNodeModel(node)
    , _cmds(node)
{
}

QCmdModel::~QCmdModel()
{
}

static QString _mimeStr = "decode/qcmdmodel_drag_node";

QMimeData* QCmdModel::mimeData(const QModelIndexList& indexes) const
{
    return packMimeData(indexes, _mimeStr);
}

QStringList QCmdModel::mimeTypes() const
{
    return QStringList{_mimeStr};
}

bmcl::OptionPtr<CmdNode> QCmdModel::decodeQModelDrop(const QMimeData* data)
{
    bmcl::OptionPtr<Node> node = QNodeModel::unpackMimeData(data, qmodelMimeStr());
    if (node.isNone()) {
        return bmcl::None;
    }
    CmdNode* cmdNode = dynamic_cast<CmdNode*>(node.unwrap());
    if (!cmdNode) {
        return bmcl::None;
    }
    return cmdNode;
}

bmcl::OptionPtr<CmdNode> QCmdModel::decodeQCmdModelDrop(const QMimeData* data, const QModelIndex& parent)
{
    bmcl::OptionPtr<Node> node = unpackMimeData(data, _mimeStr);
    if (node.isNone()) {
        return bmcl::None;
    }

    CmdNode* cmdNode = dynamic_cast<CmdNode*>(node.unwrap());
    if (!cmdNode) {
        return bmcl::None;
    }

    if (!cmdNode->hasParent()) {
        return bmcl::None;
    }

    if (cmdNode->parent().unwrap() != parent.internalPointer()) {
        return bmcl::None;
    }

    return cmdNode;
}

bool QCmdModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    if (parent.internalPointer() != _cmds.get()) {
        return false;
    }

    if (decodeQModelDrop(data).isSome()) {
        return true;
    }

    if (decodeQCmdModelDrop(data, parent).isSome()) {
        return true;
    }

    return false;
}

bool QCmdModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (parent.internalPointer() != _cmds.get()) {
        return false;
    }

    bmcl::OptionPtr<CmdNode> node = decodeQModelDrop(data);
    if (node.isSome()) {
        auto newNode = node.unwrap()->clone(_cmds.get());
        beginInsertRows(parent, _cmds->numChildren(), _cmds->numChildren());
        _cmds->addCmdNode(newNode.get());
        endInsertRows();
        emit cmdAdded(indexFromNode(newNode.get(), 0));
        return true;
    }

    bmcl::OptionPtr<CmdNode> cmdNode = decodeQCmdModelDrop(data, parent);
    if (cmdNode.isSome()) {
        if (_cmds->numChildren() <= 1) {
            return false;;
        }
        if (row == -1) {
            row = _cmds->numChildren() - 1;
        }
        if (row > 0 && std::size_t(row) >= _cmds->numChildren()) {
            row = _cmds->numChildren() - 1;
        }
        bmcl::Option<std::size_t> currentRow = _cmds->childIndex(cmdNode.unwrap());
        if (currentRow.isNone()) {
            return false;
        }
        beginMoveRows(parent, currentRow.unwrap(), currentRow.unwrap(), parent, row);
        _cmds->moveNode(currentRow.unwrap(), row);
        endMoveRows();
        return true;
    }

    return false;
}

Qt::DropActions QCmdModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions QCmdModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void QCmdModel::resizeNode(const QModelIndex& index, size_t size)
{
    Node* node = static_cast<Node*>(index.internalPointer());
    auto maxSize = node->canBeResized();
    if (maxSize.isNone())
        return;

    size_t currentSize = node->numChildren();

    if (size > *maxSize || currentSize == size)
        return;

    if (size > currentSize)
    {
        beginInsertRows(index, currentSize, size);
        node->resizeNode(size);
        endInsertRows();
    }
    else
    {
        beginRemoveRows(index, size, currentSize);
        node->resizeNode(size);
        endRemoveRows();
    }
}

void QCmdModel::reset()
{
    if(_cmds.get()) {
        beginResetModel();
        _cmds.get()->clear();
        endResetModel();
    }
}
}
