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

PhotonError PhotonDfu_HandleInitSectorData(PhotonDfuSectorData* data)
{
    data->sectorToBoot = 1;
    data->loadSuccess = true;
    data->currentSector = PHOTON_DFU_CURRENT_SECTOR;
    return PhotonError_Ok;
}

PhotonError PhotonDfu_HandleInitSectorDesc(PhotonDfuAllSectorsDesc* data)
{
    data->size = 4;
    data->data[0].kind = PhotonDfuSectorType_Bootloader;
    data->data[0].start = 0x8000000;
    data->data[0].size = 99 * 1024;
    data->data[1].kind = PhotonDfuSectorType_SectorData;
    data->data[1].start = 0x8000000 + 99 * 1024;
    data->data[1].size = 1 * 1024;
    data->data[2].kind = PhotonDfuSectorType_UserData;
    data->data[2].start = 0x8000000 + 100 * 1024;
    data->data[2].size = 200 * 1024;
    data->data[3].kind = PhotonDfuSectorType_Firmware;
    data->data[3].start = 0x8000000 + 300 * 1024;
    data->data[3].size = 16 * 1024;
    return PhotonError_Ok;
}

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

PhotonError PhotonDfu_HandleReboot(uint64_t id)
{
    return PhotonError_Ok;
}

#endif

void PhotonDfu_Init()
{
    _photonDfu.state = PhotonDfuState_Idle;
    _photonDfu.response = PhotonDfuResponse_None;
    _photonDfu.currentWriteOffset = 0;
    _photonDfu.lastError = "";
    _photonDfu.currentSector = 0;
    PhotonDfu_HandleInitSectorData(&_photonDfu.sectorData);
    PhotonDfu_HandleInitSectorDesc(&_photonDfu.allSectorsDesc);
#ifdef PHOTON_STUB
    stubFirmware = 0;

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
    uint64_t sectorId;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &sectorId));
    if (sectorId != _photonDfu.currentSector) {
        setError("Sector id does not match");
        return PhotonError_InvalidValue;
    }

    uint64_t totalSize;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &totalSize));
    //TODO: check sector id
    if (totalSize != _photonDfu.transferSize) {
        setError("Invalid total size");
        return PhotonError_InvalidValue;
    }

    PhotonDfuSectorDesc* sector = &_photonDfu.allSectorsDesc.data[_photonDfu.currentSector];
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

    _photonDfu.currentWriteOffset += size;
    _photonDfu.response = PhotonDfuResponse_WriteOk;
    return PhotonError_Ok;
}

static PhotonError reboot(PhotonReader* src)
{
    uint64_t sectorId;
    PHOTON_TRY(PhotonReader_ReadVaruint(src, &sectorId));

    if (sectorId > _photonDfu.allSectorsDesc.size) {
        setError("Invalid sector id");
        return PhotonError_InvalidValue;
    }

    return PhotonDfu_HandleReboot(sectorId);
}

static DfuHandler handlers[2][5] = {
//      getinfo, beginupdate,         writechunk, endupdate  reboot
    {reportInfo, beginUpdate, reportInvalidState, endUpdate, reboot}, //idle
    {reportInfo, beginUpdate, writeChunk,         endUpdate, reportInvalidState}, //writing
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
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_GetInfo, dest));
    PHOTON_TRY(PhotonDfuSectorData_Serialize(&_photonDfu.sectorData, dest));
    PHOTON_TRY(PhotonDynArrayOfDfuSectorDescMaxSize8_Serialize(&_photonDfu.allSectorsDesc, dest));
    _photonDfu.response = PhotonDfuResponse_None;
    return PhotonError_Ok;
}

static PhotonError writeBeginOk(PhotonWriter* dest)
{
    PHOTON_TRY(PhotonDfuResponse_Serialize(PhotonDfuResponse_BeginOk, dest));
    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, _photonDfu.currentSector));
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
