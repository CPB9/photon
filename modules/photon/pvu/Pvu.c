#include "photon/pvu/Pvu.Component.h"

#include "photon/pvu/Pvu.Constants.h"
#include "photon/clk/Clk.Component.h"
#include "photon/core/Logging.h"
#include "photon/pvu/Script.h"
#include "photon/core/Assert.h"

#include <string.h>

#define _PHOTON_FNAME "int/Int.c"

PhotonError PhotonPvu_ExecuteFrom(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* results)
{
    if (PhotonReader_ReadableSize(src) == 0) {
        return PhotonError_Ok;
    }
    _photonPvu.currentHeader = header;
    return Photon_ExecScript(src, results);
}

const PhotonExcDataHeader* PhotonPvu_CurrentHeader()
{
    return _photonPvu.currentHeader;
}

static void initScript(PhotonPvuScript* self)
{
    self->isSleeping = false;
    self->isFinished = false;
    self->sleepUntil = 0;
}

void PhotonPvu_Init()
{
    _photonPvu.numScripts = 0;
    _photonPvu.totalScriptsSize = 0;
}

static PhotonPvuScript _scripts[PHOTON_PVU_SCRIPT_NUM];
static uint8_t _temp[128];
static uint8_t _scriptData[PHOTON_PVU_TOTAL_SCRIPTS_SIZE];

static void endCurrent()
{
    _photonPvu.currentScript->isFinished = true;
}

static void execCurrent()
{
    PhotonClkTick currentTime = PhotonClk_GetTime();
    PhotonPvuScript* current = _photonPvu.currentScript;
    _photonPvu.currentHeader = NULL; //HACK
    if (current->isSleeping) {
        if (current->sleepUntil < currentTime) {
            return;
        }
        current->isSleeping = false;
    }
    if (PhotonReader_ReadableSize(&current->data) == 0) {
        endCurrent();
        return;
    }
    do {
        if (PhotonReader_ReadableSize(&current->data) < 2) {
            PHOTON_CRITICAL("Unable to decode cmd header");
            endCurrent();
            return;
        }

        uint8_t compNum = PhotonReader_ReadU8(&current->data);
        uint8_t cmdNum = PhotonReader_ReadU8(&current->data);

        PhotonWriter writer;
        PhotonWriter_Init(&writer, _temp, sizeof(_temp));

        PhotonError rv = Photon_DeserializeAndExecCmd(compNum, cmdNum, &current->data, &writer);
        if (rv == PhotonError_WouldBlock) {
            return;
        }
        if (rv != PhotonError_Ok) {
            endCurrent();
            return;
        }
    } while (PhotonReader_ReadableSize(&current->data) != 0);
    endCurrent();
}

void PhotonPvu_Tick()
{
    if (_photonPvu.numScripts == 0) {
        return;
    }
    for (size_t i = 0; i < _photonPvu.numScripts; i++) {
        _photonPvu.currentScript = &_scripts[i];
        execCurrent();
    }
    _photonPvu.currentScript = NULL;

    size_t deletedIndex = _photonPvu.numScripts;
    while (true) {
        if (deletedIndex == 0) {
            return;
        }
        deletedIndex--;
        if (_scripts[deletedIndex].isFinished) {
            break;
        }
    }
    size_t moveSize = 0;
    for (size_t i = deletedIndex + 1; i < _photonPvu.numScripts; i++) {
        moveSize += _scripts[i].data.end - _scripts[i].data.start;
    }
    PhotonPvuScript* deleted = &_scripts[deletedIndex];
    size_t deletedSize = deleted->data.end - deleted->data.start;
    memmove((uint8_t*)deleted->data.start, deleted->data.end, moveSize);
    memmove(&_scripts[deletedIndex], &_scripts[deletedIndex + 1], sizeof(PhotonPvuScript) * (_photonPvu.numScripts - deletedIndex + 1));
    _photonPvu.numScripts--;
    for (size_t i = deletedIndex; i < _photonPvu.numScripts; i++) {
        _scripts[i].data.start -= deletedSize;
        _scripts[i].data.current -= deletedSize;
        _scripts[i].data.end -= deletedSize;
    }
}

PhotonError PhotonPvu_AddScript(PhotonReader* src)
{
    PHOTON_ASSERT(_photonPvu.totalScriptsSize <= PHOTON_PVU_TOTAL_SCRIPTS_SIZE);
    if (_photonPvu.numScripts >= PHOTON_PVU_SCRIPT_NUM) {
        return PhotonError_NotEnoughSpace;
    }

    size_t scriptSize = PhotonReader_ReadableSize(src);
    if (scriptSize > (PHOTON_PVU_TOTAL_SCRIPTS_SIZE - _photonPvu.totalScriptsSize)) {
        return PhotonError_NotEnoughSpace;
    }
    PhotonReader_Read(src, &_scriptData[_photonPvu.totalScriptsSize], scriptSize);
    PhotonPvuScript* current = &_scripts[_photonPvu.numScripts];
    initScript(current);
    PhotonReader_Init(&current->data, &_scriptData[_photonPvu.totalScriptsSize], scriptSize);
    _photonPvu.totalScriptsSize += scriptSize;
    _photonPvu.numScripts++;

    return PhotonError_Ok;
}

PhotonError PhotonPvu_ExecCmd_SleepFor(uint64_t delta)
{
    if (!_photonPvu.currentScript) {
        return PhotonError_BlockingCmdOutsidePvu;
    }
    if (delta == 0) {
        return PhotonError_Ok;
    }
    _photonPvu.currentScript->isSleeping = true;
    _photonPvu.currentScript->sleepUntil = PhotonClk_GetTime() + delta;
    return PhotonError_WouldBlock;
}

PhotonError PhotonPvu_ExecCmd_SleepUntil(uint64_t time)
{
    if (!_photonPvu.currentScript) {
        return PhotonError_BlockingCmdOutsidePvu;
    }
    PhotonClkTick currentTime = PhotonClk_GetTime();
    if (time >= currentTime) {
        return PhotonError_Ok;
    }
    _photonPvu.currentScript->isSleeping = true;
    _photonPvu.currentScript->sleepUntil = time;
    return PhotonError_WouldBlock;
}

#undef _PHOTON_FNAME
