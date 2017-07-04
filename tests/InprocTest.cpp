#include "UiTest.h"

#include <decode/core/Rc.h>
#include <decode/groundcontrol/Exchange.h>

#include "photon/Init.h"
#include "photon/exc/Exc.Component.h"
#include "photon/tm/Tm.Component.h"

#include <bmcl/Logging.h>

#include <QApplication>
#include <QTimer>

using namespace decode;

class InProcStream : public DataStream {
public:
    InProcStream()
    {
        PhotonTm_Init();
        Photon_Init();
        _current = *PhotonExc_GetMsg();
        _timer = new QTimer;
        connect(_timer, &QTimer::timeout, this, &InProcStream::readyRead);
        _timer->start(100);
    }

    ~InProcStream()
    {
        delete _timer;
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

    int64_t bytesAvailable() const override
    {
        return _current.size;
    }

private:
    PhotonExcMsg _current;
    QTimer* _timer;
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    Rc<DataStream> _stream = new InProcStream;
    return runUiTest(&app, _stream.get());
}

