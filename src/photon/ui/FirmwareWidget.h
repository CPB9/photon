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

#include <bmcl/Fwd.h>
#include <bmcl/Bytes.h>

#include <QWidget>

#include <memory>

class QTreeView;

namespace photon {

class ScriptNode;
class QNodeModel;
class QNodeViewModel;
class QCmdModel;
class ScriptResultNode;
class Node;
class Model;
class NodeView;
class NodeViewUpdater;
class ValueInfoCache;
struct PacketRequest;
struct PacketResponse;

class FirmwareWidget : public QWidget {
    Q_OBJECT
public:
    FirmwareWidget(std::unique_ptr<QNodeViewModel>&& nodeView, QWidget* parent = nullptr);
    ~FirmwareWidget();

    void setRootTmNode(NodeView* root);
    void applyTmUpdates(NodeViewUpdater* updater);

    void setRootCmdNode(const ValueInfoCache* cache, Node* root);

    void acceptPacketResponse(const PacketResponse& response);

private slots:
    void nodeContextMenuRequested(const QPoint& pos);
signals:
    void unreliablePacketQueued(const PacketRequest& packet);
    void reliablePacketQueued(const PacketRequest& packet);

private:
    QTreeView* _paramViewWidget;
    QTreeView* _scriptEditWidget;
    QTreeView* _scriptResultWidget;
    QTreeView* _cmdViewWidget;
    Rc<ScriptNode> _scriptNode;
    Rc<ScriptResultNode> _scriptResultNode;
    Rc<const ValueInfoCache> _cache;
    std::unique_ptr<QNodeViewModel> _paramViewModel;
    std::unique_ptr<QCmdModel> _scriptEditModel;
    std::unique_ptr<QNodeModel> _cmdViewModel;
    std::unique_ptr<QNodeModel> _scriptResultModel;
};
}
