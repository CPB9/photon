#include "photongen/onboard/pvu/Pvu.Component.h"

#include "photongen/onboard/pvu/Pvu.Constants.h"
#include "photongen/onboard/clk/Clk.Component.h"
#include "photon/core/Logging.h"
#include "photongen/onboard/pvu/Script.h"
#include "photongen/onboard/CmdDecoder.h"
#include "photon/core/Assert.h"

#ifdef PHOTON_HAS_MODULE_BLOG
#include "photongen/onboard/blog/Blog.Component.h"
#endif

#include <string.h>

#define _PHOTON_FNAME "int/Int.c"

static PhotonError execScript(PhotonReader* src, PhotonWriter* dest)
{
    uint8_t compNum;
    uint8_t cmdNum;

    (void)src;
    (void)dest;

    while (PhotonReader_ReadableSize(src) != 0) {
#ifdef PHOTON_HAS_MODULE_BLOG
        const uint8_t* msgBegin = src->current;
#endif
        if (PhotonReader_ReadableSize(src) < 2) {
            PHOTON_CRITICAL("Not enough data to deserialize cmd header");
            return PhotonError_NotEnoughData;
        }
        compNum = PhotonReader_ReadU8(src);
        cmdNum = PhotonReader_ReadU8(src);
        PHOTON_TRY(Photon_DeserializeAndExecCmd(compNum, cmdNum, src, dest));
#ifdef PHOTON_HAS_MODULE_BLOG
        PhotonBlog_LogPvuCmd(msgBegin, src->current - msgBegin);
#endif
    }
    return PhotonError_Ok;
}

PhotonError PhotonPvu_ExecuteFrom(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* results)
{
    if (PhotonReader_ReadableSize(src) == 0) {
        return PhotonError_Ok;
    }
    _photonPvu.currentHeader = header;
    PhotonError rv = execScript(src, results);
    if (rv != PhotonError_Ok) {
        PhotonPvu_QueueEvent_PacketExecFailed(rv, header);
    }
    return rv;
}

const PhotonExcDataHeader* PhotonPvu_CurrentHeader()
{
    return _photonPvu.currentHeader;
}

void PhotonPvu_Init()
{
    _photonPvu.scripts.size = 0;
    _photonPvu.totalScriptsSize = 0;
}

static uint8_t _temp[128];
static uint8_t _scriptData[PHOTON_PVU_TOTAL_SCRIPTS_SIZE];

static void endCurrent()
{
    _photonPvu.currentScript->isFinished = true;
    _photonPvu.currentScript->isExecuting = false;
}

static void execCurrent()
{
    PhotonPvuScript* current = _photonPvu.currentScript;
    if (!current->isExecuting) {
        return;
    }
    PhotonClkTimePoint currentTime = PhotonClk_GetTime();
    _photonPvu.currentHeader = NULL; //HACK
    if (current->isSleeping) {
        if (currentTime < current->sleepUntil) {
            return;
        }
        current->isSleeping = false;
    }
    PhotonReader reader;
    PhotonReader_Init(&reader, &_scriptData[current->memOffset], current->size);
    PhotonReader_Skip(&reader, current->readOffset);
    if (PhotonReader_ReadableSize(&reader) == 0) {
        endCurrent();
        return;
    }
    do {
        if (PhotonReader_ReadableSize(&reader) < 2) {
            PHOTON_CRITICAL("Unable to decode cmd header");
            goto end;
        }

#ifdef PHOTON_HAS_MODULE_BLOG
        const uint8_t* msgBegin = reader.current;
#endif

        uint8_t compNum = PhotonReader_ReadU8(&reader);
        uint8_t cmdNum = PhotonReader_ReadU8(&reader);

        PhotonWriter writer;
        PhotonWriter_Init(&writer, _temp, sizeof(_temp));

        PhotonError rv = Photon_DeserializeAndExecCmd(compNum, cmdNum, &reader, &writer);
        if (rv == PhotonError_WouldBlock) {
            goto correct;
        }
        if (rv != PhotonError_Ok) {
            PhotonPvu_QueueEvent_ScriptExecFailed(rv, &current->desc.name);
            goto end;
        }
#ifdef PHOTON_HAS_MODULE_BLOG
        PhotonBlog_LogPvuCmd(msgBegin, reader.current - msgBegin);
#endif
    } while (PhotonReader_ReadableSize(&reader) != 0);

end:
    endCurrent();
correct:
    current->readOffset = reader.current - reader.start;
}

static void removeScript(size_t deletedIndex)
{
    size_t moveSize = 0;
    for (size_t i = deletedIndex; i < _photonPvu.scripts.size; i++) {
        moveSize += _photonPvu.scripts.data[i].size;
    }
    PhotonPvuScript* deleted = &_photonPvu.scripts.data[deletedIndex];
    uint8_t* begin = &_scriptData[deleted->memOffset];
    memmove(begin, begin + deleted->size, moveSize);
    memmove(&_photonPvu.scripts.data[deletedIndex], &_photonPvu.scripts.data[deletedIndex + 1], sizeof(PhotonPvuScript) * (_photonPvu.scripts.size - deletedIndex + 1));
    _photonPvu.scripts.size--;
    for (size_t i = deletedIndex; i < _photonPvu.scripts.size; i++) {
        _photonPvu.scripts.data[i].memOffset -= deleted->size;
    }
    _photonPvu.totalScriptsSize -= moveSize;
}

void PhotonPvu_Tick()
{
    if (_photonPvu.scripts.size == 0) {
        return;
    }
    for (size_t i = 0; i < _photonPvu.scripts.size; i++) {
        _photonPvu.currentScript = &_photonPvu.scripts.data[i];
        execCurrent();
    }
    if (!_photonPvu.currentScript->desc.autoremove) {
        _photonPvu.currentScript = NULL;
        return;
    }
    _photonPvu.currentScript = NULL;

    size_t deletedIndex = _photonPvu.scripts.size;
    while (true) {
        if (deletedIndex == 0) {
            return;
        }
        deletedIndex--;
        if (_photonPvu.scripts.data[deletedIndex].isFinished) {
            break;
        }
    }
    removeScript(deletedIndex);
}

PhotonError PhotonPvu_AddScript(const PhotonPvuScriptDesc* desc, const PhotonDynArrayOfU8MaxSize1024* body)
{
    PHOTON_ASSERT(_photonPvu.totalScriptsSize <= PHOTON_PVU_TOTAL_SCRIPTS_SIZE);
    if (_photonPvu.scripts.size >= PHOTON_PVU_SCRIPT_NUM) {
        return PhotonError_NotEnoughSpace;
    }

    size_t scriptSize = body->size;
    if (scriptSize > (PHOTON_PVU_TOTAL_SCRIPTS_SIZE - _photonPvu.totalScriptsSize)) {
        return PhotonError_NotEnoughSpace;
    }
    for (size_t i = 0; i < _photonPvu.scripts.size; i++) {
        if (strcmp(_photonPvu.scripts.data[i].desc.name.data, desc->name.data) == 0) {
            return PhotonError_ScriptNameConflict;
        }
    }
    memcpy(&_scriptData[_photonPvu.totalScriptsSize], body->data, scriptSize);
    PhotonPvuScript* current = &_photonPvu.scripts.data[_photonPvu.scripts.size];
    current->isExecuting = desc->autostart;
    current->isSleeping = false;
    current->isFinished = false;
    current->sleepUntil = 0;
    current->memOffset = _photonPvu.totalScriptsSize;
    current->readOffset = 0;
    current->size = scriptSize;
    current->desc = *desc;
    _photonPvu.totalScriptsSize += scriptSize;
    _photonPvu.scripts.size++;

    return PhotonError_Ok;
}

PhotonError PhotonPvu_ExecCmd_SleepFor(int64_t delta)
{
    if (!_photonPvu.currentScript) {
        return PhotonError_BlockingCmdOutsidePvu;
    }
    if (delta == 0) {
        return PhotonError_Ok;
    }
    _photonPvu.currentScript->isSleeping = true;
    _photonPvu.currentScript->sleepUntil = PhotonClk_GetTime() + delta; //TODO: check for overflow
    return PhotonError_WouldBlock;
}

PhotonError PhotonPvu_ExecCmd_SleepUntil(uint64_t time)
{
    if (!_photonPvu.currentScript) {
        return PhotonError_BlockingCmdOutsidePvu;
    }
    PhotonClkTimePoint currentTime = PhotonClk_GetTime();
    if (time >= currentTime) {
        return PhotonError_Ok;
    }
    _photonPvu.currentScript->isSleeping = true;
    _photonPvu.currentScript->sleepUntil = time;
    return PhotonError_WouldBlock;
}

PhotonError PhotonPvu_ExecCmd_AddScript(const PhotonPvuScriptDesc* desc, const PhotonDynArrayOfU8MaxSize1024* body)
{
    //TODO: only allow in packet cmds
    return PhotonPvu_AddScript(desc, body);
}

PhotonError PhotonPvu_ExecCmd_RemoveScript(const PhotonDynArrayOfCharMaxSize16* name)
{
    for (size_t i = 0; i < _photonPvu.scripts.size; i++) {
        if (strcmp(_photonPvu.scripts.data[i].desc.name.data, name->data) == 0) {
            removeScript(i);
            return PhotonError_Ok;
        }
    }
    return PhotonError_InvalidValue;
}

PhotonError PhotonPvu_ExecCmd_StartScript(const PhotonDynArrayOfCharMaxSize16* name)
{
    for (size_t i = 0; i < _photonPvu.scripts.size; i++) {
        PhotonPvuScript* script = &_photonPvu.scripts.data[i];
        if (strcmp(script->desc.name.data, name->data) == 0) {
            if (script->isExecuting) {
                //already executing
                return PhotonError_InvalidValue;
            }
            script->isExecuting = true;
            script->isFinished = false;
            script->readOffset = 0;
            script->isSleeping = false;
            return PhotonError_Ok;
        }
    }
    return PhotonError_InvalidValue;
}

#undef _PHOTON_FNAME
