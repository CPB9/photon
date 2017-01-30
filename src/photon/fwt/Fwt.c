#include "photon/fwt/Fwt.Component.h"

#include "photon/core/Assert.h"
#include "photon/core/Try.h"
#include "photon/core/Util.h"
#include "photon/fwt/AnswerType.h"
#include "photon/fwt/CmdType.h"
#include "photon/Package.Private.inc.c"

#define FW_START &_package[0]
#define FW_END &_package[_PHOTON_PACKAGE_SIZE]

#define EXPECT_NO_PARAMS_LEFT(src)              \
    if (PhotonReader_ReadableSize(src) != 0) {  \
        return PhotonError_InvalidValue;        \
    }

#define MAX_SIZE 128

void PhotonFwt_Init()
{
    _fwt.firmware.current = FW_START;
    _fwt.firmware.end = FW_END;
    _fwt.firmware.isTransfering = false;
    _fwt.hashRequested = false;
    _fwt.chunk.isTransfering = false;
    _fwt.chunk.current = FW_START;
    _fwt.chunk.end = FW_END;
}

static PhotonError requestHash(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    _fwt.hashRequested = true;
    return PhotonError_Ok;
}

static PhotonError requestChunk(PhotonReader* src)
{
    uint64_t chunkOffset;
    uint64_t chunkSize;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkOffset));
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &chunkSize));
    EXPECT_NO_PARAMS_LEFT(src);

    if (chunkOffset >= _PHOTON_PACKAGE_SIZE) {
        return PhotonError_InvalidValue;
    }

    if (chunkSize > _PHOTON_PACKAGE_SIZE) {
        return PhotonError_InvalidValue;
    }

    if (chunkOffset > _PHOTON_PACKAGE_SIZE - chunkSize) {
        return PhotonError_InvalidValue;
    }

    _fwt.chunk.current = FW_START + chunkOffset;
    _fwt.chunk.end = _fwt.chunk.current + chunkSize;
    _fwt.chunk.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError start(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    _fwt.firmware.current = FW_START;
    _fwt.firmware.isTransfering = true;
    return PhotonError_Ok;
}

static PhotonError stop(PhotonReader* src)
{
    EXPECT_NO_PARAMS_LEFT(src);
    _fwt.firmware.isTransfering = false;
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
    if (PhotonWriter_WritableSize(dest) < _PHOTON_PACKAGE_HASH_SIZE) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter_Write(dest, _packageHash, _PHOTON_PACKAGE_HASH_SIZE);
    _fwt.hashRequested = false;
    return PhotonError_Ok;
}

static PhotonError genNext(PhotonFwtChunk* chunk, PhotonWriter* dest)
{
    PHOTON_ASSERT(chunk->current < chunk->end);
    PHOTON_TRY(PhotonFwtAnswerType_Serialize(PhotonFwtAnswerType_Chunk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, FW_START - chunk->current));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _PHOTON_PACKAGE_SIZE));

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
    return _fwt.hashRequested | _fwt.chunk.isTransfering | _fwt.firmware.isTransfering;
}
