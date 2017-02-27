#include "photon/tm/Tm.Component.h"

#include "photon/core/Error.h"
#include "photon/core/Reader.h"
#include "photon/core/Writer.h"
#include "photon/tm/MessageDesc.h"
#include "photon/tm/StatusMessage.h"

#include "photon/StatusEncoder.Private.h"
#include "photon/Tm.Private.inc.c"

void PhotonTm_Init()
{
    _tm.currentMsg = 0;
    _tm.allowedMsgCount = _PHOTON_TM_MSG_COUNT; //HACK
    _tm.msgs.size = _PHOTON_TM_MSG_COUNT;
    _tm.msgs.data = _messageDesc;
}

static void selectNextMessage()
{
    _tm.currentMsg++;
    if (_tm.currentMsg >= _PHOTON_TM_MSG_COUNT) {
        _tm.currentMsg = 0;
    }
}

static inline PhotonTmMessageDesc* currentDesc()
{
    return &_tm.msgs.data[_tm.currentMsg];
}

static PhotonError encodeStatusMsg(void* data, PhotonWriter* dest)
{
    (void)data;
    return currentDesc()->func(dest);
}

PhotonError PhotonTm_CollectMessages(PhotonWriter* dest)
{
    if (_tm.allowedMsgCount == 0) {
        return PhotonError_NoDataAvailable;
    }
    unsigned totalMessages = 0;
    while (true) {
        if (PhotonWriter_WritableSize(dest) < 2) {
            if (totalMessages == 0) {
                return PhotonError_NotEnoughSpace;
            }
            return PhotonError_Ok;
        }
        _tm.currentStatusMsg.compNum = currentDesc()->compNum;
        _tm.currentStatusMsg.msgNum = currentDesc()->msgNum;
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonError rv = PhotonTmStatusMessage_Encode(&_tm.currentStatusMsg, encodeStatusMsg, 0, dest);
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
    return _tm.allowedMsgCount != 0;
}

/*
PhotonError PhotonTm_CollectEventMessages(PhotonWriter* dest)
{
    (void)dest;
    // TODO
    return PhotonError_Ok;
}

// commands

PhotonGtTmCmdError PhotonGcMain_TmSendStatusMessage(PhotonGtCompMsg componentMessage)
{
    (void)componentMessage;
    // TODO
    return PHOTON_GT_TM_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_TmSetMessageRequest(PhotonGtCompMsg componentMessage, PhotonBer priority)
{
    if (componentMessage.messageNum >= _MSG_COUNT) {
        return PHOTON_GT_TM_CMD_ERROR_INVALID_MESSAGE_NUM;
    }
    _tm.descriptors[componentMessage.messageNum].priority = priority;
    return PHOTON_GT_TM_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_TmClearMessageRequest(PhotonGtCompMsg componentMessage)
{
    return PhotonGcMain_TmSetMessageRequest(componentMessage, 0);
}

static PhotonGtTmCmdError allowMessage(PhotonGtCompMsg componentMessage, bool isAllowed)
{
    if (componentMessage.messageNum >= _MSG_COUNT) {
        return PHOTON_GT_TM_CMD_ERROR_INVALID_MESSAGE_NUM;
    }
    _tm.descriptors[componentMessage.messageNum].isAllowed = isAllowed;
    if (isAllowed) {
        _tm.allowedMessages++;
    } else {
        _tm.allowedMessages--;
    }
    return PHOTON_GT_TM_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_TmDenyMessage(PhotonGtCompMsg componentMessage)
{
    return allowMessage(componentMessage, false);
}

PhotonGtTmCmdError PhotonGcMain_TmAllowMessage(PhotonGtCompMsg componentMessage)
{
    return allowMessage(componentMessage, true);
}

PhotonGtTmCmdError PhotonGcMain_TmDenyEvent(const PhotonGtEventInfo* eventInfo)
{
    (void)eventInfo;
    // TODO
    return PHOTON_GT_TM_CMD_ERROR_OK;
}

PhotonGtTmCmdError PhotonGcMain_TmAllowEvent(const PhotonGtEventInfo* eventInfo)
{
    (void)eventInfo;
    // TODO
    return PHOTON_GT_TM_CMD_ERROR_OK;
}

// params

PhotonBer PhotonGcMain_TmAllowedMessages()
{
    return _tm.allowedMessages;
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
