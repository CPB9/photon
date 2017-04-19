#include "photon/test/Test.Component.h"

void PhotonTest_Init()
{
    _test.param1 = 1;
    _test.param2 = 2;
    _test.param3 = 3;
    _test.param4 = 4;
    _test.param5 = 5;
    _test.param6 = 6;
    _test.param7 = 7;
    _test.param8 = 8;
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

PhotonError PhotonTest_SetParam5(int8_t p)
{
    _test.param5 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam6(int16_t p)
{
    _test.param6 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam7(int32_t p)
{
    _test.param7 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam8(int64_t p)
{
    _test.param8 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetAllParams(uint8_t p1, uint16_t p2, uint32_t p3, uint64_t p4, int8_t p5, int16_t p6, int32_t p7, int64_t p8)
{
    _test.param1 = p1;
    _test.param2 = p2;
    _test.param3 = p3;
    _test.param4 = p4;
    _test.param5 = p5;
    _test.param6 = p6;
    _test.param7 = p7;
    _test.param8 = p8;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetAllParamsStruct(const PhotonTestParamStruct* s)
{
    _test.param1 = s->param1;
    _test.param2 = s->param2;
    _test.param3 = s->param3;
    _test.param4 = s->param4;
    _test.param5 = s->param5;
    _test.param6 = s->param6;
    _test.param7 = s->param7;
    _test.param8 = s->param8;
    return PhotonError_Ok;
}

PhotonError PhotonTest_IncAllParams()
{
    _test.param1++;
    _test.param2++;
    _test.param3++;
    _test.param4++;
    _test.param5++;
    _test.param6++;
    _test.param7++;
    _test.param8++;
    return PhotonError_Ok;
}

PhotonError PhotonTest_DecAllParams()
{
    _test.param1--;
    _test.param2--;
    _test.param3--;
    _test.param4--;
    _test.param5--;
    _test.param6--;
    _test.param7--;
    _test.param8--;
    return PhotonError_Ok;
}
