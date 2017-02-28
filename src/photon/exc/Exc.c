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
#include "photon/int/Int.Component.h"
#include "photon/core/Logging.h"

#include <string.h>

#define _PHOTON_FNAME "exc/Exc.c"

void PhotonExc_Init()
{
    _exc.outCounter = 0;
    PhotonRingBuf_Init(&_exc.outStream, _exc.outStreamData, sizeof(_exc.outStreamData));
    _exc.inCounter = 0;
    PhotonRingBuf_Init(&_exc.inStream, _exc.inStreamData, sizeof(_exc.inStreamData));
}

static inline uint8_t* findByte(uint8_t* begin, uint8_t* end, uint8_t value)
{
    while (begin < end) {
        if (*begin == value) {
            return begin;
        }
        begin++;
    }
    return end;
}

static void handleJunk(const uint8_t* data, size_t size)
{
    (void)data;
    if (size == 0) {
        return;
    }
    PHOTON_DEBUG("Recieved junk %zu bytes", size);
}

static uint8_t inTemp[1024];
static uint8_t outTemp[1024];

#define HANDLE_INVALID_PACKET(...)                              \
    do {                                                        \
        PHOTON_WARNING(__VA_ARGS__);                            \
        PHOTON_WARNING("Continuing search with 1 byte offset"); \
        PhotonRingBuf_Erase(&_exc.inStream, 1);                 \
    } while(0);

static bool handlePacket(size_t size)
{
    PHOTON_ASSERT(size >= 4);
    uint16_t crc16 = Photon_Crc16(inTemp, size);
    if (crc16 != Photon_Le16Dec(inTemp + size - 2)) {
        HANDLE_INVALID_PACKET("Recieved packet with invalid crc");
        return true;
    }
    PhotonReader payload;
    PhotonReader_Init(&payload, inTemp + 2, size - 4);
    PhotonExcPacketType type;
    if (PhotonExcPacketType_Deserialize(&type, &payload) != PhotonError_Ok) {
        HANDLE_INVALID_PACKET("Recieved packet with invalid packet type");
        return true;
    }
    switch(type) {
    case PhotonExcPacketType_Firmware: {
        if (PhotonFwt_AcceptCmd(&payload) != PhotonError_Ok) {
            HANDLE_INVALID_PACKET("Recieved packet with invalid fwt payload");
            return true;
        }
    }
    case PhotonExcPacketType_Data: {
        PhotonExcDataPacket dataHeader;
        if(PhotonExcDataPacket_Deserialize(&dataHeader, &payload) != PhotonError_Ok) {
            HANDLE_INVALID_PACKET("Recieved data packet with invalid header");
            return true;
        }
        //TODO: check data header
        PhotonWriter results;
        PhotonWriter_Init(&results, outTemp, sizeof(outTemp));
        if (PhotonInt_ExecuteFrom(&payload, &results) != PhotonError_Ok) { //FIXME: handle NotEnoughSpace error
            HANDLE_INVALID_PACKET("Recieved data packet with cmds");
            return true;
        }
        break;
    }
    case PhotonExcPacketType_Receipt:
        break;
    }
    return true;
}

// chunks contain size + data + crc + junk
static bool findPacket(PhotonMemChunks* chunks)
{
    size_t size = chunks->first.size + chunks->second.size;
    if (size < 2) {
        PHOTON_DEBUG("Packet size part not yet recieved");
        return false;
    }
    uint8_t firstSizePart;
    uint8_t secondSizePart;
    if (chunks->first.size >= 2) {
        firstSizePart = chunks->first.data[0];
        secondSizePart = chunks->first.data[1];
    } else if (chunks->first.size == 1) {
        firstSizePart = chunks->first.data[0];
        secondSizePart = chunks->second.data[0];
    } else {
        firstSizePart = chunks->second.data[0];
        secondSizePart = chunks->second.data[1];
    }

    size_t expectedSize = 2 + (firstSizePart | (secondSizePart << 8));

    size_t maxPacketSize = 1024; //TODO: define
    if (expectedSize > maxPacketSize) {
        HANDLE_INVALID_PACKET("Recieved packet with size > max allowed"); //TODO: remove macro
        return true;
    }

    if (size < expectedSize) {
        PHOTON_DEBUG("Packet payload not yet recieved");
        return false;
    }

    if (expectedSize <= chunks->first.size) {
        memcpy(inTemp, chunks->first.data, chunks->first.size);
    } else {
        memcpy(inTemp, chunks->first.data, chunks->first.size);
        memcpy(inTemp + chunks->first.size, chunks->second.data, size - expectedSize);
    }

    return handlePacket(expectedSize);
}

static bool findSep()
{
    const uint16_t separator = 0x9c3e;
    const uint8_t firstSepPart = (separator & 0xff00) >> 8;
    const uint8_t secondSepPart = separator & 0x00ff;
    PhotonMemChunks chunks = PhotonRingBuf_ReadableChunks(&_exc.inStream);

    uint8_t* it = chunks.first.data;
    uint8_t* next;
    while (true) {
        uint8_t* end = it + chunks.first.size;
        it = findByte(it, end, firstSepPart);
        if (it == end) {
            break;
        }
        next = it + 1;
        if (next == end) {
            if (chunks.second.size != 0 && chunks.second.data[0] == secondSepPart) {
                size_t skippedSize = it - chunks.first.data;
                handleJunk(chunks.first.data, skippedSize);
                PhotonRingBuf_Erase(&_exc.inStream, skippedSize);
                chunks.first.size = 0;
                chunks.second.data += 1;
                chunks.second.size -= 1;
                return findPacket(&chunks);
            }
            break;
        }
        if (*next == secondSepPart) {
            size_t skippedSize = it - chunks.first.data;
            handleJunk(chunks.first.data, skippedSize);
            PhotonRingBuf_Erase(&_exc.inStream, skippedSize);
            chunks.first.data += skippedSize + 2;
            chunks.first.size -= skippedSize + 2;
            return findPacket(&chunks);
        }
        it++;
    }

    it = chunks.second.data;
    while (true) {
        uint8_t* end = it + chunks.second.size;
        it = findByte(it, end, firstSepPart);
        next = it + 1;
        if (next >= end) {
            handleJunk(chunks.first.data, chunks.first.size);
            handleJunk(chunks.second.data, chunks.second.size);
            PhotonRingBuf_Clear(&_exc.inStream);
            return false;
        }
        if (*next == secondSepPart) {
            size_t skippedSize = it - chunks.second.data;
            handleJunk(chunks.first.data, chunks.first.size);
            handleJunk(chunks.second.data, skippedSize);
            PhotonRingBuf_Erase(&_exc.inStream, chunks.first.size + skippedSize);
            chunks.first.size = 0;
            chunks.second.data += skippedSize + 2;
            chunks.second.size -= skippedSize + 2;
            return findPacket(&chunks);
        }
        it++;
    }
}

void PhotonExc_AcceptInput(const void* src, size_t size)
{
    PhotonRingBuf_Write(&_exc.inStream, src, size);

    bool canContinue;
    do {
        canContinue = findSep();
    } while (canContinue);
}

static PhotonWriter writer;

static size_t writeOutput(void* dest, size_t size)
{
    PhotonRingBuf_Write(&_exc.outStream, inTemp, writer.current - writer.start);
    size = PHOTON_MIN(size, PhotonRingBuf_ReadableSize(&_exc.outStream));
    PhotonRingBuf_Read(&_exc.outStream, dest, size);
    return size;
}

static PhotonError genFwtPacket()
{
    PHOTON_DEBUG("Generating fwt packet");
    PhotonWriter_WriteU16Le(&writer, PHOTON_STREAM_SEPARATOR);

    PHOTON_EXC_ENCODE_PACKET_HEADER(&writer, reserved);

    PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Firmware, &reserved));
    PHOTON_TRY(PhotonFwt_GenAnswer(&reserved));

    PHOTON_EXC_ENCODE_PACKET_FOOTER(&writer, reserved);

    return PhotonError_Ok;
}

static PhotonError genTmPacket()
{
    PHOTON_DEBUG("Generating tm packet");
    PhotonWriter_WriteU16Le(&writer, PHOTON_STREAM_SEPARATOR);

    PHOTON_EXC_ENCODE_PACKET_HEADER(&writer, reserved);

    PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Data, &reserved));

    PhotonExcDataPacket dataHeader;
    dataHeader.address.srcAddress = 1;
    dataHeader.address.destAddress = 0;
    dataHeader.counter = 0;
    dataHeader.dataType = PhotonExcDataType_Telemetry;
    dataHeader.streamType = PhotonExcStreamType_Unreliable;
    dataHeader.time.type = PhotonTimeType_Secs;
    dataHeader.time.data.secsTime.seconds = 0;

    PHOTON_TRY(PhotonExcDataPacket_Serialize(&dataHeader, &reserved));
    PHOTON_TRY(PhotonTm_CollectMessages(&reserved));

    PHOTON_EXC_ENCODE_PACKET_FOOTER(&writer, reserved);

    return PhotonError_Ok;
}

static inline size_t genPacket(PhotonError (*gen)(), void* dest, size_t size)
{
    if (gen() != PhotonError_Ok) {
        return 0;
    }
    return writeOutput(dest, size);
}

size_t PhotonExc_GenOutput(void* dest, size_t size)
{
    PhotonWriter_Init(&writer, inTemp, sizeof(inTemp));

    if (PhotonFwt_HasAnswers()) {
        return genPacket(genFwtPacket, dest, size);
    }

    return genPacket(genTmPacket, dest, size);
}
