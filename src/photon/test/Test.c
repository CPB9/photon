#include "photon/test/Test.Component.h"

void PhotonTest_Init()
{
    _test.param1 = 0;
    _test.param2 = 0;
    _test.param3 = 0;
    _test.param4 = 0;
}

PhotonError PhotonTest_SetParam1(uint8_t p)
{
    _test.param1 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam2(uint16_t p)
{
    _test.param2 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam3(uint32_t p)
{
    _test.param3 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam4(uint64_t p)
{
    _test.param4 = p;
    return PhotonError_Ok;
}
