#include "photon/tm/Tm.Component.h"

#include "photon/core/Error.h"
#include "photon/core/Reader.h"
#include "photon/core/Writer.h"
#include "photon/tm/MessageDesc.h"
#include "photon/tm/StatusMessage.h"

#include "photon/StatusEncoder.Private.h"
#include "photon/Tm.Private.inc.c"

#define TM_MSG_BEGIN &_messageDesc[0]
#define TM_MSG_END &_messageDesc[_PHOTON_TM_MSG_COUNT]

#define TM_MAX_ONCE_REQUESTS 16 // TODO: generate

static uint16_t _onceRequests[TM_MAX_ONCE_REQUESTS];

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
    _photonTm.msgs.size = _PHOTON_TM_MSG_COUNT;
    _photonTm.msgs.data = _messageDesc;
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
    return &_photonTm.msgs.data[_photonTm.currentDesc];
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
        PhotonError rv = PhotonTmStatusMessage_Encode(&_photonTm.currentStatusMsg,
            encodeStatusMsg, 0, dest);
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

PhotonError PhotonTm_SetStatusEnabled(uint8_t compNum, uint8_t msgNum,
    bool isEnabled)
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
