#include "photongen/onboard/dfu/Dfu.Component.h"
#include "photongen/onboard/dfu/Cmd.h"
#include "photongen/onboard/dfu/Response.h"
#include "photon/core/Try.h"

#include "photon/core/Assert.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "fwt/Dfu.c"

static void setError(const char* err)
{
    _photonDfu.response = PhotonDfuResponse_Error;
    _photonDfu.lastError = err;
}

#ifdef PHOTON_STUB

#include <stdlib.h>
#include <string.h>

#ifndef PHOTON_DFU_CURRENT_SECTOR
#define PHOTON_DFU_CURRENT_SECTOR 0
#endif

static uint8_t* stubFirmware = 0;

PhotonError PhotonDfu_HandleBeginUpdate(const PhotonDfuSectorDesc* sector)
{
    if (stubFirmware) {
        free(stubFirmware);
        stubFirmware = 0;
    }
    stubFirmware = (uint8_t*)malloc(sector->size);
    return PhotonError_Ok;
}

PhotonError PhotonDfu_HandleWriteChunk(const void* data, uint64_t offset, uint64_t size)
{
    memcpy(&stubFirmware[offset], data, size);
    return PhotonError_Ok;
}


PhotonError PhotonDfu_HandleEndUpdate(const PhotonDfuSectorDesc* sector)
{
    if (stubFirmware) {
        free(stubFirmware);
        stubFirmware = 0;
    }
    return PhotonError_Ok;
}

#endif

void PhotonDfu_Init()
{
    _photonDfu.state = PhotonDfuState_Idle;
    _photonDfu.currentWriteOffset = 0;
    _photonDfu.lastError = "";
    _photonDfu.currentSector = 0;
#ifdef PHOTON_STUB
    stubFirmware = 0;
    _photonDfu.allSectorsDesc.size = 2;
    _photonDfu.allSectorsDesc.data[0].kind = PhotonDfuSectorType_Bootloader;
    _photonDfu.allSectorsDesc.data[0].size = 100 * 1024;
    _photonDfu.allSectorsDesc.data[0].start = 0x8000000;

    _photonDfu.allSectorsDesc.data[1].kind = PhotonDfuSectorType_Firmware;
    _photonDfu.allSectorsDesc.data[1].size = 200 * 1024;
    _photonDfu.allSectorsDesc.data[1].start = 0x8000000 + 100 * 1024;
#endif
}

void PhotonDfu_Tick()
{
}

typedef PhotonError (*DfuHandler)(PhotonReader* src);

static PhotonError reportInvalidState(PhotonReader* src)
{
    setError("invalid state");
    return PhotonError_InvalidValue;
}

static PhotonError reportInfo(PhotonReader* src)
{
    _photonDfu.response = PhotonDfuResponse_GetInfo;
    return PhotonError_Ok;
}

static PhotonError beginUpdate(PhotonReader* src)
{
    uint64_t sectorId;
    uint64_t totalSize;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &sectorId));
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &totalSize));

    if (sectorId == PHOTON_DFU_CURRENT_SECTOR) {
        setError("Cannot flash sector currently booted into");
        return PhotonError_InvalidValue;
    }

    if (sectorId >= _photonDfu.allSectorsDesc.size) {
        setError("Invalid sector id");
        return PhotonError_InvalidValue;
    }

    PhotonDfuSectorDesc* sector = &_photonDfu.allSectorsDesc.data[sectorId];
    if (sector->kind == PhotonDfuSectorType_SectorData) {
        setError("Cannot flash sector data");
        return PhotonError_InvalidValue;
    }

    if (totalSize > sector->size) {
        setError("Sector size overflow");
        return PhotonError_InvalidValue;
    }

    _photonDfu.currentWriteOffset = 0;
    _photonDfu.transferSize = totalSize;
    _photonDfu.currentSector = sectorId;
    PHOTON_TRY(PhotonDfu_HandleBeginUpdate(sector));

    _photonDfu.state = PhotonDfuState_Writing;
    _photonDfu.response = PhotonDfuResponse_BeginOk;
    return PhotonError_Ok;
}

static PhotonError endUpdate(PhotonReader* src)
{
    uint64_t totalSize;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &totalSize));
    //TODO: check sector id
    PhotonDfuSectorDesc* sector = &_photonDfu.allSectorsDesc.data[_photonDfu.currentSector];
    if (totalSize != sector->size) {
        setError("Invalid total size");
        return PhotonError_InvalidValue;
    }

    PHOTON_TRY(PhotonDfu_HandleEndUpdate(sector));
    _photonDfu.response = PhotonDfuResponse_EndOk;

    return PhotonError_Ok;
}

static PhotonError writeChunk(PhotonReader* src)
{
    uint64_t offset;
    uint64_t size;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &offset));
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &size));

    if (size != PhotonReader_ReadableSize(src)) {
        setError("invalid chunk size");
        return PhotonError_InvalidValue;
    }

    if (size > _photonDfu.transferSize) {
        setError("chunk size > total size");
        return PhotonError_InvalidValue;
    }

    if (offset != _photonDfu.currentWriteOffset) {
        setError("offset != current offset");
        return PhotonError_InvalidValue;
    }

    if (offset >= _photonDfu.transferSize) {
        setError("offset > total size");
        return PhotonError_InvalidValue;
    }

    if (offset > (_photonDfu.transferSize - size)) {
        setError("write overflow");
        return PhotonError_InvalidValue;
    }

    PHOTON_TRY(PhotonDfu_HandleWriteChunk(PhotonReader_CurrentPtr(src), offset, size));

    _photonDfu.currentWriteOffset += offset;
    _photonDfu.response = PhotonDfuResponse_WriteOk;
    return PhotonError_Ok;
}

static DfuHandler handlers[2][4] = {
//      getinfo,        beginupdate,         writechunk, endupdate
    {reportInfo,        beginUpdate, reportInvalidState, endUpdate}, //idle
    {reportInfo, reportInvalidState, writeChunk,         endUpdate}, //writing
};

PhotonError PhotonDfu_AcceptCmd(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* dest)
{
    (void)header;

    PhotonDfuCmd cmd;
    PHOTON_TRY(PhotonDfuCmd_Deserialize(&cmd, src));

    return handlers[_photonDfu.state][cmd](src);
}

static PhotonError writeInfo(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuSectorData_Serialize(&_photonDfu.sectorData, dest));
    PHOTON_TRY(PhotonDynArrayOfDfuSectorDescMaxSize8_Serialize(&_photonDfu.allSectorsDesc, dest));
    _photonDfu.response = PhotonDfuResponse_None;
    return PhotonError_Ok;
}

static PhotonError writeBeginOk(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_BeginOk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonDfu.transferSize));
    _photonDfu.response = PhotonDfuResponse_None;
    return PhotonError_Ok;
}

static PhotonError writeWriteOk(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_WriteOk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonDfu.currentWriteOffset));
    _photonDfu.response = PhotonDfuResponse_None;
    return PhotonError_Ok;
}

static PhotonError writeEndOk(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_EndOk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonDfu.transferSize));
    _photonDfu.response = PhotonDfuResponse_None;
    _photonDfu.state = PhotonDfuState_Idle;
    return PhotonError_Ok;
}

static PhotonError writeError(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_Error, dest));

    size_t size = strlen(_photonDfu.lastError);
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, size));
    if (size > PhotonWriter_WritableSize(dest)) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonWriter_Write(dest, _photonDfu.lastError, size);
    _photonDfu.lastError = "";
    _photonDfu.response = PhotonDfuResponse_None;
    return PhotonError_Ok;
}

PhotonError PhotonDfu_GenAnswer(PhotonWriter* dest)
{
    PhotonDfuResponse resp = _photonDfu.response;
    switch (resp) {
    case PhotonDfuResponse_GetInfo:
        return writeInfo(dest);
    case PhotonDfuResponse_BeginOk:
        return writeBeginOk(dest);
    case PhotonDfuResponse_WriteOk:
        return writeWriteOk(dest);
    case PhotonDfuResponse_EndOk:
        return writeEndOk(dest);
    case PhotonDfuResponse_Error:
        return writeError(dest);
    case PhotonDfuResponse_None:
        break;
    }
    return PhotonError_Ok;
}

bool PhotonDfu_HasAnswers()
{
    return _photonDfu.response != PhotonDfuResponse_None;
}

#undef _PHOTON_FNAME
