#include "photon/fl/Fl.Component.h"

#ifdef PHOTON_STUB

PhotonError PhotonFl_BeginFile(uint64_t id, uint64_t size)
{
    return PhotonError_Ok;
}

PhotonError PhotonFl_WriteFile(uint64_t id, uint64_t offset, const PhotonDynArrayOfU8MaxSize128* chunk)
{
    return PhotonError_Ok;
}

PhotonError PhotonFl_EndFile(uint64_t id, uint64_t size, PhotonFlChecksum* rv)
{
    return PhotonError_Ok;
}

#endif
