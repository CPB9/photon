#include "photongen/onboard/fl/Fl.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "fl/Fl.c"

#ifdef PHOTON_STUB

PhotonError PhotonFl_ExecCmd_BeginFile(uint64_t id, uint64_t size)
{
    PHOTON_INFO("Begin file id(%" PRIu64 "), size(%" PRIu64 ")", id, size);
    return PhotonError_Ok;
}

PhotonError PhotonFl_ExecCmd_WriteFile(uint64_t id, uint64_t offset, const PhotonDynArrayOfU8MaxSize128* chunk)
{
    PHOTON_INFO("Begin file id(%" PRIu64 "), offset(%" PRIu64 "), size(%" PRIu64 ")", id, offset, chunk->size);
    return PhotonError_Ok;
}

PhotonError PhotonFl_ExecCmd_EndFile(uint64_t id, uint64_t size, PhotonFlChecksum* rv)
{
    PHOTON_INFO("End file id(%" PRIu64 "), size(%" PRIu64 ")", id, size);
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
