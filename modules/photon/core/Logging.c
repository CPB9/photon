#include "photon/core/Logging.h"
#include "photon/core/Util.h"

#ifdef PHOTON_LOG_LEVEL
# if PHOTON_LOG_LEVEL >= 1

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#ifdef __unix__
#include <unistd.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#if !defined(alloca)
#define alloca _alloca
#endif
#else
#include <alloca.h>
#endif

static const char* deviceName = "";
static size_t deviceNameSize = 0;

static inline bool supportsColor()
{
#ifdef __unix__
    return isatty(STDOUT_FILENO);
#endif
    return false;
}

void Photon_SetLogDeviceName(const char* name)
{
    if (name) {
        deviceName = name;
        deviceNameSize = strlen(name);
    } else {
        deviceName = 0;
        deviceNameSize = 0;
    }
}

//TODO: refact

void Photon_Log(int level, const char* fname, unsigned lineNum, const char* fmt, ...)
{
    const char* levelStr;
    const char* levelPrefix;
    const char* levelPostfix;
    const char* levelAlign;
    switch (level) {
    case PHOTON_LOG_LEVEL_FATAL:
        levelStr = "FATAL";
        levelPrefix = "\x1b[1;5;31m";
        levelPostfix = "\x1b[0m:";
        levelAlign = "   ";
        break;
    case PHOTON_LOG_LEVEL_CRITICAL:
        levelStr = "CRITICAL";
        levelPrefix = "\x1b[31m";
        levelPostfix = "\x1b[0m:";
        levelAlign = "";
        break;
    case PHOTON_LOG_LEVEL_WARNING:
        levelStr = "WARNING";
        levelPrefix = "\x1b[1;33m";
        levelPostfix = "\x1b[0m:";
        levelAlign = " ";
        break;
    case PHOTON_LOG_LEVEL_INFO:
        levelStr = "INFO";
        levelPrefix = "\x1b[1;36m";
        levelPostfix = "\x1b[0m:";
        levelAlign = "    ";
        break;
    case PHOTON_LOG_LEVEL_DEBUG:
        levelStr = "DEBUG";
        levelPrefix = "\x1b[1;30m";
        levelPostfix = "\x1b[0m:";
        levelAlign = "   ";
        break;
    default:
        return;
    }

    char timeStr[20];
    timeStr[19] = '\0';
    time_t t = time(NULL);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&t));

    size_t alignSize = 40;
    size_t fnameLen = strlen(fname);
    size_t devSize = 0;
    if (deviceNameSize) {
        devSize = deviceNameSize + 1;
    }
    size_t modLen = PHOTON_MAX(alignSize + 1, devSize + fnameLen + 1 + sizeof(lineNum) * 4 + 1);
    char* modStr = (char*)alloca(modLen);
    char* fillEnd = modStr + modLen;
    memcpy(modStr, deviceName, deviceNameSize);
    memcpy(modStr + devSize, fname, fnameLen);
    modStr[devSize + fnameLen] = ':';
    if (deviceNameSize) {
        modStr[deviceNameSize] = ':';
    }
    char* numBegin = modStr + devSize + fnameLen + 1;
    numBegin += sprintf(numBegin, "%u", lineNum);
    while (numBegin < fillEnd) {
        *numBegin = ' ';
        numBegin++;
    }
    modStr[modLen - 1] = '\0';

    bool hasColor = supportsColor();
    if (hasColor) {
        printf("\x1b[0m\x1b[1m%s\x1b[0m [%s] %s%s%s%s ", timeStr, modStr, levelPrefix, levelStr, levelAlign, levelPostfix);
    } else {
        printf("%s [%s] %s:%s ", timeStr, modStr, levelStr, levelAlign);
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

# endif
#endif
