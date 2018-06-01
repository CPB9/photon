#include "photongen/onboard/px4/Px4.Component.h"

#include "photon/core/Logging.h"

#define _PHOTON_FNAME "px4/Px4.c"

extern void setArmed();

void PhotonFcu_Init()
{
}

void PhotonFcu_Tick()
{
}

PhotonError PhotonFcu_ExecCmd_Arm()
{
    setArmed(true);
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_Disarm()
{
    setArmed(false);
    return PhotonError_Ok;
}

#undef _PHOTON_FNAME
