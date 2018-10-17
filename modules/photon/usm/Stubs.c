#include "photongen/onboard/usm/Usm.Component.h"

#ifdef PHOTON_STUB

void PhotonUsm_Init()
{
    memset(&_photonUsm, 0, sizeof(_photonUsm));
    _photonUsm.applicationsInfoCpp.language = PhotonUsmLanguageCodes_LanguageCpp;
    _photonUsm.applicationsInfoPython.language = PhotonUsmLanguageCodes_LanguagePython;

    _photonUsm.buildStateCpp.language = PhotonUsmLanguageCodes_LanguageCpp;
    _photonUsm.buildStatePython.language = PhotonUsmLanguageCodes_LanguagePython;

    _photonUsm.currentApplicationInfoCpp.language = PhotonUsmLanguageCodes_LanguageCpp;
    _photonUsm.currentApplicationInfoPython.language = PhotonUsmLanguageCodes_LanguagePython;

    _photonUsm.projectInfo.language = PhotonUsmLanguageCodes_LanguageCpp;
}
void PhotonUsm_Tick() {}
void PhotonUsm_SetDir(const char* dir) {}

PhotonError PhotonUsm_ExecCmd_UpdateInfo(PhotonUsmLanguageCodes language, PhotonUsmFrames frame) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_UpdateInfoMultiple(PhotonUsmLanguageCodes language, uint64_t frames) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_ConfirmReceiving(PhotonUsmLanguageCodes language, PhotonUsmFrames frame) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_ConfirmReceivingMultiple(PhotonUsmLanguageCodes language, uint64_t frames) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_PrepareFileGetting(PhotonUsmLanguageCodes language, uint64_t fileId) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_GetFile(PhotonUsmLanguageCodes language, uint64_t fileId) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_UnpackFile(PhotonUsmLanguageCodes language) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_BuildTarget(PhotonUsmLanguageCodes language) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_InstallTarget(PhotonUsmLanguageCodes language) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_StopBuildProcess(PhotonUsmLanguageCodes language, bool kill) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_UninstallApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_RunApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName, const PhotonDynArrayOfCharMaxSize768* appNameWithArguments) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_StopApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_KillApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_RequestApplicationStatus(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_RequestProjectOfApplication(PhotonUsmLanguageCodes language, const PhotonDynArrayOfCharMaxSize32* appName) { return PhotonError_NotImplemented; }

#endif
