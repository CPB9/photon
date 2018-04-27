#include "photongen/onboard/param/Param.Component.h"

#define _PHOTON_FNAME "param/Param.c"

PhotonError PhotonParam_Set(PhotonParamParamId id, const void* src, size_t size)
{
    (void)id;
    (void)src;
    (void)size;
    return PhotonError_NotImplemented;
}

PhotonError PhotonParam_ExecCmd_SetParamU8(uint64_t id, uint8_t value)
{
    return PhotonParam_Set(id, &value, 1);
}

PhotonError PhotonParam_ExecCmd_SetParamU16(uint64_t id, uint16_t value)
{
    return PhotonParam_Set(id, &value, 2);
}

PhotonError PhotonParam_ExecCmd_SetParamU32(uint64_t id, uint32_t value)
{
    return PhotonParam_Set(id, &value, 4);
}

PhotonError PhotonParam_ExecCmd_SetParamU64(uint64_t id, uint64_t value)
{
    return PhotonParam_Set(id, &value, 8);
}

PhotonError PhotonParam_ExecCmd_SetParamI8(uint64_t id, int8_t value)
{
    return PhotonParam_Set(id, &value, 1);
}

PhotonError PhotonParam_ExecCmd_SetParamI16(uint64_t id, int16_t value)
{
    return PhotonParam_Set(id, &value, 2);
}

PhotonError PhotonParam_ExecCmd_SetParamI32(uint64_t id, int32_t value)
{
    return PhotonParam_Set(id, &value, 4);
}

PhotonError PhotonParam_ExecCmd_SetParamI64(uint64_t id, int64_t value)
{
    return PhotonParam_Set(id, &value, 8);
}

PhotonError PhotonParam_ExecCmd_SetParamF32(uint64_t id, float value)
{
    return PhotonParam_Set(id, &value, 4);
}

PhotonError PhotonParam_ExecCmd_SetParamF64(uint64_t id, double value)
{
    return PhotonParam_Set(id, &value, 8);
}

#undef _PHOTON_FNAME
