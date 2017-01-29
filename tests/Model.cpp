#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/core/Writer.h"
#include "photon/core/RingBuf.h"
#include "photon/core/Util.h"

#include <bmcl/Logging.h>

class Exchange {
public:
    void acceptInput(const void* src, std::size_t size);
    std::size_t genOutput(void* dest, std::size_t size);
};

static uint8_t uplinkBuf[4096];
static uint8_t downlinkBuf[4096];
static PhotonRingBuf downlink;
static PhotonRingBuf uplink;
static uint8_t temp[4096];

const std::size_t chunkSize = 200;

static void onboardTick()
{
    if (PhotonRingBuf_ReadableSize(&uplink) != 0) {
        std::size_t size = PhotonRingBuf_LinearReadableSize(&uplink);
        size = PHOTON_MIN(size, chunkSize);
        PhotonExc_AcceptInput(PhotonRingBuf_CurrentReadPtr(&uplink), size);
        PhotonRingBuf_Advance(&uplink, size);
    }

    std::size_t size = PhotonExc_GenOutput(temp, sizeof(temp));

    if (size == 0) {
        BMCL_DEBUG() << "No data from photon";
    } else {
        PhotonRingBuf_Write(&downlink, temp, size);
    }

}

static void gcTick()
{
}

int main()
{
    PhotonTm_Init();
    PhotonExc_Init();
    PhotonFwt_Init();

    PhotonRingBuf_Init(&uplink, uplinkBuf, sizeof(uplinkBuf));
    PhotonRingBuf_Init(&downlink, downlinkBuf, sizeof(downlinkBuf));

    while (true) {
        onboardTick();
        gcTick();
    }
}
