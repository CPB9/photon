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
#define FWT_NUM_CHUNKS (sizeof(_photonFwt.chunks) / sizeof(_photonFwt.chunks[0]))

void PhotonFwt_Init()
{
    _photonFwt.state = PhotonFwtState_Idle;
    _photonFwt.numChunks = 0;
    _photonFwt.currentChunk = 0;
}

void PhotonFwt_Tick()
{
}

static PhotonError requestHash(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    _photonFwt.state = PhotonFwtState_AnsweringHash;
    PHOTON_DEBUG("HashRequested requested");
    return PhotonError_Ok;
}

static void mergeChunk(const uint8_t* start, const uint8_t* end)
{
    size_t cur = _photonFwt.currentChunk;
    size_t num = _photonFwt.numChunks;

    while (num) {
        PhotonFwtChunk* other = &_photonFwt.chunks[cur];

        if (start <= other->start) {
            if (end < other->start) {
                goto next;
            }
            if (end <= other->end) {
                other->start  = start;
                return;
            }
            other->start = start;
            other->end = end;
            return;
        }
        if (start > other->end) {
            goto next;
        }
        if (end <= other->end) {
            return;
        }
        other->end = end;
        return;
next:
        cur = (cur + 1) % FWT_NUM_CHUNKS;
        num--;
    }

    size_t i = (_photonFwt.currentChunk + _photonFwt.numChunks) % FWT_NUM_CHUNKS;
    _photonFwt.numChunks++;
    PhotonFwtChunk* chunk = &_photonFwt.chunks[i];
    chunk->start = start;
    chunk->end = end;
    _photonFwt.state = PhotonFwtState_Transfering;
}

static PhotonError requestChunk(PhotonReader* src)
{
    while (true) {
        if (_photonFwt.numChunks == FWT_NUM_CHUNKS) {
            return PhotonError_Ok;
        }

        if (PhotonReader_IsAtEnd(src)) {
            return PhotonError_Ok;
        }

        uint64_t chunkBegin;
        uint64_t chunkEnd;
        PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkBegin));
        PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkEnd));

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

        PHOTON_DEBUG("ChunkRequested %u, %u", (unsigned)chunkBegin, (unsigned)chunkEnd);

        mergeChunk(FW_START + chunkBegin, FW_START + chunkEnd);
    }

    return PhotonError_Ok;
}

static PhotonError stop(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    PHOTON_DEBUG("StopRequested requested");
    _photonFwt.state = PhotonFwtState_Idle;
    _photonFwt.numChunks = 0;
    _photonFwt.currentChunk = 0;
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
    _photonFwt.state = PhotonFwtState_Idle;
    PHOTON_DEBUG("Generated hash response");
    return PhotonError_Ok;
}

static PhotonError genNextChunk(PhotonWriter* dest)
{
    if (_photonFwt.numChunks == 0) {
        _photonFwt.state = PhotonFwtState_Idle;
        return PhotonError_Ok;
    }
    PhotonFwtChunk* chunk = &_photonFwt.chunks[_photonFwt.currentChunk];
    PHOTON_ASSERT(chunk->start < chunk->end);
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Chunk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, chunk->start - FW_START));

    size_t size = chunk->end - chunk->start;
    size = PHOTON_MIN(size, MAX_SIZE);
    size = PHOTON_MIN(size, PhotonWriter_WritableSize(dest));

    if (size == 0) {
        PHOTON_CRITICAL("Not enough space to generate next");
        return PhotonError_NotEnoughSpace;
    }

#if PHOTON_LOG_LEVEL == PHOTON_LOG_LEVEL_DEBUG
    unsigned current = chunk->start - FW_START;
    unsigned end = current + size;
#endif
    PhotonWriter_Write(dest, chunk->start, size);
    chunk->start += size;

    if (chunk->start == chunk->end) {
        _photonFwt.currentChunk = (_photonFwt.currentChunk + 1) % FWT_NUM_CHUNKS;
        _photonFwt.numChunks--;
        if (_photonFwt.numChunks == 0) {
            _photonFwt.state = PhotonFwtState_Idle;
        }
    }

    PHOTON_DEBUG("Generated chunk response %u, %u", current, end);
    return PhotonError_Ok;
}

PhotonError PhotonFwt_GenAnswer(PhotonWriter* dest)
{
    switch (_photonFwt.state) {
    case PhotonFwtState_Idle:
        break;
    case PhotonFwtState_AnsweringHash:
        return genHash(dest);
    case PhotonFwtState_Transfering:
        return genNextChunk(dest);
    };

    return PhotonError_NoDataAvailable;
}

bool PhotonFwt_HasAnswers()
{
    return _photonFwt.state != PhotonFwtState_Idle;
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
