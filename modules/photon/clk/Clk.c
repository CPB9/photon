#include "photon/clk/Clk.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "clk/Clk.c"

#if defined(__unix__)

#include <sys/time.h>

static void  getTime(PhotonClkTick* dest)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    *dest = t.tv_sec * 1000 + t.tv_usec / 1000;
}

#elif defined(_WIN32)

// https://social.msdn.microsoft.com/Forums/vstudio/en-US/430449b3-f6dd-4e18-84de-eebd26a8d668/gettimeofday?forum=vcgeneral
#include <time.h>
#include <windows.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
# define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
# define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

static void getTime(PhotonClkTimePoint* dest)
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
    *dest = seconds * 1000 + milliseconds;
}

#endif

void PhotonClk_Init()
{
    getTime(&_photonClk.time);
    _photonClk.timeKind.type = PhotonClkTimeKindType_Absolute;
    _photonClk.timeKind.data.absoluteTimeKind.epoch = 0;
}

void PhotonClk_Tick()
{
    getTime(&_photonClk.time);
}

PhotonClkTick PhotonClk_GetTime()
{
    return _photonClk.time;
}

#undef _PHOTON_FNAME
