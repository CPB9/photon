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
#include "photon/model/NodeView.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/model/CoderState.h"

#include "photongen/groundcontrol/Validator.hpp"
#include "photongen/groundcontrol/pvu/Script.hpp"

#include <QWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QTreeView>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QCheckBox>
#include <QTabWidget>
#include <QLabel>

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

namespace photon {

void FirmwareWidget::updateButtonsAfterTabSwitch(int index)
{
    if (index == 0) {
        updateButtonsFromSelection(_scriptEditWidget->selectionModel()->selection());
    } else {
        updateButtonsFromSelection(_pvuScriptEditWidget->selectionModel()->selection());
    }
}

void FirmwareWidget::updateButtonsFromSelection(const QItemSelection& selection)
{
    for (const QModelIndex& idx : selection.indexes()) {
        if (QCmdModel::isCmdIndex(idx)) {
            _removeButton->setEnabled(true);
            return;
        }
    }
    _removeButton->setEnabled(false);
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
    int i = _tabWidget->currentIndex();
    if (i == 0) {
        removeCmdFrom(_scriptEditWidget, _scriptEditModel.get());
    } else {
        removeCmdFrom(_pvuScriptEditWidget, _pvuScriptEditModel.get());
    }
    updateButtonsAfterTabSwitch(i);
}

FirmwareWidget::FirmwareWidget(std::unique_ptr<QNodeViewModel>&& paramView,
                               std::unique_ptr<QNodeViewModel>&& eventView,
                               QWidget* parent)
    : QWidget(parent)
{
    Rc<Node> emptyNode = new Node(bmcl::None);
    _paramViewModel = std::move(paramView);
    _eventViewModel = std::move(eventView);

    auto buttonLayout = new QHBoxLayout;
    _removeButton = new QPushButton("Remove");
    _removeButton->setDisabled(true);
    connect(_removeButton, &QPushButton::clicked, this, &FirmwareWidget::removeCurrentCmd);
    auto clearButton = new QPushButton("Clear");
    auto sendButton = new QPushButton("Send");
    _autoScrollBox = new QCheckBox("Autoscroll events");
    _autoScrollBox->setCheckState(Qt::Checked);
    buttonLayout->setDirection(QBoxLayout::LeftToRight);
    buttonLayout->addWidget(_autoScrollBox);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(_removeButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch();

    _tabWidget = new QTabWidget();
    connect(_tabWidget, &QTabWidget::currentChanged, this, &FirmwareWidget::updateButtonsAfterTabSwitch);

    _scriptNode.reset(new ScriptNode(bmcl::None));
    QObject::connect(sendButton, &QPushButton::clicked, _paramViewModel.get(), [this]() {
        if (_tabWidget->currentIndex() == 0) {
            bmcl::Buffer dest;
            dest.reserve(2048);
            CoderState ctx(OnboardTime::now());
            if (_scriptNode->encode(&ctx, &dest)) {
                PacketRequest req;
                req.streamType = StreamType::Cmd;
                req.payload = bmcl::SharedBytes::create(dest);
                setEnabled(false);
                emit reliablePacketQueued(req);
            }
            else {
                QString err = "Error while encoding cmd: ";
                err.append(ctx.error().c_str());
                QMessageBox::warning(this, "UiTest", err, QMessageBox::Ok);
            }
        }
        else {
            bmcl::Buffer dest;
            dest.reserve(1024);
            CoderState ctx(OnboardTime::now());
            if (!_pvuScriptNode->encode(&ctx, &dest)) {
                QString err = "Error while encoding cmd: ";
                err.append(ctx.error().c_str());
                QMessageBox::warning(this, "UiTest", err, QMessageBox::Ok);
                return;
            }
            sendPvuScriptCommand(_pvuScriptNameWidget->text().toStdString(), _autoremovePvuScript->isChecked(), _autostartPvuScript->isChecked(), dest);
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

    connect(clearButton, &QPushButton::clicked, this,
            [=]()
    {
        if (_tabWidget->currentIndex() == 0) {
            _scriptEditModel->reset();
            _scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
        } else {
            _pvuScriptEditModel->reset();
            _pvuScriptEditWidget->setRootIndex(_pvuScriptEditModel->index(0, 0));
        }
    });

    _cmdViewModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    _cmdViewWidget = new QTreeView;
    _cmdViewWidget->setModel(_cmdViewModel.get());
    _cmdViewWidget->setAlternatingRowColors(true);
    _cmdViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _cmdViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _cmdViewWidget->setDragEnabled(true);
    _cmdViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _cmdViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _cmdViewWidget->header()->setStretchLastSection(false);
    _cmdViewWidget->setRootIndex(_cmdViewModel->index(0, 0));
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

    QWidget* krlCmdWidget = new QWidget();
    QVBoxLayout* krlCmdLayout = new QVBoxLayout();
    krlCmdLayout->addWidget(_scriptEditWidget);
    krlCmdLayout->addWidget(_scriptResultWidget);
    krlCmdWidget->setLayout(krlCmdLayout);

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
    pvuEditLayout->addWidget(new QLabel("Script Name:"), 0, 0, 1, 1);
    pvuEditLayout->addWidget(_pvuScriptNameWidget, 0, 1, 1, 1);
    pvuEditLayout->addWidget(_autoremovePvuScript, 0, 2, 1, 1);
    pvuEditLayout->addWidget(_autostartPvuScript, 0, 3, 1, 1);
    pvuEditLayout->addWidget(_pvuScriptEditWidget, 1, 0, 1, 4);

    pvuEditWidget->setLayout(pvuEditLayout);

    _tabWidget->addTab(krlCmdWidget, "KRL");
    _tabWidget->addTab(pvuEditWidget, "PVU");

    auto rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->addWidget(_cmdViewWidget);
    rightSplitter->addWidget(_tabWidget);

    _paramViewWidget = new QTreeView;
    _paramViewWidget->setAcceptDrops(true);
    _paramViewWidget->setAlternatingRowColors(true);
    _paramViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _paramViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _paramViewWidget->setDragEnabled(true);
    _paramViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _paramViewWidget->setDropIndicatorShown(true);
    //_paramViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _paramViewWidget->header()->setStretchLastSection(false);
    _paramViewWidget->setModel(_paramViewModel.get());
    _paramViewWidget->header()->moveSection(2, 1);

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

    QObject::connect(_scriptEditWidget, &QTreeView::expanded, _scriptEditWidget, [this]() {
        _scriptEditWidget->resizeColumnToContents(0);
    });
    QObject::connect(_paramViewWidget, &QTreeView::expanded, _paramViewWidget, [this]() {
        _paramViewWidget->resizeColumnToContents(0);
    });

    auto centralSplitter = new QSplitter(Qt::Horizontal);
    centralSplitter->addWidget(leftSplitter);
    centralSplitter->addWidget(rightSplitter);

    auto centralLayout = new QVBoxLayout;
    centralLayout->addWidget(centralSplitter);
    centralLayout->addLayout(buttonLayout);
    setLayout(centralLayout);
}

FirmwareWidget::~FirmwareWidget()
{
}

void FirmwareWidget::acceptPacketResponse(const PacketResponse& response)
{
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
    std::vector<char> charName;
    charName.resize(name.size());
    memcpy(charName.data(), name.data(), name.size());

    photongen::pvu::ScriptDesc desc(charName, autoremove, autostart);
    desc.setName(charName); //HACK
    photongen::pvu::ScriptBody scriptBody;
    scriptBody.resize(scriptBuffer.size());
    memcpy(scriptBody.data(), scriptBuffer.data(), scriptBuffer.size());

    bmcl::Buffer dest;
    dest.reserve(1024);
    CoderState ctx(OnboardTime::now());
    if (_validator->encodeCmdPvuAddScript(desc, scriptBody, &dest, &ctx))
    {
        PacketRequest req;
        req.streamType = StreamType::Cmd;
        req.payload = bmcl::SharedBytes::create(dest);
        setEnabled(false);
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

void FirmwareWidget::setRootTmNode(NodeView* statusView, NodeView* eventView)
{
    _paramViewModel->setRoot(statusView);
    _paramViewWidget->expandToDepth(0);
    _eventViewModel->setRoot(eventView);
    _eventViewWidget->expandToDepth(0);
    _eventViewWidget->setRootIndex(_eventViewModel->index(0, 0));
}

void FirmwareWidget::setRootCmdNode(const ValueInfoCache* cache, Node* root)
{
    _cmdViewModel->setRoot(root);
    _cmdViewWidget->expandToDepth(0);
    _cache.reset(cache);
}

void FirmwareWidget::setValidator(const Rc<photongen::Validator>& validator)
{
    _validator = validator;
}

void FirmwareWidget::applyTmUpdates(NodeViewUpdater* statusUpdater, NodeViewUpdater* eventUpdater)
{
    //TODO: remove viewport updates
    _paramViewModel->applyUpdates(statusUpdater);
    _paramViewWidget->viewport()->update();
    _eventViewModel->applyUpdates(eventUpdater);
    if (_autoScrollBox->isChecked()) {
        _eventViewWidget->scrollToBottom();
    }
    _eventViewWidget->viewport()->update();
}
}
