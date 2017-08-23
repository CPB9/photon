#include "photon/test/Test.Component.h"

void PhotonTest_Init()
{
    _photonTest.param1 = 1;
    _photonTest.param2 = 2;
    _photonTest.param3 = 3;
    _photonTest.param4 = 4;
    _photonTest.param5 = 5;
    _photonTest.param6 = 6;
    _photonTest.param7 = 7;
    _photonTest.param8 = 8;
    _photonTest.param9.parama = 9;
    _photonTest.param9.paramb = 9;
    _photonTest.param10 = 10;
    _photonTest.param11 = 11;
}

void PhotonTest_Tick()
{
}

PhotonError PhotonTest_SetParam1(uint8_t p)
{
    _photonTest.param1 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam1AndReturn(uint8_t p, PhotonTestReturnStruct* rv)
{
    _photonTest.param1 = p;
    rv->param1 = p;
    rv->param2 = -66;
    rv->param3.type = PhotonTestVariantParamType_StructType;
    rv->param3.data.structTypeVariantParam.a = -66;
    rv->param3.data.structTypeVariantParam.b = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam2(uint16_t p)
{
    _photonTest.param2 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam3(uint32_t p)
{
    _photonTest.param3 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam4(uint64_t p)
{
    _photonTest.param4 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam5(int8_t p)
{
    _photonTest.param5 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam6(int16_t p)
{
    _photonTest.param6 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam7(int32_t p)
{
    _photonTest.param7 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam8(int64_t p)
{
    _photonTest.param8 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam9(const PhotonTestParamStruct* p)
{
    _photonTest.param9 = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam10(float p)
{
    _photonTest.param10 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetParam11(double p)
{
    _photonTest.param11 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetAllParams(uint8_t p1, uint16_t p2, uint32_t p3, uint64_t p4, int8_t p5, int16_t p6, int32_t p7, int64_t p8, const PhotonTestParamStruct* p9, float p10, double p11)
{
    _photonTest.param1 = p1;
    _photonTest.param2 = p2;
    _photonTest.param3 = p3;
    _photonTest.param4 = p4;
    _photonTest.param5 = p5;
    _photonTest.param6 = p6;
    _photonTest.param7 = p7;
    _photonTest.param8 = p8;
    _photonTest.param9 = *p9;
    _photonTest.param10 = p10;
    _photonTest.param11 = p11;
    return PhotonError_Ok;
}

PhotonError PhotonTest_SetAllParamsStruct(const PhotonTestCmdStruct* s)
{
    _photonTest.param1 = s->param1;
    _photonTest.param2 = s->param2;
    _photonTest.param3 = s->param3;
    _photonTest.param4 = s->param4;
    _photonTest.param5 = s->param5;
    _photonTest.param6 = s->param6;
    _photonTest.param7 = s->param7;
    _photonTest.param8 = s->param8;
    _photonTest.param9 = s->param9;
    _photonTest.param10 = s->param10;
    _photonTest.param11 = s->param11;
    return PhotonError_Ok;
}

PhotonError PhotonTest_IncAllParams()
{
    _photonTest.param1++;
    _photonTest.param2++;
    _photonTest.param3++;
    _photonTest.param4++;
    _photonTest.param5++;
    _photonTest.param6++;
    _photonTest.param7++;
    _photonTest.param8++;
    _photonTest.param9.parama++;
    _photonTest.param9.paramb++;
    _photonTest.param10++;
    _photonTest.param11++;
    return PhotonError_Ok;
}

PhotonError PhotonTest_DecAllParams()
{
    _photonTest.param1--;
    _photonTest.param2--;
    _photonTest.param3--;
    _photonTest.param4--;
    _photonTest.param5--;
    _photonTest.param6--;
    _photonTest.param7--;
    _photonTest.param8--;
    _photonTest.param9.parama--;
    _photonTest.param9.paramb--;
    _photonTest.param10--;
    _photonTest.param11--;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestU8(uint8_t p, uint8_t* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestI8(int8_t p, int8_t* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestStruct(const PhotonTestParamStruct* p, PhotonTestParamStruct* rv)
{
    *rv = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestVariant(const PhotonTestVariantParam* p, PhotonTestVariantParam* rv)
{
    *rv = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestEnum(PhotonTestEnumParam p, PhotonTestEnumParam* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_TestArray(uint16_t p[4], uint16_t rv[4])
{
    rv[0] = p[0];
    rv[1] = p[1];
    rv[2] = p[2];
    rv[3] = p[3];
    return PhotonError_Ok;
}

//PhotonError PhotonTest_TestDynArray(const PhotonDynArrayOfU32MaxSize4* p, PhotonDynArrayOfU32MaxSize4* rv)
PhotonError PhotonTest_TestDynArray(PhotonDynArrayOfU32MaxSize4* rv)
{
    rv->data[0] = 111;
    rv->data[1] = 222;
    rv->data[2] = 333;
    rv->size = 3;
    return PhotonError_Ok;
}
