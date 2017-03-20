#include "photon/fwt/Fwt.Component.h"

#include <decode/ui/QModel.h>
#include <decode/model/Model.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>

#include <bmcl/Result.h>

#include <QApplication>
#include <QTreeView>

using namespace decode;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    auto firmware = PhotonFwt_Firmware();
    Rc<Diagnostics> diag = new Diagnostics;
    auto rv = Package::decodeFromMemory(diag, firmware.data, firmware.size);
    assert(rv.isOk());
    Rc<Package> package = rv.take();
    Rc<Model> model = new Model(package.get());
    std::unique_ptr<QModel> qmodel = bmcl::makeUnique<QModel>(model.get());
    QTreeView w;
    w.setModel(qmodel.get());
    w.show();
    return app.exec();
}
