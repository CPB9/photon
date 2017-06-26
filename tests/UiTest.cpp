#include "photon/Init.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"
#include "photon/int/Int.Component.h"
#include "photon/test/Test.Component.h"
#include "photon/exc/Exc.Component.h"

#include <decode/ui/QModel.h>
#include <decode/ui/QCmdModel.h>
#include <decode/ui/QModelEventHandler.h>
#include <decode/model/Model.h>
#include <decode/model/CmdNode.h>
#include <decode/parser/Package.h>
#include <decode/core/Diagnostics.h>
#include <decode/groundcontrol/GroundControl.h>
#include <decode/groundcontrol/Scheduler.h>
#include <decode/groundcontrol/Exchange.h>

#include <dtacan/Parser.h>
#include <dtacan/Encoder.h>

#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/MemReader.h>
#include <bmcl/MemWriter.h>

#include <tclap/CmdLine.h>

#include <QApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QDebug>
#include <QtSerialPort/QtSerialPort>
#include <QUdpSocket>
#include <QMessageBox>

using namespace decode;

class UiSched : public Scheduler {
public:
    void scheduleAction(StateAction action, std::chrono::milliseconds delay) override
    {
        QTimer::singleShot(delay.count(), action);
    }
};

class UiSink : public Sink {
public:
    virtual quint64 readData(void* dest, std::size_t size) = 0;
};

class RsSink : public UiSink {
public:
    RsSink(const char* path)
        : _port(new QSerialPort(path))
    {
        _port->setBaudRate(62500);
        _port->open(QIODevice::ReadWrite);
        qDebug() << _port->error();
    }

    ~RsSink()
    {
        delete _port;
    }

    void sendData(bmcl::Bytes packet) override
    {
        qDebug() << "sending " << packet.size();
        _port->write((char*)packet.data(), packet.size());
    }

    quint64 readData(void* dest, std::size_t maxSize) override
    {
        return _port->read((char*)dest, maxSize);
    }

private:
    QSerialPort* _port;
};

class DtaCanSink : public UiSink, public dtacan::Parser<DtaCanSink>, public dtacan::Encoder<DtaCanSink> {
public:
    DtaCanSink(uint32_t address, const char* path)
        : _port(new QSerialPort(path))
        , _address(address)
      //  , _dataSent(false)
    {
        _port->setBaudRate(115200);
        _port->open(QIODevice::ReadWrite);
        qDebug() << _port->error();
        openCanChannel();
        setBaudrate(dtacan::BaudRate::Baud1M);
        readAndParse();
       // _dataSent = false;
    }

    ~DtaCanSink()
    {
        delete _port;
    }

    void sendData(bmcl::Bytes packet) override
    {
        openCanChannel();
        qDebug() << "sending " << packet.size();
        for (auto it = packet.begin(); it < packet.end(); it += 8) {
            auto end = std::min(it + 8, packet.end());
            transmitStdFrame(_address, it, end - it);
            readAndParse();
        }
    }

    quint64 readData(void* dest, std::size_t maxSize) override
    {
        readAndParse();
        std::size_t size = std::min(maxSize, _recievedData.size());
        std::memcpy(dest, _recievedData.data(), size);
        _recievedData.removeFront(size);
        return size;
    }

    void handleData(uint32_t address, const uint8_t* data, std::size_t size)
    {
        if (address != _address) {
            BMCL_DEBUG() << "expected addr " << _address << " got " << address;
            return;
        }
        _recievedData.write(data, size);
    }

    void handleJunk(const uint8_t* junk, std::size_t size)
    {
        BMCL_DEBUG() << "recieved junk " << size;
    }

    void handleEncodedData(const char* str, std::size_t size)
    {
        _port->write(str, size);
        //_port->flush();
    }

    void handleReceipt()
    {
        //_dataSent = true;
    }

    void readAndParse()
    {
        char tmp[102400];
        auto size = _port->read(tmp, sizeof(tmp));
        assert(size >= 0);
        acceptData(tmp, size);
    }

private:

    QSerialPort* _port;
    uint32_t _address;
    bmcl::Buffer _recievedData;
   // bool _dataSent;
};


class UpdSink : public UiSink {
public:
    UpdSink()
        : _sock(new QUdpSocket)
    {
        //_sock->setBaudRate(QSerialPort::Baud115200);
        QHostAddress addr;
        addr.setAddress("127.0.0.1");
        _sock->connectToHost(addr, 6000);
        qDebug() << _sock->error();
    }

    ~UpdSink()
    {
        delete _sock;
    }

    void sendData(bmcl::Bytes packet) override
    {
        QHostAddress addr;
        addr.setAddress("127.0.0.1");
        _sock->writeDatagram((char*)packet.data(), packet.size(), addr, 6000);
    }

    quint64 readData(void* dest, std::size_t maxSize) override
    {
        _sock->waitForReadyRead();
        return _sock->readDatagram((char*)dest, maxSize);
    }

private:
    QUdpSocket* _sock;
};

class InProcSink : public UiSink {
public:
    InProcSink()
    {
        Photon_Init();
        _current = *PhotonExc_GetMsg();
    }

    void sendData(bmcl::Bytes packet) override
    {
        for (uint8_t byte : packet) {
            PhotonExc_AcceptInput(&byte, 1);
        }
    }

    quint64 readData(void* dest, std::size_t maxSize) override
    {
        if (maxSize >= _current.size) {
            std::size_t size = _current.size;
            std::memcpy(dest, _current.data, size);
            PhotonExc_PrepareNextMsg();
            _current = *PhotonExc_GetMsg();
            return size;
        }
        std::memcpy(dest, _current.data, maxSize);
        _current.data += maxSize;
        _current.size -= maxSize;
        return maxSize;
    }

private:
    PhotonExcMsg _current;
};

class UdpSink : public Sink {
public:
};

std::unique_ptr<QWidget> _w;
std::unique_ptr<QModel> _qmodel = bmcl::makeUnique<QModel>(new Node(bmcl::None));
Rc<Model> _m;
Rc<QModelEventHandler> _handler = new QModelEventHandler;
Rc<UiSink> _sink;
Rc<UiSched> _sched = new UiSched;

class Gc : public GroundControl {
public:
    using GroundControl::GroundControl;

    void updateModel(Model* model) override
    {
        _m.reset(model);
        _qmodel->setRoot(model);
        _w->showMaximized();
    }
};

Rc<Gc> _gc;

uint8_t tmp[10240];
uint8_t tmp2[1024];

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("UiTest");
    TCLAP::ValueArg<std::string> comArg("c", "com", "Read from COM", false, "", "com device");
    TCLAP::ValueArg<std::string> dtaArg("d", "dta", "Read from DTACAN", false, "", "com device");
    TCLAP::ValueArg<uint32_t> addrArg("a", "address", "CAN address", false, 0, "can address");
    TCLAP::SwitchArg inprocArg("i", "inproc", "Read from INPROC", false);

    std::vector<TCLAP::Arg*>  xorVec;
    xorVec.push_back(&comArg);
    xorVec.push_back(&dtaArg);
    xorVec.push_back(&inprocArg);
    cmdLine.xorAdd(xorVec);
    cmdLine.add(&addrArg);
    cmdLine.parse(argc, argv);

    QApplication app(argc, argv);

    if (!comArg.getValue().empty()) {
        _sink.reset(new RsSink(comArg.getValue().c_str()));
    } else if (!dtaArg.getValue().empty()) {
        _sink.reset(new DtaCanSink(addrArg.getValue(), dtaArg.getValue().c_str()));
    } else if (inprocArg.getValue()) {
        _sink.reset(new InProcSink);
    } else {
        std::abort();
    }

    _w.reset(new QWidget);
    _qmodel->setEditable(true);
    _gc.reset(new Gc(_sink.get(), _sched.get(), _handler.get()));

    auto buttonLayout = new QVBoxLayout;
    auto sendButton = new QPushButton("send");
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch();

    auto rightLayout = new QHBoxLayout;
    auto cmdWidget = new QTreeView;
    Rc<CmdContainerNode> cmdCont = new CmdContainerNode(bmcl::None);
    QObject::connect(sendButton, &QPushButton::clicked, _qmodel.get(), [cmdCont]() {
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        uint8_t prefix[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        dest.write(prefix, sizeof(prefix));
        if (cmdCont->encode(_handler.get(), &dest)) {
            _gc->sendPacket(dest.writenData());
        } else {
            qDebug() << "error encoding";
            QMessageBox::warning(_w.get(), "UiTest", "Error while encoding cmd. Args may be empty", QMessageBox::Ok);
        }
    });

    auto resizeColumnsFn = [](QTreeView* view) {
        view->resizeColumnToContents(0);
    };

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
    cmdWidget->header()->setStretchLastSection(false);
    cmdWidget->setRootIndex(cmdModel->index(0, 0));

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
//    mainView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mainView->header()->setStretchLastSection(false);
    mainView->setModel(_qmodel.get());

    QObject::connect(cmdWidget, &QTreeView::expanded, _w.get(), [&]() { resizeColumnsFn(cmdWidget); });
    QObject::connect(mainView.get(), &QTreeView::expanded, _w.get(), [&]() { resizeColumnsFn(mainView.get()); });

    auto centralLayout = new QHBoxLayout;
    centralLayout->addWidget(mainView.get());
    centralLayout->addLayout(rightLayout);
    _w->setLayout(centralLayout);

    //_w->resize(1024, 768);
    //_w->showMaximized();

    std::unique_ptr<QTimer> timer = bmcl::makeUnique<QTimer>();

    QObject::connect(_handler.get(), &QModelEventHandler::nodeValueUpdated, _qmodel.get(), &QModel::notifyValueUpdate);
    QObject::connect(_handler.get(), &QModelEventHandler::nodesInserted, _qmodel.get(), &QModel::notifyNodesInserted);
    QObject::connect(_handler.get(), &QModelEventHandler::nodesRemoved, _qmodel.get(), &QModel::notifyNodesRemoved);

    QObject::connect(timer.get(), &QTimer::timeout, _w.get(), [&]() {
        auto rv = _sink->readData(tmp, sizeof(tmp));
        assert(rv >= 0);
        if (rv == 0) {
            qDebug() << "no data";
            return;
        } else {
            qDebug() << "recieved " << rv;
        }
        _gc->acceptData(bmcl::Bytes(tmp, rv));
    });
    timer->start(100);
    return app.exec();
}
