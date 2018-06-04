#include "photongen/onboard/fcu/Fcu.Component.h"

#include "photon/core/Logging.h"

#define _PHOTON_FNAME "fcu/Fcu.c"

extern void setArmed(bool flag);

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

#ifdef PHOTON_STUB

void setArmed(bool flag)
{

}

PhotonError PhotonFcu_ExecCmd_StartSensorCalibration(PhotonFcuSensors sensor)
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_CancelSensorCalibration()
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_WriteRadioCalibrationRatios(PhotonFcuRcRatio rcRatios[18], PhotonFcuRcCalFunction calFunctions[4])
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_SetActive(bool active)
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_TakeOff()
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_Land()
{
    return PhotonError_Ok;
}

PhotonError PhotonFcu_ExecCmd_SetMode(const PhotonDynArrayOfCharMaxSize256* mode)
{
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
