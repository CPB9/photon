/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/ui/QNodeModel.h"

namespace photon {

class ScriptNode;
class CmdNode;

class QCmdModel : public QNodeModel {
    Q_OBJECT
public:
    QCmdModel(ScriptNode* node);
    ~QCmdModel();

    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    QStringList mimeTypes() const override;

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    void reset();

    const ScriptNode* scriptNode() const
    {
        return _cmds.get();
    }

    void resizeNode(const QModelIndex& index, size_t size);
    void removeCmd(const QModelIndex& idx);

    static bool isCmdIndex(const QModelIndex& idx);

signals:
    void cmdAdded(const QModelIndex& idx);

private:
    static bmcl::OptionPtr<CmdNode> decodeQModelDrop(const QMimeData* data);
    static bmcl::OptionPtr<CmdNode> decodeQCmdModelDrop(const QMimeData* data, const QModelIndex& parent);

    Rc<ScriptNode> _cmds;
};

}
