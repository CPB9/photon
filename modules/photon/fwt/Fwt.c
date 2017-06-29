#include "photon/fwt/Fwt.Component.h"

#include "photon/core/Assert.h"
#include "photon/core/Try.h"
#include "photon/core/Util.h"
#include "photon/core/Logging.h"
#include "photon/fwt/AnswerType.h"
#include "photon/fwt/CmdType.h"
#include "photon/Package.Private.inc.c"

#include <inttypes.h>

#define _PHOTON_FNAME "fwt/Fwt.c"

#define FW_START &_package[0]
#define FW_END &_package[_PHOTON_PACKAGE_SIZE]

#define EXPECT_NO_PARAMS_LEFT(src)              \
    if (PhotonReader_ReadableSize(src) != 0) {  \
        return PhotonError_InvalidValue;        \
    }

#define MAX_SIZE 128

void PhotonFwt_Init()
{
    _photonFwt.firmware.current = FW_END;
    _photonFwt.firmware.end = FW_END;
    _photonFwt.firmware.isTransfering = false;
    _photonFwt.hashRequested = false;
    _photonFwt.chunk.isTransfering = false;
    _photonFwt.chunk.current = FW_END;
    _photonFwt.chunk.end = FW_END;
}

static PhotonError requestHash(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("HashRequested requested");
    _photonFwt.hashRequested = true;
    return PhotonError_Ok;
}

//TODO: resend only sent parts
static PhotonError requestChunk(PhotonReader* src)
{
    if (_photonFwt.chunk.isTransfering) {
        return PhotonError_Ok;
    }

    uint64_t chunkOffset;
    uint64_t chunkSize;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkOffset));
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkSize));
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("ChunkRequested requested %u, %u", (unsigned)chunkOffset, (unsigned)chunkSize);

    if (chunkSize == 0) {
        return PhotonError_Ok;
    }

    if (chunkOffset >= _PHOTON_PACKAGE_SIZE) {
        return PhotonError_InvalidValue;
    }

    if (chunkSize > _PHOTON_PACKAGE_SIZE) {
        return PhotonError_InvalidValue;
    }

    if (chunkOffset > _PHOTON_PACKAGE_SIZE - chunkSize) {
        return PhotonError_InvalidValue;
    }

    // avoid sending twice
    const uint8_t* start = FW_START + chunkOffset;
    if (start >= _photonFwt.firmware.current) {
        return PhotonError_Ok;
    }

    // avoid intersecting part twice
    const uint8_t* end = start + chunkSize;
    if (end > _photonFwt.firmware.current) {
        end = _photonFwt.firmware.current;
    }

    _photonFwt.chunk.current = start;
    _photonFwt.chunk.end = end;
    _photonFwt.chunk.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError start(PhotonReader* src)
{

    uint64_t startId;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &startId));

    if (_photonFwt.firmware.isTransfering && startId == _photonFwt.startId) {
        return PhotonError_Ok;
    }
    PHOTON_DEBUG("StartRequested requested %u", (unsigned)startId);
    _photonFwt.startId = startId;
    _photonFwt.startRequested = true;

    EXPECT_NO_PARAMS_LEFT(src);

    _photonFwt.firmware.current = FW_START;
    _photonFwt.firmware.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError stop(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("StopRequested requested");
    _photonFwt.firmware.isTransfering = false;
    _photonFwt.firmware.current = FW_END;
    _photonFwt.chunk.isTransfering = false;
    _photonFwt.chunk.current = FW_END;
    return PhotonError_Ok;
}

PhotonError PhotonFwt_AcceptCmd(PhotonReader* src)
{
    PhotonFwtCmdType cmd;
    PHOTON_TRY(PhotonFwtCmdType_Deserialize(&cmd, src));

    switch (cmd) {
    case PhotonFwtCmdType_RequestHash:
        return requestHash(src);
    case PhotonFwtCmdType_RequestChunk:
        return requestChunk(src);
    case PhotonFwtCmdType_Start:
        return start(src);
    case PhotonFwtCmdType_Stop:
        return stop(src);
    }

    return PhotonError_InvalidValue;
}

static PhotonError genHash(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Hash, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _PHOTON_PACKAGE_SIZE));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _PHOTON_DEVICE_NAME_SIZE));
    if (PhotonWriter_WritableSize(dest) < (_PHOTON_PACKAGE_HASH_SIZE + _PHOTON_DEVICE_NAME_SIZE)) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter_Write(dest, _deviceName, _PHOTON_DEVICE_NAME_SIZE);
    PhotonWriter_Write(dest, _packageHash, _PHOTON_PACKAGE_HASH_SIZE);
    _photonFwt.hashRequested = false;
    return PhotonError_Ok;
}

static PhotonError genStart(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Start, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonFwt.startId));
    _photonFwt.startRequested = false;
    return PhotonError_Ok;
}

static PhotonError genNext(PhotonFwtChunk* chunk, PhotonWriter* dest)
{
    PHOTON_ASSERT(chunk->current < chunk->end);
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Chunk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, chunk->current - FW_START));

    size_t size = chunk->end - chunk->current;
    size = PHOTON_MIN(size, MAX_SIZE);
    size = PHOTON_MIN(size, PhotonWriter_WritableSize(dest));

    if (size == 0) {
        return PhotonError_NotEnoughSpace;
    }

    PhotonWriter_Write(dest, chunk->current, size);
    chunk->current += size;

    if (chunk->current == chunk->end) {
        chunk->isTransfering = false;
    }

    return PhotonError_Ok;
}

PhotonError PhotonFwt_GenAnswer(PhotonWriter* dest)
{
    if (_photonFwt.hashRequested) {
        return genHash(dest);
    }

    if (_photonFwt.startRequested) {
        return genStart(dest);
    }

    if (_photonFwt.chunk.isTransfering) {
        return genNext(&_photonFwt.chunk, dest);
    }

    if (_photonFwt.firmware.isTransfering) {
        return genNext(&_photonFwt.firmware, dest);
    }

    return PhotonError_NoDataAvailable;
}

bool PhotonFwt_HasAnswers()
{
    return _photonFwt.hashRequested | _photonFwt.startRequested | _photonFwt.chunk.isTransfering | _photonFwt.firmware.isTransfering;
}

#undef _PHOTON_FNAME
