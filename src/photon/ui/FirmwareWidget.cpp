/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/ui/FirmwareWidget.h"
#include "photon/model/CmdNode.h"
#include "photon/ui/QCmdModel.h"
#include "photon/ui/QNodeModel.h"
#include "photon/ui/QNodeViewModel.h"
#include "photon/model/CmdModel.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/model/CoderState.h"

#include "photongen/groundcontrol/Validator.hpp"
#include "photongen/groundcontrol/pvu/Script.hpp"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>
#include <bmcl/Uuid.h>

namespace photon {

static constexpr const int cmdTimeoutMs = 5000;

class TreeSortFilterProxyModel : public QSortFilterProxyModel {
public:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        if (filterAcceptsRowItself(sourceRow, sourceParent)) {
            return true;
        }

        QModelIndex parent = sourceParent;

        while (parent.isValid()) {
            if (filterAcceptsRowItself(parent.row(), parent.parent())) {
                return true;
            }
            parent = parent.parent();
        }

        return hasAcceptedChildren(sourceRow, sourceParent);
    }

    bool hasAcceptedChildren(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex item = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!item.isValid()) {
            return false;
        }

        int childCount = item.model()->rowCount(item);
        if (childCount == 0) {
            return false;
        }

        for (int i = 0; i < childCount; ++i) {
            if (filterAcceptsRowItself(i, item)) {
                return true;
            }
            if (hasAcceptedChildren(i, item)) {
                return true;
            }
        }
        return false;
    }

    bool filterAcceptsRowItself(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex infoIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        return sourceModel()->data(infoIndex).toString().contains(filterRegExp());
    }
};

void FirmwareWidget::updateButtonsAfterTabSwitch(int index)
{
    if (index == 0) {
        _sendOneButton->setHidden(false);
        updateButtonsFromSelection(_scriptEditWidget->selectionModel()->selection());
    } else {
        _sendOneButton->setHidden(true);
        updateButtonsFromSelection(_pvuScriptEditWidget->selectionModel()->selection());
    }
}

void FirmwareWidget::updateButtonsFromSelection(const QItemSelection& selection)
{
    for (const QModelIndex& idx : selection.indexes()) {
        if (QCmdModel::isCmdIndex(idx)) {
            _selectedCmd.emplace(idx);
            _removeButton->setEnabled(true);
            _sendOneButton->setEnabled(true);
            return;
        }
    }
    _selectedCmd.clear();
    _removeButton->setEnabled(false);
    _sendOneButton->setEnabled(false);
}

void FirmwareWidget::updateButtons()
{
    auto selection = _pvuScriptEditWidget->selectionModel()->selection();
    selection += _scriptEditWidget->selectionModel()->selection();

    updateButtonsFromSelection(selection);
}

void FirmwareWidget::expandSubtree(const QAbstractItemModel* model, const QModelIndex& idx, QTreeView* view)
{
    view->expand(idx);
    int numRows = model->rowCount(idx);
    for (int i = 0; i < numRows; i++) {
        QModelIndex child = model->index(i, 0, idx);
        expandSubtree(model, child, view);
    }
}

static void removeCmdFrom(const QTreeView* view, QCmdModel* model)
{
    auto idxs = view->selectionModel()->selection().indexes();
    if (idxs.isEmpty()) {
        return;
    }
    model->removeCmd(idxs[0]);
}

void FirmwareWidget::removeCurrentCmd()
{
    int i = _krlPvuTabWidget->currentIndex();
    if (i == 0) {
        removeCmdFrom(_scriptEditWidget, _scriptEditModel.get());
    } else {
        removeCmdFrom(_pvuScriptEditWidget, _pvuScriptEditModel.get());
    }
    updateButtonsAfterTabSwitch(i);
}

PacketRequest FirmwareWidget::createCmdRequest(const bmcl::Buffer& data)
{
    _cmdUuid = bmcl::Uuid::create();
    return PacketRequest(_cmdUuid, data, StreamType::Cmd);
}

FirmwareWidget::FirmwareWidget(std::unique_ptr<QNodeViewModel>&& paramView,
                               std::unique_ptr<QNodeViewModel>&& eventView,
                               std::unique_ptr<QNodeViewModel>&& statsView,
                               QWidget* parent)
    : QWidget(parent)
    , _sendTimer(new QTimer)
    , _cmdUuid(bmcl::Uuid::create())
{
    Rc<Node> emptyNode = new Node(bmcl::None);
    _paramViewModel = std::move(paramView);
    _eventViewModel = std::move(eventView);
    _statsViewModel = std::move(statsView);

    auto buttonLayout = new QHBoxLayout;
    _removeButton = new QPushButton("Remove");
    _removeButton->setDisabled(true);
    connect(_removeButton, &QPushButton::clicked, this, &FirmwareWidget::removeCurrentCmd);
    auto clearButton = new QPushButton("Clear");
    auto sendButton = new QPushButton("Send");
    _sendOneButton = new QPushButton("Send one cmd");
    _autoScrollBox = new QCheckBox("Autoscroll events");
    _autoScrollBox->setCheckState(Qt::Checked);
    buttonLayout->setDirection(QBoxLayout::LeftToRight);
    buttonLayout->addWidget(_sendOneButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(_removeButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch();

    _krlPvuTabWidget = new QTabWidget();
    connect(_krlPvuTabWidget, &QTabWidget::currentChanged, this, &FirmwareWidget::updateButtonsAfterTabSwitch);

    _scriptNode.reset(new ScriptNode(bmcl::None));
    connect(sendButton, &QPushButton::clicked, this, [this]() {
        if (_krlPvuTabWidget->currentIndex() == 0) {
            bmcl::Buffer dest;
            dest.reserve(2048);
            CoderState ctx(OnboardTime::now());
            if (_scriptNode->encode(&ctx, &dest)) {
                PacketRequest req = createCmdRequest(dest);
                setEnabled(false);
                _sendTimer->start(cmdTimeoutMs);
                emit reliablePacketQueued(req);
            }
            else {
                QString err = "Error while encoding script: ";
                err.append(ctx.error().c_str());
                QMessageBox::warning(this, "FirmwareWidget", err, QMessageBox::Ok);
            }
        } else {
            bmcl::Buffer dest;
            dest.reserve(1024);
            CoderState ctx(OnboardTime::now());
            if (!_pvuScriptNode->encode(&ctx, &dest)) {
                QString err = "Error while encoding pvu script: ";
                err.append(ctx.error().c_str());
                QMessageBox::warning(this, "FirmwareWidget", err, QMessageBox::Ok);
                return;
            }
            sendPvuScriptCommand(_pvuScriptNameWidget->text().toStdString(), _autoremovePvuScript->isChecked(), _autostartPvuScript->isChecked(), dest);
        }
    });

    connect(_sendOneButton, &QPushButton::clicked, this, [this]() {
        if (_krlPvuTabWidget->currentIndex() != 0) {
            return;
        }
        if (_selectedCmd.isNone()) {
            return;
        }
        bmcl::Buffer dest;
        dest.reserve(128);
        CoderState ctx(OnboardTime::now());
        const CmdNode* node = static_cast<const CmdNode*>(_selectedCmd.unwrap().internalPointer());
        if (node->encode(&ctx, &dest)) {
            PacketRequest req = createCmdRequest(dest);
            setEnabled(false);
            _sendTimer->start(cmdTimeoutMs);
            emit reliablePacketQueued(req);
        } else {
            QString err = "Error while encoding single cmd: ";
            err.append(ctx.error().c_str());
            QMessageBox::warning(this, "FirmwareWidget", err, QMessageBox::Ok);
        }
    });

    _scriptEditModel = bmcl::makeUnique<QCmdModel>(_scriptNode.get());
    _scriptEditModel->setEditable(true);
    _scriptEditWidget = new QTreeView();
    _scriptEditWidget->setModel(_scriptEditModel.get());
    _scriptEditWidget->setItemDelegate(new QModelItemDelegate<Node>(_scriptEditWidget));
    _scriptEditWidget->setAlternatingRowColors(true);
    _scriptEditWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _scriptEditWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _scriptEditWidget->setDropIndicatorShown(true);
    _scriptEditWidget->setDragEnabled(true);
    _scriptEditWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _scriptEditWidget->viewport()->setAcceptDrops(true);
    _scriptEditWidget->setAcceptDrops(true);
    //_scriptEditWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //_scriptEditWidget->header()->setStretchLastSection(false);
    _scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
    _scriptEditWidget->expandToDepth(1);
    _scriptEditWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    _scriptEditWidget->setColumnHidden(3, true);
    connect(_scriptEditWidget, &QWidget::customContextMenuRequested, this, &FirmwareWidget::nodeContextMenuRequested);
    connect(_scriptEditModel.get(), &QCmdModel::cmdAdded, this, [this](const QModelIndex& idx) {
        expandSubtree(_scriptEditModel.get(), idx, _scriptEditWidget);
    });
    connect(_scriptEditWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FirmwareWidget::updateButtons);

    connect(clearButton, &QPushButton::clicked, this, [=]() {
        if (_krlPvuTabWidget->currentIndex() == 0) {
            _scriptEditModel->reset();
            _scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
            updateButtonsFromSelection(_scriptEditWidget->selectionModel()->selection());
        } else {
            _pvuScriptEditModel->reset();
            _pvuScriptEditWidget->setRootIndex(_pvuScriptEditModel->index(0, 0));
            updateButtonsFromSelection(_pvuScriptEditWidget->selectionModel()->selection());
        }
    });

    _cmdViewModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    _cmdViewProxyModel.reset(new TreeSortFilterProxyModel);
    _cmdViewProxyModel->setSourceModel(_cmdViewModel.get());
    _cmdViewProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    _cmdViewWidget = new QTreeView;
    _cmdViewWidget->setModel(_cmdViewProxyModel.get());
    _cmdViewWidget->setAlternatingRowColors(true);
    _cmdViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _cmdViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _cmdViewWidget->setDragEnabled(true);
    _cmdViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _cmdViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _cmdViewWidget->header()->setStretchLastSection(false);
    _cmdViewWidget->setRootIndex(_cmdViewProxyModel->index(0, 0));
    _cmdViewWidget->setColumnHidden(2, true);
    _cmdViewWidget->setColumnHidden(3, true);
    _cmdViewWidget->expandToDepth(1);

    _scriptResultModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    _scriptResultWidget = new QTreeView;
    _scriptResultWidget->setModel(_scriptResultModel.get());
    _scriptResultWidget->setAlternatingRowColors(true);
    _scriptResultWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _scriptResultWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _scriptResultWidget->setDragEnabled(true);
    _scriptResultWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _scriptResultWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _scriptResultWidget->header()->setStretchLastSection(false);
    _scriptResultWidget->setRootIndex(_scriptResultModel->index(0, 0));
    _scriptResultWidget->setColumnHidden(3, true);

    auto krlCmdWidget = new QSplitter(Qt::Vertical);
    krlCmdWidget->addWidget(_scriptEditWidget);
    krlCmdWidget->addWidget(_scriptResultWidget);

    _pvuScriptNode.reset(new ScriptNode(bmcl::None));
    _pvuScriptEditWidget = new QTreeView();
    _pvuScriptNameWidget = new QLineEdit();
    _pvuScriptNameWidget->setMaxLength(16);
    _pvuScriptNameWidget->setText("Script");

    _pvuScriptEditModel = bmcl::makeUnique<QCmdModel>(_pvuScriptNode.get());
    _pvuScriptEditModel->setEditable(true);
    _pvuScriptEditWidget->setModel(_pvuScriptEditModel.get());
    _pvuScriptEditWidget->setItemDelegate(new QModelItemDelegate<Node>(_pvuScriptEditWidget));
    _pvuScriptEditWidget->setAlternatingRowColors(true);
    _pvuScriptEditWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _pvuScriptEditWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _pvuScriptEditWidget->setDropIndicatorShown(true);
    _pvuScriptEditWidget->setDragEnabled(true);
    _pvuScriptEditWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _pvuScriptEditWidget->viewport()->setAcceptDrops(true);
    _pvuScriptEditWidget->setAcceptDrops(true);
    _pvuScriptEditWidget->setRootIndex(_pvuScriptEditModel->index(0, 0));
    _pvuScriptEditWidget->expandToDepth(1);
    _pvuScriptEditWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    _pvuScriptEditWidget->setColumnHidden(3, true);
    connect(_pvuScriptEditWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FirmwareWidget::updateButtons);
    connect(_pvuScriptEditModel.get(), &QCmdModel::cmdAdded, this, [this](const QModelIndex& idx) {
        expandSubtree(_pvuScriptEditModel.get(), idx, _pvuScriptEditWidget);
    });

    _autostartPvuScript = new QCheckBox("Auto remove");
    _autoremovePvuScript = new QCheckBox("Auto start");

    QWidget* pvuEditWidget = new QWidget();
    auto pvuEditLayout = new QGridLayout();
    pvuEditLayout->setMargin(0);
    pvuEditLayout->addWidget(new QLabel("Script Name:"), 0, 0, 1, 1);
    pvuEditLayout->addWidget(_pvuScriptNameWidget, 0, 1, 1, 1);
    pvuEditLayout->addWidget(_autoremovePvuScript, 0, 2, 1, 1);
    pvuEditLayout->addWidget(_autostartPvuScript, 0, 3, 1, 1);
    pvuEditLayout->addWidget(_pvuScriptEditWidget, 1, 0, 1, 4);

    pvuEditWidget->setLayout(pvuEditLayout);

    _krlPvuTabWidget->addTab(krlCmdWidget, "KRL");
    _krlPvuTabWidget->addTab(pvuEditWidget, "PVU");

    auto krlPvuTabLayout = new QVBoxLayout;
    krlPvuTabLayout->addWidget(_krlPvuTabWidget);
    krlPvuTabLayout->addLayout(buttonLayout);
    auto krlPvuWrapper = new QWidget;
    krlPvuWrapper->setLayout(krlPvuTabLayout);
    krlPvuTabLayout->setMargin(0);

    _cmdFilterEdit = new QLineEdit;

    auto cmdViewLayout = new QVBoxLayout;
    cmdViewLayout->setMargin(0);
    cmdViewLayout->addWidget(_cmdFilterEdit);
    cmdViewLayout->addWidget(_cmdViewWidget);
    auto cmdViewWrapper = new QWidget;
    cmdViewWrapper->setLayout(cmdViewLayout);

    auto rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(cmdViewWrapper);
    rightSplitter->addWidget(krlPvuWrapper);

    _paramViewWidget = new QTreeView;
    _paramViewWidget->setAcceptDrops(true);
    _paramViewWidget->setAlternatingRowColors(true);
    _paramViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _paramViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _paramViewWidget->setDragEnabled(true);
    _paramViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _paramViewWidget->setDropIndicatorShown(true);
    _paramViewWidget->header()->setStretchLastSection(false);
    _paramViewWidget->setModel(_paramViewModel.get());
    _paramViewWidget->header()->moveSection(2, 1);

    _statsViewWidget = new QTreeView;
    _statsViewWidget->setAcceptDrops(true);
    _statsViewWidget->setAlternatingRowColors(true);
    _statsViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _statsViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _statsViewWidget->setDragEnabled(true);
    _statsViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _statsViewWidget->setDropIndicatorShown(true);
    _statsViewWidget->header()->setStretchLastSection(false);
    _statsViewWidget->setModel(_statsViewModel.get());
    //_statsViewWidget->header()->moveSection(2, 1);
    _statsViewWidget->setColumnHidden(1, true);
    _statsViewWidget->setColumnHidden(4, true);

    _eventViewWidget = new QTreeView;
    _eventViewWidget->setAcceptDrops(false);
    _eventViewWidget->setAlternatingRowColors(true);
    _eventViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _eventViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _eventViewWidget->setDragEnabled(false);
    _eventViewWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
    _eventViewWidget->setDropIndicatorShown(false);
    _eventViewWidget->header()->setStretchLastSection(false);
    _eventViewWidget->setModel(_eventViewModel.get());
    _eventViewWidget->header()->moveSection(2, 1);
    _eventViewWidget->setRootIndex(_eventViewModel->index(0, 0));


    auto leftSplitter = new QSplitter(Qt::Vertical);
    leftSplitter->addWidget(_paramViewWidget);
    leftSplitter->addWidget(_eventViewWidget);

    auto paramStatsTabWidget = new QTabWidget;
    paramStatsTabWidget->addTab(leftSplitter, "Telemetry");
    paramStatsTabWidget->addTab(_statsViewWidget, "Statistics");

    auto autoscrollLayout = new QHBoxLayout;
    autoscrollLayout->addStretch();
    autoscrollLayout->addWidget(_autoScrollBox);
    auto leftLayout = new QVBoxLayout;
    leftLayout->setMargin(0);
    leftLayout->addWidget(paramStatsTabWidget);
    leftLayout->addLayout(autoscrollLayout);
    auto leftWrapper = new QWidget;
    leftWrapper->setLayout(leftLayout);

    connect(_scriptEditWidget, &QTreeView::expanded, _scriptEditWidget, [this]() {
        _scriptEditWidget->resizeColumnToContents(0);
    });
    connect(_paramViewWidget, &QTreeView::expanded, _paramViewWidget, [this]() {
        _paramViewWidget->resizeColumnToContents(0);
    });

    connect(_cmdFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        _cmdViewProxyModel->setFilterFixedString(text);
        if (text.isEmpty()) {
            _cmdViewWidget->expandToDepth(0);
        } else {
            _cmdViewWidget->expandAll();
        }
    });

    _sendTimer->setSingleShot(true);
    connect(_sendTimer.get(), &QTimer::timeout, this, [this]() {
        setEnabled(true);
        _cmdUuid = bmcl::Uuid::createNil();
        QMessageBox::warning(this, "Packet error", "Failed to send packet: timeout");
    });

    auto centralSplitter = new QSplitter(Qt::Horizontal);
    centralSplitter->addWidget(leftWrapper);
    centralSplitter->addWidget(rightSplitter);

    auto centralLayout = new QVBoxLayout;
    centralLayout->addWidget(centralSplitter);
    centralLayout->setMargin(0);
    setLayout(centralLayout);
}

FirmwareWidget::~FirmwareWidget()
{
}

void FirmwareWidget::acceptPacketResponse(const PacketResponse& response)
{
    if (response.requestUuid != _cmdUuid) {
        BMCL_WARNING() << "firmwarewidget recieved packet response with invalid uuid";
        return;
    }
    _sendTimer->stop();
    _cmdUuid = bmcl::Uuid::createNil();
    bmcl::MemReader reader(response.payload.view());
    _scriptResultNode = ScriptResultNode::fromScriptNode(_scriptNode.get(), _cache.get(), bmcl::None);
    //TODO: check errors
    CoderState ctx(response.tickTime);
    _scriptResultNode->decode(&ctx, &reader);
    _scriptResultModel->setRoot(_scriptResultNode.get());
    _scriptResultWidget->expandAll();
    setEnabled(true);
}

void FirmwareWidget::sendPvuScriptCommand(const std::string& name, bool autoremove, bool autostart, const bmcl::Buffer& scriptBuffer)
{
    photongen::pvu::ScriptDesc desc(name, autoremove, autostart);
    photongen::pvu::ScriptBody scriptBody;
    scriptBody.resize(scriptBuffer.size());
    memcpy(scriptBody.data(), scriptBuffer.data(), scriptBuffer.size());

    bmcl::Buffer dest;
    dest.reserve(1024);
    CoderState ctx(OnboardTime::now());
    if (_validator->encodeCmdPvuAddScript(desc, scriptBody, &dest, &ctx)) {
        PacketRequest req = createCmdRequest(dest);
        setEnabled(false);
        _sendTimer->start(cmdTimeoutMs);
        emit reliablePacketQueued(req);
        _pvuScriptEditModel->reset();
        _pvuScriptEditWidget->setRootIndex(_pvuScriptEditModel->index(0, 0));
    }
}

void FirmwareWidget::nodeContextMenuRequested(const QPoint& pos)
{
    auto index = _scriptEditWidget->indexAt(pos);
    if (!index.isValid())
        return;

    Node* cmdNode = static_cast<Node*>(index.internalPointer());
    auto maxSize = cmdNode->canBeResized();
    if (maxSize.isNone())
        return;

    QMenu contextMenu;
    auto resizeAction = contextMenu.addAction("&Resize...");

    if (contextMenu.exec(_scriptEditWidget->mapToGlobal(pos)) == resizeAction)
    {
        bool ok = false;
        int newSize = QInputDialog::getInt(this, "Resize array", "New array size:", cmdNode->numChildren(), 0, *maxSize, 1, &ok);
        if (!ok)
            return;
        _scriptEditModel->resizeNode(index, newSize);
    }
}

void FirmwareWidget::setRootTmNode(NodeView* statusView, NodeView* eventView, NodeView* statsView)
{
    _paramViewModel->setRoot(statusView);
    _paramViewWidget->expandToDepth(0);
    _eventViewModel->setRoot(eventView);
    _eventViewWidget->expandToDepth(0);
    _eventViewWidget->setRootIndex(_eventViewModel->index(0, 0));
    _statsViewModel->setRoot(statsView);
    _statsViewWidget->expandToDepth(0);
    _statsViewWidget->resizeColumnToContents(0);
}

void FirmwareWidget::setRootCmdNode(const ValueInfoCache* cache, Node* root)
{
    _cmdUuid = bmcl::Uuid::createNil();
    _cache.reset(cache);
    _cmdViewModel->setRoot(root);
    _cmdViewWidget->expandToDepth(0);
}

void FirmwareWidget::setValidator(const Rc<photongen::Validator>& validator)
{
    _validator = validator;
}

void FirmwareWidget::applyTmUpdates(NodeViewUpdater* statusUpdater, NodeViewUpdater* eventUpdater, NodeViewUpdater* statsUpdater)
{
    //TODO: remove viewport updates
    _paramViewModel->applyUpdates(statusUpdater);
    _paramViewWidget->viewport()->update();
    _eventViewModel->applyUpdates(eventUpdater);
    if (_autoScrollBox->isChecked()) {
        _eventViewWidget->scrollToBottom();
    }
    _eventViewWidget->viewport()->update();

    _statsViewModel->applyUpdates(statsUpdater);
    _statsViewWidget->viewport()->update();
}
}
