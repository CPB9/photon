#include "photongen/onboard/clk/Clk.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "clk/Clk.c"

#if defined(__unix__)

#include <sys/time.h>

PhotonClkDuration PhotonClk_GetRawSystemTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

#elif defined(_WIN32)

// https://social.msdn.microsoft.com/Forums/vstudio/en-US/430449b3-f6dd-4e18-84de-eebd26a8d668/gettimeofday?forum=vcgeneral
#include <time.h>
#include <windows.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
# define DELTA_EPOCH_IN_MICROSECS  116444736000000000Ui64
#else
# define DELTA_EPOCH_IN_MICROSECS  116444736000000000ULL
#endif

PhotonClkDuration PhotonClk_GetRawSystemTime()
{
    FILETIME ft;
    uint64_t tmpres = 0;

    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    /*convert into milliseconds*/
    tmpres /= 10;
    uint64_t seconds = tmpres / 1000000UL;
    uint64_t milliseconds = (tmpres % 1000000UL) / 1000;
    return seconds * 1000 + milliseconds;
}

#endif

void PhotonClk_Init()
{
    _photonClk.correction = 0;
    _photonClk.tickTime = PhotonClk_GetTime();
}

void PhotonClk_Tick()
{
    _photonClk.tickTime = PhotonClk_GetTime();
}

PhotonClkTimePoint PhotonClk_GetTickTime()
{
    return _photonClk.tickTime;
}

PhotonClkTimePoint PhotonClk_GetTime()
{
    return PhotonClk_GetRawSystemTime() + _photonClk.correction;
}

PhotonError PhotonClk_ExecCmd_SetTimeCorrection(int64_t delta)
{
#ifdef PHOTON_HAS_MODULE_TM
    PhotonClkDuration rawTime = PhotonClk_GetRawSystemTime();
    PhotonClkTimePoint oldTime = rawTime + _photonClk.correction;
    PhotonClkTimePoint newTime = rawTime + delta;
    _photonClk.correction = delta;
    PhotonClk_QueueEvent_TimeCorrected(rawTime, oldTime, newTime); //TODO: handle error
#else
    _photonClk.correction = delta;
#endif
    return PhotonError_Ok;
}

#undef _PHOTON_FNAME
