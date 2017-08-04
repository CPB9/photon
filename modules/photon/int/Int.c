#include "photon/int/Int.Component.h"

#include "photon/core/Try.h"
#include "photon/CmdDecoder.Private.h"

void PhotonInt_Init()
{
}

void PhotonInt_Tick()
{
}

PhotonError PhotonInt_ExecuteFrom(PhotonReader* src, PhotonWriter* results)
{
    while (PhotonReader_ReadableSize(src) != 0) {
        if (PhotonReader_ReadableSize(src) < 2) {
            return PhotonError_NotEnoughData;
        }

        uint8_t compNum = PhotonReader_ReadU8(src);
        uint8_t cmdNum = PhotonReader_ReadU8(src);

        PHOTON_TRY(Photon_ExecCmd(compNum, cmdNum, src, results));
    }

    return PhotonError_Ok;
}
