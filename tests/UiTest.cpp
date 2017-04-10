#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"
#include "photon/test/Test.Component.h"

#include <decode/ui/QModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/model/Model.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>

#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <QApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QTimer>

using namespace decode;

uint8_t tmp[1024];

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
    QTreeView w;
    w.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    w.header()->setStretchLastSection(false);
    w.setModel(qmodel.get());
    w.showMaximized();
    std::unique_ptr<QTimer> timer = bmcl::makeUnique<QTimer>();
    _test.param1 = 1;
    _test.param2 = 2;
    _test.param3 = 3;
    _test.param4 = 4;
    uint8_t testData[5] = {0, 1, 2, 3, 4};
    _test.param5.data = testData;
    _test.param5.size = 0;

    QObject::connect(handler.get(), &QModelEventHandler::nodeValueUpdated, qmodel.get(), &QModel::notifyValueUpdate);
    QObject::connect(handler.get(), &QModelEventHandler::nodesInserted, qmodel.get(), &QModel::notifyNodesInserted);
    QObject::connect(handler.get(), &QModelEventHandler::nodesRemoved, qmodel.get(), &QModel::notifyNodesRemoved);

    QObject::connect(timer.get(), &QTimer::timeout, &w, [&, model]() {
        _test.param1++;
        _test.param2++;
        _test.param3++;
        _test.param4++;
        _test.param5.size++;
        for (int i = 0; i < 5; i++) {
            testData[i]++;
        }
        if (_test.param5.size > 5) {
            _test.param5.size = 0;
        }
        PhotonWriter writer;
        PhotonWriter_Init(&writer, tmp, sizeof(tmp));
        PhotonTm_CollectMessages(&writer);
        model->acceptTelemetry(bmcl::ArrayView<uint8_t>(writer.start, writer.current));
        //w.viewport()->update();
    });
    timer->start(std::chrono::milliseconds(200));
    return app.exec();
}
