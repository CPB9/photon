#include "photon/core/Reader.h"
#include "photon/core/Writer.h"
#include "photon/core/Error.h"
#include "photon/tm/MessageDesc.h"
#include "photon/tm/Tm.Component.h"

#include "photon/tm/Tm.Component.inc.c"
#include "photon/TmPrivate.inc.c"

void PhotonTm_Init()
{
    _tm.currentMessage = 0;
    _tm.allowedMessagesCount = _PHOTON_TM_MSG_COUNT;
}

static void selectNextMessage()
{
    _tm.currentMessage++;
    if (_tm.currentMessage >= _PHOTON_TM_MSG_COUNT) {
        _tm.currentMessage = 0;
    }
}
/*
PhotonError PhotonTm_CollectStatusMessages(PhotonWriter* dest)
{
    if (_tm.allowedMessages == 0) {
        return PhotonError_Ok;
    }
    unsigned totalMessages = 0;
    while (true) {
        _tm.statusEnc.msg.messageNumber = _tm.currentMessage;
        //FIXME: set component number
        if (PhotonWriter_WritableSize(dest) < 2) {
            return PhotonError_NotEnoughSpace;
        }
        uint8_t* current = PhotonWriter_CurrentPtr(dest);
        PhotonWriter_WriteU16Be(dest, PHOTON_TM_STREAM_SEPARATOR);
        PhotonError rv = PhotonEncoder_EncodeTmStatusMessage(&_tm.statusEnc, dest);
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
}*/

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
