#include "photongen/onboard/fwt/Fwt.Component.h"

#include "photon/core/Assert.h"
#include "photon/core/Try.h"
#include "photon/core/Util.h"
#include "photon/core/Logging.h"
#include "photongen/onboard/fwt/AnswerType.h"
#include "photongen/onboard/fwt/CmdType.h"
#include "photongen/onboard/Package.inc.c"

#ifdef PHOTON_HAS_MODULE_BLOG
# include "photongen/onboard/blog/Blog.Component.h"
#endif

#include <inttypes.h>

#define _PHOTON_FNAME "fwt/Fwt.c"

#define FW_START &_package[0]
#define FW_END &_package[_PHOTON_PACKAGE_SIZE]

#define EXPECT_NO_PARAMS_LEFT(src)              \
    if (PhotonReader_ReadableSize(src) != 0) {  \
        PHOTON_CRITICAL("Cmd size too big");    \
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
    _photonFwt.startId = 0;
}

void PhotonFwt_Tick()
{
}

static PhotonError requestHash(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    _photonFwt.hashRequested = true;
    PHOTON_DEBUG("HashRequested requested");
    return PhotonError_Ok;
}

//TODO: resend only sent parts
static PhotonError requestChunk(PhotonReader* src)
{
    if (_photonFwt.chunk.isTransfering) {
        return PhotonError_Ok;
    }

    uint64_t chunkBegin;
    uint64_t chunkEnd;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkBegin));
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkEnd));
    EXPECT_NO_PARAMS_LEFT(src);

    if (chunkBegin >= _PHOTON_PACKAGE_SIZE) {
        PHOTON_CRITICAL("Ignoring chunk with begin > fwsize");
        return PhotonError_InvalidValue;
    }

    if (chunkEnd > _PHOTON_PACKAGE_SIZE) {
        PHOTON_CRITICAL("Ignoring chunk with end > fwsize");
        return PhotonError_InvalidValue;
    }

    if (chunkBegin > chunkEnd) {
        PHOTON_CRITICAL("Ignoring chunk with begin > end");
        return PhotonError_InvalidValue;
    }

    if (chunkBegin == chunkEnd) {
        PHOTON_DEBUG("Ignoring zero size chunk");
        return PhotonError_Ok;
    }

    // avoid sending twice
    const uint8_t* start = FW_START + chunkBegin;
    if (start >= _photonFwt.firmware.current) {
        PHOTON_DEBUG("Ignoring duplicating chunk");
        return PhotonError_Ok;
    }

    // avoid intersecting part twice
    const uint8_t* end = FW_START + chunkEnd;
    if (end > _photonFwt.firmware.current) {
        end = _photonFwt.firmware.current;
    }

    _photonFwt.chunk.current = start;
    _photonFwt.chunk.end = end;
    _photonFwt.chunk.isTransfering = true;

    PHOTON_DEBUG("ChunkRequested %u, %u", (unsigned)chunkBegin, (unsigned)chunkEnd);
    return PhotonError_Ok;
}

static PhotonError start(PhotonReader* src)
{

    uint64_t startId;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &startId));

    if (_photonFwt.firmware.isTransfering || startId == _photonFwt.startId) {
        PHOTON_WARNING("Ignoring duplicate start cmd");
        return PhotonError_Ok;
    }
    _photonFwt.startId = startId;
    _photonFwt.startRequested = true;

    EXPECT_NO_PARAMS_LEFT(src);

    _photonFwt.firmware.current = FW_START;
    _photonFwt.firmware.isTransfering = true;
    PHOTON_DEBUG("StartRequested requested %u", (unsigned)startId);
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

PhotonError PhotonFwt_AcceptCmd(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* dest)
{
    (void)dest;
    (void)header;
#ifdef PHOTON_HAS_MODULE_BLOG
    const uint8_t* msgBegin = src->current;
#endif
    PhotonFwtCmdType cmd;
    PHOTON_TRY(PhotonFwtCmdType_Deserialize(&cmd, src));

    PhotonError rv = PhotonError_Ok;

    switch (cmd) {
    case PhotonFwtCmdType_RequestHash:
        rv = requestHash(src);
        break;
    case PhotonFwtCmdType_RequestChunk:
        rv = requestChunk(src);
        break;
    case PhotonFwtCmdType_Start:
        rv = start(src);
        break;
    case PhotonFwtCmdType_Stop:
        rv = stop(src);
        break;
    default:
        PHOTON_CRITICAL("Invalid cmd type");
        return PhotonError_InvalidValue;
    }

    if (rv == PhotonError_Ok) {
#ifdef PHOTON_HAS_MODULE_BLOG
        PhotonBlog_LogFwtCmd(msgBegin, src->current - msgBegin);
#endif
    }

    return rv;
}

static PhotonError genHash(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Hash, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _PHOTON_PACKAGE_SIZE));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _PHOTON_DEVICE_NAME_SIZE));
    if (PhotonWriter_WritableSize(dest) < (_PHOTON_PACKAGE_HASH_SIZE + _PHOTON_DEVICE_NAME_SIZE)) {
        PHOTON_CRITICAL("Not enough space to generate fwt cmd");
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter_Write(dest, _deviceName, _PHOTON_DEVICE_NAME_SIZE);
    PhotonWriter_Write(dest, _packageHash, _PHOTON_PACKAGE_HASH_SIZE);
    _photonFwt.hashRequested = false;
    PHOTON_DEBUG("Generated hash response");
    return PhotonError_Ok;
}

static PhotonError genStart(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Start, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonFwt.startId));
    _photonFwt.startRequested = false;
    PHOTON_DEBUG("Generated start response %u", (unsigned)_photonFwt.startId);
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
        PHOTON_CRITICAL("Not enough space to generate next");
        return PhotonError_NotEnoughSpace;
    }

#if PHOTON_LOG_LEVEL == PHOTON_LOG_LEVEL_DEBUG
    unsigned current = chunk->current - FW_START;
    unsigned end = current + size;
#endif
    PhotonWriter_Write(dest, chunk->current, size);
    chunk->current += size;

    if (chunk->current == chunk->end) {
        chunk->isTransfering = false;
    }

    PHOTON_DEBUG("Generated chunk response %u, %u", current, end);
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

const uint8_t* PhotonFwt_GetFirmwareData()
{
    return FW_START;
}

size_t PhotonFwt_GetFirmwareSize()
{
    return _PHOTON_PACKAGE_SIZE;
}

#undef _PHOTON_FNAME
