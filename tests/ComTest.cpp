#include "UiTest.h"

#include <decode/groundcontrol/Exchange.h>

#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <QApplication>
#include <QtSerialPort/QtSerialPort>
//#include <QUdpSocket>

using namespace decode;

class RsSink : public DataStream {
public:
    RsSink(const char* path, unsigned baudRate)
        : _port(new QSerialPort(path))
    {
        _port->setBaudRate(baudRate);
        _port->open(QIODevice::ReadWrite);
        BMCL_DEBUG() << _port->error();
        connect(_port, &QSerialPort::readyRead, this, &DataStream::readyRead);
    }

    ~RsSink()
    {
        delete _port;
    }

    void sendData(bmcl::Bytes packet) override
    {
        _port->write((char*)packet.data(), packet.size());
    }

    int64_t readData(void* dest, std::size_t maxSize) override
    {
        return _port->read((char*)dest, maxSize);
    }

    int64_t bytesAvailable() const override
    {
        return _port->bytesAvailable();
    }

private:
    QSerialPort* _port;
};

// class UpdSink : public DataStream {
// public:
//     UpdSink()
//         : _sock(new QUdpSocket)
//     {
//         //_sock->setBaudRate(QSerialPort::Baud115200);
//         QHostAddress addr;
//         addr.setAddress("127.0.0.1");
//         _sock->connectToHost(addr, 6000);
//         BMCL_DEBUG() << _sock->error();
//     }
//
//     ~UpdSink()
//     {
//         delete _sock;
//     }
//
//     void sendData(bmcl::Bytes packet) override
//     {
//         QHostAddress addr;
//         addr.setAddress("127.0.0.1");
//         _sock->writeDatagram((char*)packet.data(), packet.size(), addr, 6000);
//     }
//
//     int64_t readData(void* dest, std::size_t maxSize) override
//     {
//         _sock->waitForReadyRead();
//         return _sock->readDatagram((char*)dest, maxSize);
//     }
//
// private:
//     QUdpSocket* _sock;
// };

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("PhotonUi");
    TCLAP::ValueArg<std::string> comArg("c", "com", "Read from COM", true, "", "com device");
    TCLAP::ValueArg<unsigned> baudArg("b", "baud", "Com port baud rate", false, 9600, "baud rate");

    cmdLine.add(&comArg);
    cmdLine.add(&baudArg);
    cmdLine.parse(argc, argv);

    QApplication app(argc, argv);

    Rc<DataStream> _stream = new RsSink(comArg.getValue().c_str(), baudArg.getValue());
    return runUiTest(&app, _stream.get());
}
