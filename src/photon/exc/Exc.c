#include "photon/exc/Exc.Component.h"

#include "photon/core/RingBuf.h"
#include "photon/tm/Tm.Component.h"
#include "photon/exc/Exc.Constants.h"
#include "photon/exc/DataPacket.h"

void PhotonExc_Init()
{
    _exc.outCounter = 0;
    PhotonRingBuf_Init(&_exc.outStream, _exc.outStreamData, sizeof(_exc.outStreamData));
}

static uint8_t temp[1024];

static PhotonError tmGen(void* data, PhotonWriter* dest)
{
    (void)data;
    return PhotonTm_CollectStatusMessages(dest);
}

static PhotonError packTelemetry()
{
    PhotonWriter tmpWriter;
    PhotonWriter_Init(&tmpWriter, temp, sizeof(temp));

    PhotonExcDataPacket packet;
    packet.address.type = PhotonAddressType_Simple;
    packet.address.data.simpleAddress.srcAddress = 1;
    packet.address.data.simpleAddress.destAddress = 0;
    packet.counter = _exc.outCounter;
    packet.dataType = PhotonExcDataType_Telemetry;
    packet.streamType = PhotonExcStreamType_Unreliable;
    packet.time.type = PhotonTimeType_Secs;
    packet.time.data.secsTime.seconds = 0;

    PhotonWriter_WriteU16Le(&tmpWriter, PHOTON_STREAM_SEPARATOR);
    PhotonError err = PhotonExcDataPacket_Encode(&packet, tmGen, 0, &tmpWriter); //TODO: check error

    PhotonRingBuf_Write(&_exc.outStream, tmpWriter.start, tmpWriter.current - tmpWriter.start);

    _exc.outCounter++;

    return PhotonError_Ok;
}
