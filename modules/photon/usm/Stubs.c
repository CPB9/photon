#include "photongen/onboard/usm/Usm.Component.h"

#ifdef PHOTON_STUB

void PhotonUsm_Init()
{
}

void PhotonUsm_Tick()
{
}

PhotonError PhotonUsm_ExecCmd_GetFile(uint64_t fileId)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_UnpackFile()
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_BuildTarget()
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_InstallTarget()
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_UninstallApplication(const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_RunApplication(const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_StopApplication(const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_KillApplication(const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

#endif
