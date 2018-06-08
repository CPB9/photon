#include "photongen/onboard/usm/Usm.Component.h"

#ifdef PHOTON_STUB

void PhotonUsm_Init() {}
void PhotonUsm_Tick() {}
void PhotonUsm_SetDir(const char* dir) {}

PhotonError PhotonUsm_ExecCmd_UpdateInfo(PhotonUsmLanguageCodes language, PhotonUsmFrames frame) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_UpdateInfoMultiple(PhotonUsmLanguageCodes language, uint64_t frames) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_ConfirmReceiving(PhotonUsmLanguageCodes language, PhotonUsmFrames frame) { return PhotonError_NotImplemented; }
PhotonError PhotonUsm_ExecCmd_ConfirmReceivingMultiple(PhotonUsmLanguageCodes language, uint64_t frames) { return PhotonError_NotImplemented; }
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
