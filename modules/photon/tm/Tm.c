#include "photongen/onboard/tm/Tm.Component.h"

#include "photongen/onboard/core/Error.h"
#include "photongen/onboard/core/Reader.h"
#include "photongen/onboard/core/Writer.h"
#include "photongen/onboard/core/RingBuf.h"
#include "photon/core/Logging.h"
#include "photongen/onboard/tm/MessageDesc.h"
#include "photongen/onboard/tm/StatusMessage.h"

#include "photongen/onboard/StatusTable.inc.c"

#define _PHOTON_FNAME "tm/Tm.c"

#define TM_MSG_BEGIN &_messageDesc[0]
#define TM_MSG_END &_messageDesc[_PHOTON_TM_MSG_COUNT]

#define TM_MAX_ONCE_REQUESTS 16 // TODO: generate

static uint16_t _onceRequests[TM_MAX_ONCE_REQUESTS];
static uint8_t eventTmp[512];
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
    size_t writableSize = PhotonRingBuf_WritableSize(&_eventRingBuf);
    while ((msgSize + 2) > writableSize) {
        _photonTm.lostEvents++;
        _photonTm.storedEvents--;
        uint16_t currentSize;
        PhotonRingBuf_Read(&_eventRingBuf, &currentSize, 2);
        //TODO: check available size before erase
        PhotonRingBuf_Erase(&_eventRingBuf, currentSize);
        writableSize -= currentSize + 2;
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

PhotonError PhotonTm_CollectMessages(PhotonWriter* dest)
{
    unsigned totalMessages = 0;

    while (true) {
        if (PhotonRingBuf_ReadableSize(&_eventRingBuf) == 0) {
            break;
        }
        uint16_t currentSize;
        PhotonRingBuf_Peek(&_eventRingBuf, &currentSize, 2, 0);
        //TODO: check available size before peak
        if (currentSize > PhotonWriter_WritableSize(dest)) {
            break;
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonRingBuf_Peek(&_eventRingBuf, current, currentSize, 2);
        PhotonWriter_SetCurrentPtr(dest, current + currentSize);
        PhotonRingBuf_Erase(&_eventRingBuf, currentSize + 2);
        _photonTm.storedEvents--;
    }

    if (_photonTm.onceRequestsNum > TM_MAX_ONCE_REQUESTS) {
        _photonTm.onceRequestsNum = TM_MAX_ONCE_REQUESTS;
        //TODO: report error
    }

    if (_photonTm.allowedMsgCount == 0) {
        return PhotonError_NoDataAvailable;
    }

    size_t i;
    for (i = 0; i < _photonTm.onceRequestsNum; i++) {
        size_t currentId = _onceRequests[i];
        if (currentId >= _PHOTON_TM_MSG_COUNT) {
            i++;
            continue;
            //TODO: report error
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = _messageDesc[currentId].func(dest);
        if (rv == PhotonError_Ok) {
            totalMessages++;
            continue;
        } else if (rv == PhotonError_NotEnoughSpace) {
            PhotonWriter_SetCurrentPtr(dest, current);
            if (totalMessages == 0) {
                popOnceRequests(i);
                return PhotonError_NotEnoughSpace;
            }
            popOnceRequests(i);
            return PhotonError_Ok;
        } else {
            popOnceRequests(i);
            return rv;
        }
    }
    popOnceRequests(i);

    while (true) {
        while (!currentDesc()->isEnabled) {
            selectNextMessage();
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = currentDesc()->func(dest);
        if (rv == PhotonError_Ok) {
            selectNextMessage();
            totalMessages++;
            continue;
        } else if (rv == PhotonError_NotEnoughSpace) {
            PhotonWriter_SetCurrentPtr(dest, current);
            if (totalMessages == 0) {
                return PhotonError_NotEnoughSpace;
            }
            return PhotonError_Ok;
        } else {
            return rv;
        }
    }
}

bool PhotonTm_HasMessages()
{
    return _photonTm.allowedMsgCount != 0;
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
            _onceRequests[_photonTm.onceRequestsNum] = it - TM_MSG_BEGIN;
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
