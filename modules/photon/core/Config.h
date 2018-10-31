#ifndef __PHOTON_CONFIG_H__
#define __PHOTON_CONFIG_H__

#ifdef _MSC_VER
#define PHOTON_INLINE __inline
#define PHOTON_WEAK
#else
#define PHOTON_INLINE inline
#define PHOTON_WEAK __attribute__((weak))
#endif

#ifndef PHOTON_LOG_LEVEL
#define PHOTON_LOG_LEVEL 0
#endif

#endif
