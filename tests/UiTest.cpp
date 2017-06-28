#include "photon/Init.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"
#include "photon/int/Int.Component.h"
#include "photon/test/Test.Component.h"
#include "photon/exc/Exc.Component.h"

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

class QSched : public Scheduler {
public:
    void scheduleAction(StateAction action, std::chrono::milliseconds delay) override
    {
        QTimer::singleShot(delay.count(), action);
    }
};

class RsSink : public DataStream {
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

    int64_t readData(void* dest, std::size_t maxSize) override
    {
        return _port->read((char*)dest, maxSize);
    }

private:
    QSerialPort* _port;
};

class UpdSink : public DataStream {
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

    int64_t readData(void* dest, std::size_t maxSize) override
    {
        _sock->waitForReadyRead();
        return _sock->readDatagram((char*)dest, maxSize);
    }

private:
    QUdpSocket* _sock;
};

class InProcSink : public DataStream {
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

    int64_t readData(void* dest, std::size_t maxSize) override
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

class UdpSink : public DataSink {
public:
};

std::unique_ptr<FirmwareWidget> _w;
Rc<Model> _m;
Rc<QModelEventHandler> _handler = new QModelEventHandler;
Rc<DataStream> _stream;
Rc<QSched> _sched = new QSched;

Rc<GroundControl> _gc;

uint8_t tmp[10240];
uint8_t tmp2[1024];

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("UiTest");
    TCLAP::ValueArg<std::string> comArg("c", "com", "Read from COM", false, "", "com device");
    TCLAP::ValueArg<uint32_t> addrArg("a", "address", "CAN address", false, 0, "can address");
    TCLAP::SwitchArg inprocArg("i", "inproc", "Read from INPROC", false);

    std::vector<TCLAP::Arg*>  xorVec;
    xorVec.push_back(&comArg);
    xorVec.push_back(&inprocArg);
    cmdLine.xorAdd(xorVec);
    cmdLine.add(&addrArg);
    cmdLine.parse(argc, argv);

    QApplication app(argc, argv);

    if (!comArg.getValue().empty()) {
        _stream.reset(new RsSink(comArg.getValue().c_str()));
    } else if (inprocArg.getValue()) {
        _stream.reset(new InProcSink);
    } else {
        std::abort();
    }

    _gc.reset(new GroundControl(_stream.get(), _sched.get(), _handler.get()));
    _w.reset(new FirmwareWidget(_handler.get()));

    //_w->resize(1024, 768);
    //_w->showMaximized();
    QObject::connect(_handler.get(), &QModelEventHandler::nodeValueUpdated, _w->qmodel(), &QModel::notifyValueUpdate);
    QObject::connect(_handler.get(), &QModelEventHandler::nodesInserted, _w->qmodel(), &QModel::notifyNodesInserted);
    QObject::connect(_handler.get(), &QModelEventHandler::nodesRemoved, _w->qmodel(), &QModel::notifyNodesRemoved);
    QObject::connect(_handler.get(), &QModelEventHandler::modelUpdated, _w.get(), [](const Rc<Model>& model) {
        _w->setModel(model.get());
        BMCL_DEBUG() << model.get();
        _w->show();
    });
    QObject::connect(_handler.get(), &QModelEventHandler::packetQueued, _w.get(), [](bmcl::Bytes packet) {
        _gc->sendPacket(packet);
    });

    _gc->start();

    return app.exec();
}
