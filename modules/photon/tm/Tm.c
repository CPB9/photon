#include "photongen/onboard/tm/Tm.Component.h"

#include "photongen/onboard/core/Error.h"
#include "photongen/onboard/core/Reader.h"
#include "photongen/onboard/core/Writer.h"
#include "photongen/onboard/core/RingBuf.h"
#include "photon/core/Logging.h"
#include "photon/core/Try.h"
#include "photongen/onboard/tm/MessageDesc.h"
#include "photongen/onboard/tm/StatusMessage.h"

#ifdef PHOTON_HAS_MODULE_BLOG
# include "photongen/onboard/blog/Blog.Component.h"
#endif

#include "photongen/onboard/StatusTable.inc.c"

#define _PHOTON_FNAME "tm/Tm.c"

#define TM_MSG_BEGIN &_messageDesc[0]
#define TM_MSG_END &_messageDesc[_PHOTON_TM_MSG_COUNT]

#define TM_MAX_ONCE_REQUESTS 16 // TODO: generate

static uint16_t _onceRequests[TM_MAX_ONCE_REQUESTS];
static uint8_t eventTmp[1024];
static PhotonWriter eventWriter;
static uint8_t _eventData[2048];
static PhotonRingBuf _eventRingBuf;

void PhotonTm_Init()
{
    _photonTm.lostEvents = 0;
    _photonTm.generatedEvents = 0;
    _photonTm.currentDesc = 0;
    size_t allowedMsgCount = 0;
    for (PhotonTmMessageDesc* it = TM_MSG_BEGIN; it < TM_MSG_END; it++) {
        if (it->isEnabled) {
            allowedMsgCount++;
        }
    }
    _photonTm.allowedMsgCount = allowedMsgCount;
    _photonTm.onceRequestsNum = 0;
    PhotonWriter_Init(&eventWriter, eventTmp, sizeof(eventTmp));
    PhotonRingBuf_Init(&_eventRingBuf, _eventData, sizeof(_eventData));
}

void PhotonTm_Tick()
{
}

PhotonWriter* PhotonTm_BeginEventMsg(uint8_t compNum, uint8_t msgNum)
{
    eventWriter.current = eventWriter.start;
    PhotonWriter_WriteU8(&eventWriter, compNum);
    PhotonWriter_WriteU8(&eventWriter, msgNum);
    return &eventWriter;
}

void PhotonTm_EndEventMsg()
{
    //TODO: check overflow
    uint16_t msgSize = eventWriter.current - eventWriter.start;

#ifdef PHOTON_HAS_MODULE_BLOG
    PhotonBlog_LogTmMsg(eventWriter.start, msgSize);
#endif

    size_t writableSize = PhotonRingBuf_WritableSize(&_eventRingBuf);
    while ((msgSize + 2u) > writableSize) {
        PHOTON_DEBUG("removing event to fit new");
        _photonTm.lostEvents++;
        _photonTm.storedEvents--;
        uint16_t currentSize;
        PhotonRingBuf_Peek(&_eventRingBuf, &currentSize, 2, 0);
        //TODO: check available size before erase
        PhotonRingBuf_Erase(&_eventRingBuf, currentSize + 2);
        writableSize = PhotonRingBuf_WritableSize(&_eventRingBuf);
    }
    PhotonRingBuf_Write(&_eventRingBuf, &msgSize, 2);
    PhotonRingBuf_Write(&_eventRingBuf, eventWriter.start, msgSize);
    _photonTm.storedEvents++;
    _photonTm.generatedEvents++;
}

static void selectNextMessage()
{
    _photonTm.currentDesc++;
    if (_photonTm.currentDesc >= _PHOTON_TM_MSG_COUNT) {
        _photonTm.currentDesc = 0;
    }
}

static inline PhotonTmMessageDesc* currentDesc()
{
    return &_messageDesc[_photonTm.currentDesc];
}

static void popOnceRequests(size_t num)
{
    if (num >= _photonTm.onceRequestsNum) {
        _photonTm.onceRequestsNum = 0;
        return;
    }
    size_t delta = _photonTm.onceRequestsNum - num;
    for (size_t i = 0; i < delta; i++) {
        _onceRequests[i] = _onceRequests[i + num];
    }
    _photonTm.onceRequestsNum -= num;
}

static PhotonError collectEvents(PhotonWriter* dest, unsigned* totalMessages)
{
    while (true) {
        if (PhotonRingBuf_ReadableSize(&_eventRingBuf) == 0) {
            break;
        }
        uint16_t currentSize;
        PhotonRingBuf_Peek(&_eventRingBuf, &currentSize, 2, 0);
        //TODO: check available size before peak
        if (currentSize > PhotonWriter_WritableSize(dest)) {
            if (*totalMessages == 0) {
                PHOTON_CRITICAL("unable to fit event, skipping");
                PhotonRingBuf_Erase(&_eventRingBuf, currentSize + 2);
                _photonTm.storedEvents--;
                _photonTm.lostEvents++;
            }
            break;
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonRingBuf_Peek(&_eventRingBuf, current, currentSize, 2);
        PhotonWriter_SetCurrentPtr(dest, current + currentSize);
        PhotonRingBuf_Erase(&_eventRingBuf, currentSize + 2);
        _photonTm.storedEvents--;
        (*totalMessages)++;
    }
    return PhotonError_Ok;
}

static PhotonError collectOnceRequests(PhotonWriter* dest, unsigned* totalMessages)
{
    if (_photonTm.onceRequestsNum > TM_MAX_ONCE_REQUESTS) {
        _photonTm.onceRequestsNum = TM_MAX_ONCE_REQUESTS;
        PHOTON_CRITICAL("once requests overflow");
    }

    size_t i;
    for (i = 0; i < _photonTm.onceRequestsNum; i++) {
        size_t currentId = _onceRequests[i];
        if (currentId >= _PHOTON_TM_MSG_COUNT) {
            PHOTON_CRITICAL("invalid msg id in once requests (%u)", currentId);
            continue;
        }
        PhotonTmMessageDesc* desc = &_messageDesc[currentId];
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = desc->func(dest);
        if (rv == PhotonError_Ok) {
#ifdef PHOTON_HAS_MODULE_BLOG
            PhotonBlog_LogTmMsg(current, dest->current - current);
#endif
            (*totalMessages)++;
            continue;
        } else if (rv == PhotonError_NotEnoughSpace) {
            PhotonWriter_SetCurrentPtr(dest, current);
            if (*totalMessages == 0) {
                PHOTON_CRITICAL("unable to fit once request (%u, %u), skipping",
                                (unsigned)desc->compNum,
                                (unsigned)desc->msgNum);
                continue;
            }
            break;
        } else {
            PhotonWriter_SetCurrentPtr(dest, current);
            PHOTON_CRITICAL("unable to serialize request (%u, %u), skipping",
                            (unsigned)desc->compNum,
                            (unsigned)desc->msgNum);
            continue;
        }
    }
    popOnceRequests(i);
    return PhotonError_Ok;
}

static PhotonError collectStatuses(PhotonWriter* dest, unsigned* totalMessages)
{
    if (_photonTm.allowedMsgCount == 0) {
        return PhotonError_Ok;
    }

    for (size_t i = 0; i < _PHOTON_TM_MSG_COUNT; i++) {
        if (!currentDesc()->isEnabled) {
            selectNextMessage();
            continue;
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = currentDesc()->func(dest);
        if (rv == PhotonError_Ok) {
#ifdef PHOTON_HAS_MODULE_BLOG
            PhotonBlog_LogTmMsg(current, dest->current - current);
#endif
            selectNextMessage();
            (*totalMessages)++;
            continue;
        } else if (rv == PhotonError_NotEnoughSpace) {
            PhotonWriter_SetCurrentPtr(dest, current);
            if (*totalMessages == 0) {
                PHOTON_CRITICAL("unable to fit status (%u, %u), skipping",
                                (unsigned)currentDesc()->compNum,
                                (unsigned)currentDesc()->msgNum);
                selectNextMessage();
                continue;
            }
            return PhotonError_Ok;
        } else {
            PhotonWriter_SetCurrentPtr(dest, current);
            PHOTON_CRITICAL("unable to serialize status (%u, %u), skipping",
                            (unsigned)currentDesc()->compNum,
                            (unsigned)currentDesc()->msgNum);
            selectNextMessage();
            continue;
        }
    }
    return PhotonError_Ok;
}

PhotonError PhotonTm_CollectMessages(PhotonWriter* dest)
{
    static unsigned tick = 0;

    unsigned totalMessages = 0;

    switch (tick) {
    case 0:
        PHOTON_TRY(collectEvents(dest, &totalMessages));
        PHOTON_TRY(collectOnceRequests(dest, &totalMessages));
        PHOTON_TRY(collectStatuses(dest, &totalMessages));
        tick = 1;
        break;
    case 1:
        PHOTON_TRY(collectStatuses(dest, &totalMessages));
        PHOTON_TRY(collectEvents(dest, &totalMessages));
        PHOTON_TRY(collectOnceRequests(dest, &totalMessages));
        tick = 2;
        break;
    case 2:
        PHOTON_TRY(collectOnceRequests(dest, &totalMessages));
        PHOTON_TRY(collectStatuses(dest, &totalMessages));
        PHOTON_TRY(collectEvents(dest, &totalMessages));
        tick = 0;
        break;
    default:
        //error
        tick = 0;
    }

    if (totalMessages == 0) {
        return PhotonError_NoDataAvailable;
    }
    return PhotonError_Ok;
}

bool PhotonTm_HasMessages()
{
    return (_photonTm.allowedMsgCount > 0) || (PhotonRingBuf_ReadableSize(&_eventRingBuf) > 0) || (_photonTm.onceRequestsNum > 0);
}

PhotonError PhotonTm_SetStatusEnabled(uint8_t compNum, uint8_t msgNum, bool isEnabled)
{
    for (PhotonTmMessageDesc* it = TM_MSG_BEGIN; it < TM_MSG_END; it++) {
        if (it->compNum == compNum && it->msgNum == msgNum) {
            if (it->isEnabled != isEnabled) {
                if (isEnabled) {
                    _photonTm.allowedMsgCount++;
                } else {
                    _photonTm.allowedMsgCount--;
                }
                it->isEnabled = isEnabled;
            }
            return PhotonError_Ok;
        }
    }
    return PhotonError_NoSuchStatusMsg;
}

PhotonError PhotonTm_RequestStatusOnce(uint8_t compNum, uint8_t msgNum)
{
    if (_photonTm.onceRequestsNum == TM_MAX_ONCE_REQUESTS) {
        return PhotonError_MaximumOnceRequestsReached;
    }
    for (PhotonTmMessageDesc* it = TM_MSG_BEGIN; it < TM_MSG_END; it++) {
        if (it->compNum == compNum && it->msgNum == msgNum) {
            size_t id = it - TM_MSG_BEGIN;
            for (size_t i = 0; i < _photonTm.onceRequestsNum; i++) {
                if (_onceRequests[i] == id) {
                    PHOTON_DEBUG("once request already set (%u, %u)", (unsigned)it->compNum, (unsigned)it->msgNum);
                    return PhotonError_Ok;
                }
            }
            PHOTON_DEBUG("setting once request (%u, %u)", (unsigned)it->compNum, (unsigned)it->msgNum);
            _onceRequests[_photonTm.onceRequestsNum] = id;
            _photonTm.onceRequestsNum++;
            return PhotonError_Ok;
        }
    }
    return PhotonError_NoSuchStatusMsg;
}

PhotonError PhotonTm_ExecCmd_RequestStatusOnce(uint8_t compNum, uint8_t msgNum)
{
    return PhotonTm_RequestStatusOnce(compNum, msgNum);
}

PhotonError PhotonTm_ExecCmd_SetStatusEnabled(uint8_t compNum, uint8_t msgNum, bool isEnabled)
{
    return PhotonTm_SetStatusEnabled(compNum, msgNum, isEnabled);
}

#undef _PHOTON_FNAME
