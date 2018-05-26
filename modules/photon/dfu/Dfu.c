#include "photongen/onboard/dfu/Dfu.Component.h"

#include "photon/core/Assert.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "fwt/Dfu.c"

void PhotonDfu_Init()
{
}

void PhotonDfu_Tick()
{
}

PhotonError PhotonDfu_AcceptCmd(const PhotonExcDataHeader* header, PhotonReader* src, PhotonWriter* dest)
{
    return PhotonError_NotImplemented;
}

PhotonError PhotonDfu_GenAnswer(PhotonWriter* dest)
{
    return PhotonError_NotImplemented;
}

bool PhotonDfu_HasAnswers()
{
    return false;
}

#undef _PHOTON_FNAME
