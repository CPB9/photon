#ifndef __PHOTON_CORE_TRY_H__
#define __PHOTON_CORE_TRY_H__

#define PHOTON_TRY(expr)              \
    do {                              \
        PhotonError _err = expr;      \
        if (_err != PhotonError_Ok) { \
            return _err;              \
        }                             \
    } while(0);

#endif
