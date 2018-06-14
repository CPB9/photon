#include "photongen/onboard/fcu/Fcu.Component.h"

#include "photon/core/Logging.h"

#define _PHOTON_FNAME "fcu/Fcu.c"

extern void setArmed(bool flag);

void PhotonFcu_Init()
{
    _photonFcu.commonCalibrationState.accelerometer = false;
    _photonFcu.commonCalibrationState.magnetometer = false;
    _photonFcu.commonCalibrationState.gyroscope = false;
    _photonFcu.commonCalibrationState.level = false;
    _photonFcu.commonCalibrationState.esc = false;
    _photonFcu.commonCalibrationState.radio = false;
}

void PhotonFcu_Tick()
{
#ifndef PHOTON_STUB
    _photonFcu.commonCalibrationState.magnetometer = (_photonPx4_autogen._Sensor_Calibration.CAL_MAG0_ID != 0);
    _photonFcu.commonCalibrationState.accelerometer = (_photonPx4_autogen._Sensor_Calibration.CAL_ACC0_ID != 0);
    _photonFcu.commonCalibrationState.gyroscope = (_photonPx4_autogen._Sensor_Calibration.CAL_GYRO0_ID != 0);
    _photonFcu.commonCalibrationState.level = (_photonPx4_autogen._Sensor_Calibration.SENS_BOARD_X_OFF != 0);
    _photonFcu.commonCalibrationState.radio = (    _photonPx4_autogen._Radio_Calibration.RC_MAP_ROLL != 0 &&
                                                   _photonPx4_autogen._Radio_Calibration.RC_MAP_PITCH != 0 &&
                                                   _photonPx4_autogen._Radio_Calibration.RC_MAP_YAW != 0 &&
                                                   _photonPx4_autogen._Radio_Calibration.RC_MAP_THROTTLE != 0);
#endif
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

PhotonError PhotonFcu_ExecCmd_WriteRadioCalibrationRatios(PhotonFcuRcRatio rcRatios[18], uint8_t rollChannel, uint8_t pitchChannel, uint8_t yawChannel, uint8_t throttleChannel)
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

PhotonError PhotonFcu_ExecCmd_SetSimpleFlightModes(PhotonFcuRcChannel modeChannel, PhotonFcuRcChannel returnChannel, PhotonFcuRcChannel killSwitchChannel, PhotonFcuRcChannel offboardSwitchChannel, PhotonFcuFlightMode flightModes[6])
{
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
