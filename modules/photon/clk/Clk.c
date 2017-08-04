#include "photon/clk/Clk.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "clk/Clk.c"

#if defined(__unix__)

#include <sys/time.h>

static void  getTime(PhotonClkAbsoluteTime* dest)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    dest->seconds = t.tv_sec;
    dest->milliseconds = t.tv_usec / 1000;
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

static void getTime(PhotonClkAbsoluteTime* dest)
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
    dest->seconds = tmpres / 1000000UL;
    dest->milliseconds = (tmpres % 1000000UL) / 1000;
}

#endif

void PhotonClk_Init()
{
    getTime(&_photonClk.time);
    _photonClk.baseTime = _photonClk.time;
    _photonClk.clockResolution = 1;
    _photonClk.relativeTime = 0;
}

void PhotonClk_Tick()
{
    PhotonClkAbsoluteTime t;
    getTime(&t);
    _photonClk.time = t;
    t.seconds -= _photonClk.baseTime.seconds;
    if (t.milliseconds < _photonClk.baseTime.milliseconds) {
        t.seconds -= 1;
        t.milliseconds += 1000;
    }
    t.milliseconds -= _photonClk.baseTime.milliseconds;
    _photonClk.relativeTime = t.seconds * 1000 + t.milliseconds;
}

PhotonClkRelativeTime PhotonClk_CurrentTickTime()
{
    return _photonClk.relativeTime;
}

PhotonError PhotonClk_ResetBaseTime()
{
    getTime(&_photonClk.baseTime);
    return PhotonError_NotImplemented;
}
