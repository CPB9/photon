#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"
#include "photon/test/Test.Component.h"

#include <decode/ui/QModel.h>
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
    Rc<Diagnostics> diag = new Diagnostics;
    auto rv = Package::decodeFromMemory(diag, firmware.data, firmware.size);
    assert(rv.isOk());
    Rc<Package> package = rv.take();
    Rc<Model> model = new Model(package.get());
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
    QObject::connect(timer.get(), &QTimer::timeout, &w, [model, &w]() {
        _test.param1++;
        _test.param2++;
        _test.param3++;
        _test.param4++;
        PhotonWriter writer;
        PhotonWriter_Init(&writer, tmp, sizeof(tmp));
        PhotonTm_CollectMessages(&writer);
        model->acceptTelemetry(bmcl::ArrayView<uint8_t>(writer.start, writer.current));
    });
    timer->start(std::chrono::milliseconds(1000));
    return app.exec();
}
