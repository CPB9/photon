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
    _fwt.firmware.current = FW_END;
    _fwt.firmware.end = FW_END;
    _fwt.firmware.isTransfering = false;
    _fwt.hashRequested = false;
    _fwt.chunk.isTransfering = false;
    _fwt.chunk.current = FW_END;
    _fwt.chunk.end = FW_END;
}

static PhotonError requestHash(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("HashRequested requested");
    _fwt.hashRequested = true;
    return PhotonError_Ok;
}

//TODO: resend only sent parts
static PhotonError requestChunk(PhotonReader* src)
{
    if (_fwt.chunk.isTransfering) {
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
    uint8_t* start = FW_START + chunkOffset;
    if (start >= _fwt.firmware.current) {
        return PhotonError_Ok;
    }

    // avoid intersecting part twice
    const uint8_t* end = start + chunkSize;
    if (end > _fwt.firmware.current) {
        end = _fwt.firmware.current;
    }

    _fwt.chunk.current = start;
    _fwt.chunk.end = end;
    _fwt.chunk.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError start(PhotonReader* src)
{

    uint64_t startId;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &startId));

    if (_fwt.firmware.isTransfering && startId == _fwt.startId) {
        return PhotonError_Ok;
    }
    PHOTON_DEBUG("StartRequested requested %u", (unsigned)startId);
    _fwt.startId = startId;
    _fwt.startRequested = true;

    EXPECT_NO_PARAMS_LEFT(src);

    _fwt.firmware.current = FW_START;
    _fwt.firmware.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError stop(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("StopRequested requested");
    _fwt.firmware.isTransfering = false;
    _fwt.firmware.current = FW_END;
    _fwt.chunk.isTransfering = false;
    _fwt.chunk.current = FW_END;
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
    if (PhotonWriter_WritableSize(dest) < _PHOTON_PACKAGE_HASH_SIZE) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter_Write(dest, _packageHash, _PHOTON_PACKAGE_HASH_SIZE);
    _fwt.hashRequested = false;
    return PhotonError_Ok;
}

static PhotonError genStart(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Start, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _fwt.startId));
    _fwt.startRequested = false;
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
    if (_fwt.hashRequested) {
        return genHash(dest);
    }

    if (_fwt.startRequested) {
        return genStart(dest);
    }

    if (_fwt.chunk.isTransfering) {
        return genNext(&_fwt.chunk, dest);
    }

    if (_fwt.firmware.isTransfering) {
        return genNext(&_fwt.firmware, dest);
    }

    return PhotonError_NoDataAvailable;
}

bool PhotonFwt_HasAnswers()
{
    return _fwt.hashRequested | _fwt.startRequested | _fwt.chunk.isTransfering | _fwt.firmware.isTransfering;
}

PhotonSliceOfU8 PhotonFwt_Firmware()
{
    PhotonSliceOfU8 fw;
    fw.data = FW_START;
    fw.size = _PHOTON_PACKAGE_SIZE;
    return fw;
}
