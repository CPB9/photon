#include "UiTest.h"

#include <decode/ui/QModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/ui/FirmwareWidget.h>
#include <decode/model/Model.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>
#include <decode/groundcontrol/GroundControl.h>
#include <decode/groundcontrol/Scheduler.h>
#include <decode/groundcontrol/Exchange.h>

#include <QApplication>
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

    QObject::connect(handler.get(), &QModelEventHandler::nodeValueUpdated, w->qmodel(), &QModel::notifyValueUpdate);
    QObject::connect(handler.get(), &QModelEventHandler::nodesInserted, w->qmodel(), &QModel::notifyNodesInserted);
    QObject::connect(handler.get(), &QModelEventHandler::nodesRemoved, w->qmodel(), &QModel::notifyNodesRemoved);
    QObject::connect(handler.get(), &QModelEventHandler::modelUpdated, w.get(), [&w](const Rc<Model>& model) {
        w->setModel(model.get());
        w->showMaximized();
    });
    QObject::connect(handler.get(), &QModelEventHandler::packetQueued, w.get(), [gc](bmcl::Bytes packet) {
        gc->sendPacket(packet);
    });

    gc->start();

    return app->exec();
}

