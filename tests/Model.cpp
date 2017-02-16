#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/core/Writer.h"
#include "photon/core/RingBuf.h"
#include "photon/core/Util.h"
#include "photon/core/Logging.h"
#include "photon/gc/Exchange.h"

#include <bmcl/Logging.h>
#include <bmcl/RingBuffer.h>
#include <bmcl/Endian.h>

#include <chrono>
#include <thread>

#define _PHOTON_FNAME "tests/Model.cpp"

using namespace photon;

class UavModel : public Exchange {
private:
    void handleIncoming(bmcl::Bytes packet) override
    {
    }
    void handleJunk(bmcl::Bytes junk) override
    {
    }
};

static uint8_t inTemp[4096];
static UavModel exchange;

const std::size_t chunkSize = 200;

static void onboardTick()
{
    std::size_t size = PhotonExc_GenOutput(inTemp, sizeof(inTemp));
    if (size == 0) {
        PHOTON_DEBUG("No data from photon");
    } else {
        exchange.acceptInput(bmcl::Bytes(inTemp, sizeof(inTemp)));
    }
}

static void gcTick()
{
    std::size_t size = exchange.genOutput(inTemp, sizeof(inTemp));
    if (size == 0) {
        PHOTON_DEBUG("No data from gc");
    } else {
        PhotonExc_AcceptInput(inTemp, sizeof(inTemp));
    }
}

int main()
{
    PhotonTm_Init();
    PhotonExc_Init();
    PhotonFwt_Init();

    while (true) {
        onboardTick();
        gcTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
