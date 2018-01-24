#ifndef __PHOTON_CORE_TRY_H__
#define __PHOTON_CORE_TRY_H__

#include "photongen/onboard/Config.h"

#define PHOTON_TRY(expr)              \
    do {                              \
        PhotonError _err = expr;      \
        if (_err != PhotonError_Ok) { \
            return _err;              \
        }                             \
    } while(0);

#define PHOTON_TRY_MSG(expr, msg)     \
    do {                              \
        PhotonError _err = expr;      \
        if (_err != PhotonError_Ok) { \
            PHOTON_WARNING(msg);      \
            return _err;              \
        }                             \
    } while(0);

#endif
