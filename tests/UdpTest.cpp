#include "UiTest.h"

#include <decode/groundcontrol/Exchange.h>

#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <QApplication>
#include <QUdpSocket>

using namespace decode;

class UdpSink : public DataStream {
public:
    UdpSink(const char* ipAddress, uint16_t port)
        : _sock(new QUdpSocket)
        , _port(port)
    {
        connect(_sock, &QUdpSocket::readyRead, this, &UdpSink::readyRead);
        _addr.setAddress(ipAddress);
        _sock->connectToHost(_addr, port);
        BMCL_DEBUG() << _sock->errorString();
    }

    ~UdpSink()
    {
        delete _sock;
    }

    void sendData(bmcl::Bytes packet) override
    {
        _sock->writeDatagram((char*)packet.data(), packet.size(), _addr, _port);
    }

    int64_t readData(void* dest, std::size_t maxSize) override
    {
        return _sock->readDatagram((char*)dest, maxSize);
    }

    int64_t bytesAvailable() const override
    {
        return _sock->bytesAvailable();
    }

private:
    QUdpSocket* _sock;
    QHostAddress _addr;
    uint16_t _port;
};

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("PhotonUi");
    TCLAP::ValueArg<std::string> ipArg("a", "address", "Read from ip address", false, "127.0.0.1", "ip address");
    TCLAP::ValueArg<unsigned> portArg("p", "port", "Port number", false, 6666, "port number");

    cmdLine.add(&ipArg);
    cmdLine.add(&portArg);
    cmdLine.parse(argc, argv);

    QApplication app(argc, argv);

    Rc<DataStream> _stream = new UdpSink(ipArg.getValue().c_str(), portArg.getValue());
    return runUiTest(&app, _stream.get());
}

