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
class QCheckBox;
class QLineEdit;
class QAbstractItemModel;

namespace photongen {
class Validator;
}

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
    FirmwareWidget(std::unique_ptr<QNodeViewModel>&& paramView,
                   std::unique_ptr<QNodeViewModel>&& eventView,
                   QWidget* parent = nullptr);
    ~FirmwareWidget();

    void setRootTmNode(NodeView* statusView, NodeView* eventView);
    void applyTmUpdates(NodeViewUpdater* statusUpdater, NodeViewUpdater* eventUpdater);

    void setRootCmdNode(const ValueInfoCache* cache, Node* root);
    void setValidator(const Rc<photongen::Validator>& validator);

    void acceptPacketResponse(const PacketResponse& response);
private:
    void sendPvuScriptCommand(const std::string& name, bool autoremove, bool autostart, const bmcl::Buffer& scriptBuffer);

private slots:
    void nodeContextMenuRequested(const QPoint& pos);
    void expandSubtree(const QAbstractItemModel* model, const QModelIndex& idx, QTreeView* view);

signals:
    void unreliablePacketQueued(const PacketRequest& packet);
    void reliablePacketQueued(const PacketRequest& packet);

private:

    QTreeView* _paramViewWidget;
    QTreeView* _eventViewWidget;
    QTreeView* _scriptEditWidget;
    QTreeView* _scriptResultWidget;
    QTreeView* _cmdViewWidget;

    QTreeView* _pvuScriptEditWidget;
    QLineEdit* _pvuScriptNameWidget;
    QCheckBox* _autoremovePvuScript;
    QCheckBox* _autostartPvuScript;

    QCheckBox* _autoScrollBox;

    Rc<const photongen::Validator> _validator;

    Rc<ScriptNode> _scriptNode;
    Rc<ScriptNode> _pvuScriptNode;
    Rc<ScriptResultNode> _scriptResultNode;
    Rc<const ValueInfoCache> _cache;
    std::unique_ptr<QNodeViewModel> _paramViewModel;
    std::unique_ptr<QNodeViewModel> _eventViewModel;
    std::unique_ptr<QCmdModel> _scriptEditModel;
    std::unique_ptr<QCmdModel> _pvuScriptEditModel;
    std::unique_ptr<QNodeModel> _cmdViewModel;
    std::unique_ptr<QNodeModel> _scriptResultModel;
};
}
