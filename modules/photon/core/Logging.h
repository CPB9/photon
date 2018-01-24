#ifndef __PHOTON_CORE_LOGGING_H__
#define __PHOTON_CORE_LOGGING_H__

#include "photongen/onboard/Config.h"

//TODO: find a better way to set _PHOTON_FNAME

#ifdef _PHOTON_FNAME
#error _PHOTON_FNAME defined before photon/Logging.h
#endif

#ifndef PHOTON_LOG_LEVEL
# define PHOTON_LOG_LEVEL 0
#endif

# define PHOTON_LOG_LEVEL_NONE 0
# define PHOTON_LOG_LEVEL_FATAL 1
# define PHOTON_LOG_LEVEL_CRITICAL 2
# define PHOTON_LOG_LEVEL_WARNING 3
# define PHOTON_LOG_LEVEL_INFO 4
# define PHOTON_LOG_LEVEL_DEBUG 5

#if PHOTON_LOG_LEVEL != PHOTON_LOG_LEVEL_NONE

# ifdef __cplusplus
extern "C" {
# endif

void Photon_SetLogDeviceName(const char* name);
void Photon_Log(int level, const char* fname, unsigned lineNum, const char* fmt, ...);

# ifdef __cplusplus
}
# endif

#endif

#if PHOTON_LOG_LEVEL >= PHOTON_LOG_LEVEL_FATAL //fatal
# define PHOTON_FATAL(...) Photon_Log(PHOTON_LOG_LEVEL_FATAL, _PHOTON_FNAME, __LINE__, __VA_ARGS__)
#else
# define PHOTON_FATAL(...)
#endif

#if PHOTON_LOG_LEVEL >= PHOTON_LOG_LEVEL_CRITICAL //critical
# define PHOTON_CRITICAL(...) Photon_Log(PHOTON_LOG_LEVEL_CRITICAL, _PHOTON_FNAME, __LINE__, __VA_ARGS__)
#else
# define PHOTON_CRITICAL(...)
#endif

#if PHOTON_LOG_LEVEL >= PHOTON_LOG_LEVEL_WARNING //warning
# define PHOTON_WARNING(...) Photon_Log(PHOTON_LOG_LEVEL_WARNING, _PHOTON_FNAME, __LINE__, __VA_ARGS__)
#else
# define PHOTON_WARNING(...)
#endif

#if PHOTON_LOG_LEVEL >= PHOTON_LOG_LEVEL_INFO //info
# define PHOTON_INFO(...) Photon_Log(PHOTON_LOG_LEVEL_INFO, _PHOTON_FNAME, __LINE__, __VA_ARGS__)
#else
# define PHOTON_INFO(...)
#endif

#if PHOTON_LOG_LEVEL >= PHOTON_LOG_LEVEL_DEBUG //debug
# define PHOTON_DEBUG(...) Photon_Log(PHOTON_LOG_LEVEL_DEBUG, _PHOTON_FNAME, __LINE__, __VA_ARGS__)
#else
# define PHOTON_DEBUG(...)
#endif

#endif
