#include "photon/int/Int.Component.h"

#include "photon/core/Try.h"
#include "photon/core/Logging.h"
#include "photon/CmdDecoder.Private.h"

#define _PHOTON_FNAME "int/Int.c"

void PhotonInt_Init()
{
}

void PhotonInt_Tick()
{
}

PhotonError PhotonInt_ExecuteFrom(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* results)
{
    (void)header;
    if (PhotonReader_ReadableSize(src) == 0) {
        return PhotonError_Ok;
    }
    while (PhotonReader_ReadableSize(src) != 0) {
        if (PhotonReader_ReadableSize(src) < 2) {
            PHOTON_CRITICAL("Unable to decode cmd header");
            return PhotonError_NotEnoughData;
        }

        uint8_t compNum = PhotonReader_ReadU8(src);
        uint8_t cmdNum = PhotonReader_ReadU8(src);

        PHOTON_TRY(Photon_ExecCmd(compNum, cmdNum, src, results));
    }

    return PhotonError_Ok;
}

#undef _PHOTON_FNAME
