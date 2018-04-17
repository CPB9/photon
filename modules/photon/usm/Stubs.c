#include "photongen/onboard/usm/Usm.Component.h"

#ifdef PHOTON_STUB

void PhotonUsm_Init()
{
}

void PhotonUsm_Tick()
{
}

PhotonError PhotonUsm_ExecCmd_RequestStatus(PhotonUsmLanguageCodes language, uint8_t statusCode)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_GetFile(PhotonUsmLanguageCodes language, uint64_t fileId)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_UnpackFile(PhotonUsmLanguageCodes language)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_BuildTarget(PhotonUsmLanguageCodes language)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_InstallTarget(PhotonUsmLanguageCodes language)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_StopBuildProcess(PhotonUsmLanguageCodes language, bool kill)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_UninstallApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_RunApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize768* appNameWithArguments)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_StopApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_KillApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

PhotonError PhotonUsm_ExecCmd_RequestApplicationStatus(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize255* appName)
{
    return PhotonError_Ok;
}

#endif
