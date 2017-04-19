#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"
#include "photon/int/Int.Component.h"
#include "photon/test/Test.Component.h"

#include <decode/ui/QModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/model/Model.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>

#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/MemReader.h>
#include <bmcl/MemWriter.h>

#include <QApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QDebug>

using namespace decode;

uint8_t tmp[1024];
uint8_t tmp2[1024];

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    PhotonTm_Init();
    auto firmware = PhotonFwt_Firmware();
    BMCL_DEBUG() << "firmware size: " << firmware.size;
    Rc<Diagnostics> diag = new Diagnostics;
    auto rv = Package::decodeFromMemory(diag.get(), firmware.data, firmware.size);
    assert(rv.isOk());
    Rc<Package> package = rv.take();
    Rc<QModelEventHandler> handler = new QModelEventHandler;
    Rc<Model> model = new Model(package.get(), handler.get());
    BMCL_DEBUG() << model->numChildren();
    std::unique_ptr<QModel> qmodel = bmcl::makeUnique<QModel>(model.get());
    qmodel->setEditable(true);

    QWidget w;

    auto buttonLayout = new QHBoxLayout;
    auto sendButton = new QPushButton("send");
    buttonLayout->addStretch();
    buttonLayout->addWidget(sendButton);

    auto rightLayout = new QVBoxLayout;
    auto cmdWidget = new QTreeView;
    Rc<CmdContainerNode> cmdCont = new CmdContainerNode(bmcl::None);
    QObject::connect(sendButton, &QPushButton::clicked, qmodel.get(), [cmdCont, handler]() {
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        if (cmdCont->encode(handler.get(), &dest)) {
            PhotonReader src;
            PhotonReader_Init(&src, dest.start(), dest.sizeUsed());
            PhotonWriter results;
            PhotonWriter_Init(&results, tmp2, sizeof(tmp2));
            if (PhotonInt_ExecuteFrom(&src, &results) != PhotonError_Ok) {
                qDebug() << "error executing";
            }
        } else {
                qDebug() << "error encoding";
        }
    });
    auto cmdModel = bmcl::makeUnique<QCmdModel>(cmdCont.get());
    cmdModel->setEditable(true);
    cmdWidget->setModel(cmdModel.get());
    cmdWidget->setAlternatingRowColors(true);
    cmdWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    cmdWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    cmdWidget->setDropIndicatorShown(true);
    cmdWidget->setDragEnabled(true);
    cmdWidget->setDragDropMode(QAbstractItemView::DragDrop);
    cmdWidget->viewport()->setAcceptDrops(true);
    cmdWidget->setAcceptDrops(true);
    cmdWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    cmdWidget->header()->setStretchLastSection(false);
    cmdWidget->setRowHidden(0, cmdWidget->rootIndex(), true);
    rightLayout->addWidget(cmdWidget);
    rightLayout->addLayout(buttonLayout);

    auto mainView = bmcl::makeUnique<QTreeView>();
    mainView->setAcceptDrops(true);
    mainView->setAlternatingRowColors(true);
    mainView->setSelectionMode(QAbstractItemView::SingleSelection);
    mainView->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainView->setDragEnabled(true);
    mainView->setDragDropMode(QAbstractItemView::DragDrop);
    mainView->setDropIndicatorShown(true);
    mainView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mainView->header()->setStretchLastSection(false);
    mainView->setModel(qmodel.get());

    auto centralLayout = new QHBoxLayout;
    centralLayout->addWidget(mainView.get());
    centralLayout->addLayout(rightLayout);
    w.setLayout(centralLayout);

    w.showMaximized();

    std::unique_ptr<QTimer> timer = bmcl::makeUnique<QTimer>();

    QObject::connect(handler.get(), &QModelEventHandler::nodeValueUpdated, qmodel.get(), &QModel::notifyValueUpdate);
    QObject::connect(handler.get(), &QModelEventHandler::nodesInserted, qmodel.get(), &QModel::notifyNodesInserted);
    QObject::connect(handler.get(), &QModelEventHandler::nodesRemoved, qmodel.get(), &QModel::notifyNodesRemoved);

    QObject::connect(timer.get(), &QTimer::timeout, &w, [&, model]() {
        PhotonWriter writer;
        PhotonWriter_Init(&writer, tmp, sizeof(tmp));
        PhotonTm_CollectMessages(&writer);
        model->acceptTelemetry(bmcl::Bytes(writer.start, writer.current));
        //w.viewport()->update();
    });
    timer->start(std::chrono::milliseconds(100));
    return app.exec();
}
