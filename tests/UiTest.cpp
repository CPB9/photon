#include "UiTest.h"

#include <decode/ui/QModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/ui/FirmwareWidget.h>
#include <decode/ui/FirmwareStatusWidget.h>
#include <decode/model/Model.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>
#include <decode/groundcontrol/GroundControl.h>
#include <decode/groundcontrol/Scheduler.h>
#include <decode/groundcontrol/Exchange.h>

#include <bmcl/Logging.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>

#include <chrono>

using namespace decode;

class QSched : public Scheduler {
public:
    void scheduleAction(StateAction action, std::chrono::milliseconds delay) override
    {
        QTimer::singleShot(delay.count(), action);
    }
};

int runUiTest(QApplication* app, DataStream* stream)
{
    Rc<QSched> sched = new QSched;
    Rc<QModelEventHandler> handler = new QModelEventHandler;
    Rc<GroundControl> gc = new GroundControl(stream, sched.get(), handler.get());
    std::unique_ptr<FirmwareWidget> w = bmcl::makeUnique<FirmwareWidget>(handler.get());
    std::unique_ptr<FirmwareStatusWidget> fwStatusWidget = bmcl::makeUnique<FirmwareStatusWidget>();

    fwStatusWidget->resize(640, 300);
    fwStatusWidget->move(app->desktop()->screen()->rect().center() - fwStatusWidget->rect().center());
    fwStatusWidget->show();

    QObject::connect(stream, &DataStream::readyRead, stream, [&]() {
        int64_t size = stream->bytesAvailable();
        if (size > 0) {
            uint8_t* data = new uint8_t[size];
            int64_t readData = stream->readData(data, size);
            if (readData > 0) {
                gc->acceptData(bmcl::Bytes(data, readData));
            } else {
                BMCL_DEBUG() << "no data to read";
            }
            delete [] data;
        } else {
            BMCL_DEBUG() << "no data available";
        }
    });

    QObject::connect(handler.get(), &QModelEventHandler::beginHashDownload, fwStatusWidget.get(), [&]() {
        fwStatusWidget->beginHashDownload();
    });
    QObject::connect(handler.get(), &QModelEventHandler::endHashDownload, fwStatusWidget.get(), &FirmwareStatusWidget::endHashDownload);
    QObject::connect(handler.get(), &QModelEventHandler::beginFirmwareStartCommand, fwStatusWidget.get(), &FirmwareStatusWidget::beginFirmwareStartCommand);
    QObject::connect(handler.get(), &QModelEventHandler::endFirmwareStartCommand, fwStatusWidget.get(), &FirmwareStatusWidget::endFirmwareStartCommand);
    QObject::connect(handler.get(), &QModelEventHandler::beginFirmwareDownload, fwStatusWidget.get(), &FirmwareStatusWidget::beginFirmwareDownload);
    QObject::connect(handler.get(), &QModelEventHandler::firmwareError, fwStatusWidget.get(), &FirmwareStatusWidget::firmwareError);
    QObject::connect(handler.get(), &QModelEventHandler::firmwareDownloadProgress, fwStatusWidget.get(), &FirmwareStatusWidget::firmwareDownloadProgress);
    QObject::connect(handler.get(), &QModelEventHandler::endFirmwareDownload, fwStatusWidget.get(), &FirmwareStatusWidget::endFirmwareDownload);

    QObject::connect(handler.get(), &QModelEventHandler::nodeValueUpdated, w->qmodel(), &QModel::notifyValueUpdate);
    QObject::connect(handler.get(), &QModelEventHandler::nodesInserted, w->qmodel(), &QModel::notifyNodesInserted);
    QObject::connect(handler.get(), &QModelEventHandler::nodesRemoved, w->qmodel(), &QModel::notifyNodesRemoved);
    QObject::connect(handler.get(), &QModelEventHandler::modelUpdated, w.get(), [&](const Rc<Model>& model) {
        w->setModel(model.get());
        fwStatusWidget->hide();
        w->showMaximized();
    });
    QObject::connect(handler.get(), &QModelEventHandler::packetQueued, w.get(), [gc](bmcl::Bytes packet) {
        gc->sendPacket(packet);
    });

    gc->start();

    return app->exec();
}

