#include "photongen/onboard/tm/Tm.Component.h"

#include "photongen/onboard/core/Error.h"
#include "photongen/onboard/core/Reader.h"
#include "photongen/onboard/core/Writer.h"
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

void PhotonTm_Init()
{
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
    //TODO: implement
    PHOTON_WARNING("Event messages not implemented");
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

static PhotonError encodeStatusMsg(void* data, PhotonWriter* dest)
{
    (void)data;
    return currentDesc()->func(dest);
}

PhotonError PhotonTm_CollectMessages(PhotonWriter* dest)
{
    if (_photonTm.allowedMsgCount == 0) {
        return PhotonError_NoDataAvailable;
    }
    unsigned totalMessages = 0;

    //while (true) {
    //}

    while (true) {
        if (PhotonWriter_WritableSize(dest) < 2) {
            if (totalMessages == 0) {
                return PhotonError_NotEnoughSpace;
            }
            return PhotonError_Ok;
        }
        _photonTm.currentStatusMsg.compNum = currentDesc()->compNum;
        _photonTm.currentStatusMsg.msgNum = currentDesc()->msgNum;
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = PhotonTmStatusMessage_Encode(&_photonTm.currentStatusMsg, encodeStatusMsg, 0, dest);
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

bool PhotonTm_HasMessages() { return _photonTm.allowedMsgCount != 0; }

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

/*
PhotonError PhotonTm_CollectEventMessages(PhotonWriter* dest)
{
    (void)dest;
    // TODO
    return PhotonError_Ok;
}

// commands

PhotonGtTmCmdError PhotonGcMain_photonTmSendStatusMessage(PhotonGtCompMsg
componentMessage)
{
    (void)componentMessage;
    // TODO
    return PHOTON_GT_photonTm_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_photonTmSetMessageRequest(PhotonGtCompMsg
componentMessage, PhotonBer priority)
{
    if (componentMessage.messageNum >= _MSG_COUNT) {
        return PHOTON_GT_photonTm_CMD_ERROR_INVALID_MESSAGE_NUM;
    }
    _photonTm.descriptors[componentMessage.messageNum].priority = priority;
    return PHOTON_GT_photonTm_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_photonTmClearMessageRequest(PhotonGtCompMsg
componentMessage)
{
    return PhotonGcMain_photonTmSetMessageRequest(componentMessage, 0);
}

static PhotonGtTmCmdError allowMessage(PhotonGtCompMsg componentMessage, bool
isAllowed)
{
    if (componentMessage.messageNum >= _MSG_COUNT) {
        return PHOTON_GT_photonTm_CMD_ERROR_INVALID_MESSAGE_NUM;
    }
    _photonTm.descriptors[componentMessage.messageNum].isAllowed = isAllowed;
    if (isAllowed) {
        _photonTm.allowedMessages++;
    } else {
        _photonTm.allowedMessages--;
    }
    return PHOTON_GT_photonTm_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_photonTmDenyMessage(PhotonGtCompMsg
componentMessage)
{
    return allowMessage(componentMessage, false);
}

PhotonGtTmCmdError PhotonGcMain_photonTmAllowMessage(PhotonGtCompMsg
componentMessage)
{
    return allowMessage(componentMessage, true);
}

PhotonGtTmCmdError PhotonGcMain_photonTmDenyEvent(const PhotonGtEventInfo*
eventInfo)
{
    (void)eventInfo;
    // TODO
    return PHOTON_GT_photonTm_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_photonTmAllowEvent(const PhotonGtEventInfo*
eventInfo)
{
    (void)eventInfo;
    // TODO
    return PHOTON_GT_photonTm_CMD_ERROR_OK;
}

// params

PhotonBer PhotonGcMain_photonTmAllowedMessages()
{
    return _photonTm.allowedMessages;
}

// other

PhotonGtB8 PhotonGcMain_IsEventAllowed(PhotonBer messageId, PhotonBer eventId)
{
    (void)messageId;
    (void)eventId;
    // TODO
    return 0;
}
*/

#undef _PHOTON_FNAME
