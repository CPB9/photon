#include "photon/exc/Exc.Component.h"

#include "photon/core/RingBuf.h"
#include "photon/core/Try.h"
#include "photon/core/Util.h"
#include "photon/tm/Tm.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/exc/Exc.Constants.h"
#include "photon/exc/DataPacket.h"
#include "photon/exc/Utils.h"
#include "photon/exc/PacketType.h"

void PhotonExc_Init()
{
    _exc.outCounter = 0;
    PhotonRingBuf_Init(&_exc.outStream, _exc.outStreamData, sizeof(_exc.outStreamData));
}

void PhotonExc_AcceptInput(const void* src, size_t size)
{
}

static uint8_t temp[1024];
PhotonWriter writer;

static PhotonError tmGen(void* data, PhotonWriter* dest)
{
    (void)data;
    return PhotonTm_CollectMessages(dest);
}

static PhotonError packTelemetry(PhotonWriter* dest)
{
    PhotonExcDataPacket packet;
    packet.address.type = PhotonAddressType_Simple;
    packet.address.data.simpleAddress.srcAddress = 1;
    packet.address.data.simpleAddress.destAddress = 0;
    packet.counter = _exc.outCounter;
    packet.dataType = PhotonExcDataType_Telemetry;
    packet.streamType = PhotonExcStreamType_Unreliable;
    packet.time.type = PhotonTimeType_Secs;
    packet.time.data.secsTime.seconds = 0;

    PhotonError err = PhotonExcDataPacket_Encode(&packet, tmGen, 0, dest); //TODO: check error

    PhotonRingBuf_Write(&_exc.outStream, dest->start, dest->current - dest->start);

    _exc.outCounter++;

    return PhotonError_Ok;
}

static size_t writeOutput(void* dest, size_t size)
{
    PhotonRingBuf_Write(&_exc.outStream, temp, writer.current - writer.start);
    size = PHOTON_MAX(size, PhotonRingBuf_ReadableSize(&_exc.outStream));
    PhotonRingBuf_Read(&_exc.outStream, dest, size);
    return size;
}

size_t PhotonExc_GenOutput(void* dest, size_t size)
{
    PhotonWriter_Init(&writer, temp, sizeof(temp));

    if (PhotonFwt_HasAnswers()) {
        PhotonWriter_WriteU16Le(&writer, PHOTON_STREAM_SEPARATOR);

        PHOTON_EXC_ENCODE_PACKET_HEADER(&writer, reserved);

        PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Firmware, &reserved));
        PHOTON_TRY(PhotonFwt_GenAnswer(&reserved));

        PHOTON_EXC_ENCODE_PACKET_FOOTER(&writer, reserved);

        return writeOutput(dest, size);
    }

    return 0;
}
