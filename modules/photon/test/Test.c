#include "photongen/onboard/test/Test.Component.h"
#include "photongen/onboard/pvu/Pvu.Component.h"

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
    _photonTest.param12 = 'a';
    _photonTest.dynArrayParam.size = 10;
    _photonTest.dynArrayParam2.size = 10;
    _photonTest.optionParam.type = PhotonOptionOptionOptionTestParamStructType_Some;
    _photonTest.optionVaruintParam.type = PhotonOptionVaruintType_Some;
    _photonTest.longString.size = 3;
    _photonTest.longString.data[0] = 'a';
    _photonTest.longString.data[1] = 's';
    _photonTest.longString.data[2] = 'd';
    _photonTest.longString.data[3] = '\0';
    for (size_t i = 0; i < 10; i++) {
        _photonTest.dynArrayParam.data[i].parama = i;
        _photonTest.dynArrayParam.data[i].paramb = i + 1;
    }
    for (size_t i = 0; i < 10; i++) {
        _photonTest.dynArrayParam2.data[i] = i + 1;
    }
    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 10; j++) {
            _photonTest.arrayParam[i][j] = i + j;
        }
    }
}

static uint64_t testEventsLeft = 0;

void PhotonTest_Tick()
{
    for (size_t i = 0; i < testEventsLeft; i++) {
        PhotonTestParamStruct s;
        s.parama = testEventsLeft;
        s.paramb = testEventsLeft;
         PhotonTest_QueueEvent_Event1(testEventsLeft, &s);
    }
         testEventsLeft = 0;
    //_photonTest.dynArrayParam.size++;
    //if (_photonTest.dynArrayParam.size > 10) {
    //    _photonTest.dynArrayParam.size = 0;
    //}
}

PhotonError PhotonTest_ExecCmd_SetParam1(uint8_t p)
{
    _photonTest.param1 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam1AndReturn(uint8_t p, PhotonTestReturnStruct* rv)
{
    _photonTest.param1 = p;
    rv->param1 = p;
    rv->param2 = -66;
    rv->param3.type = PhotonTestVariantParamType_StructType;
    rv->param3.data.structTypeTestVariantParam.a = -66;
    rv->param3.data.structTypeTestVariantParam.b = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam2(uint16_t p)
{
    _photonTest.param2 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam3(uint32_t p)
{
    _photonTest.param3 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam4(uint64_t p)
{
    _photonTest.param4 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam5(int8_t p)
{
    _photonTest.param5 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam6(int16_t p)
{
    _photonTest.param6 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam7(int32_t p)
{
    _photonTest.param7 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam8(int64_t p)
{
    _photonTest.param8 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam9(const PhotonTestParamStruct* p)
{
    _photonTest.param9 = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam10(float p)
{
    _photonTest.param10 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam11(double p)
{
    _photonTest.param11 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetParam12(char p)
{
    _photonTest.param12 = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetAllParams(uint8_t p1, uint16_t p2, uint32_t p3, uint64_t p4, int8_t p5, int16_t p6, int32_t p7, int64_t p8, const PhotonTestParamStruct* p9, float p10, double p11, char p12)
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
    _photonTest.param12 = p12;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_SetAllParamsStruct(const PhotonTestCmdStruct* s)
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
    _photonTest.param12 = s->param12;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_IncAllParams()
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
    _photonTest.param12++;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_DecAllParams()
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
    _photonTest.param12--;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestU8(uint8_t p, uint8_t* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestI8(int8_t p, int8_t* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestStruct(const PhotonTestParamStruct* p, PhotonTestParamStruct* rv)
{
    *rv = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestVariant(const PhotonTestVariantParam* p, PhotonTestVariantParam* rv)
{
    *rv = *p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestEnum(PhotonTestEnumParam p, PhotonTestEnumParam* rv)
{
    *rv = p;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestArray(uint16_t p[4], uint16_t rv[4])
{
    rv[0] = p[0];
    rv[1] = p[1];
    rv[2] = p[2];
    rv[3] = p[3];
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestDynArray(const PhotonDynArrayOfU32MaxSize4* p, PhotonDynArrayOfU32MaxSize4* rv)
{
    rv->data[0] = p->data[0];
    rv->data[1] = p->data[1];
    rv->data[2] = p->data[2];
    rv->data[3] = p->data[3];
    rv->size = p->size;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestString(const PhotonDynArrayOfCharMaxSize4* p, PhotonDynArrayOfCharMaxSize4* rv)
{
    rv->data[0] = p->data[0];
    rv->data[1] = p->data[1];
    rv->data[2] = p->data[2];
    rv->data[3] = p->data[3];
    rv->size = p->size;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestOptionU64(const PhotonOptionU64* p, PhotonOptionU64* rv)
{
    *rv = *p;
    return PhotonError_Ok;
}

static PhotonDynArrayOfU32MaxSize10 tempArray;

PhotonDynArrayOfU32MaxSize10* PhotonTest_AllocCmdArg_TestCmdCall_P2()
{
    return &tempArray;
}

PhotonError PhotonTest_ExecCmd_TestCmdCall(PhotonTestParamStruct p1, const PhotonDynArrayOfU32MaxSize10* p2, const PhotonTestEnumParam* p3)
{
    (void)p1;
    (void)p2;
    (void)p3;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestEvents(uint64_t count)
{
    testEventsLeft = count;
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestError(PhotonError value)
{
    return value;
}

PhotonError PhotonTest_ExecCmd_GenBigEvent()
{
    PhotonDynArrayOfU8MaxSize1024 s;
    s.size = 980;
    PhotonTest_QueueEvent_Big(&s);
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_GenTestEvent()
{
    PhotonTestParamStruct s;
    s.parama = 2;
    s.paramb = 3;
    PhotonTest_QueueEvent_Event1(1, &s);
    return PhotonError_Ok;
}

PhotonError PhotonTest_ExecCmd_TestPvuScript(const PhotonDynArrayOfCharMaxSize16* name)
{
    PhotonDynArrayOfU8MaxSize1024 cmds;
    PhotonWriter writer;
    PhotonWriter_Init(&writer, cmds.data, sizeof(cmds.data));
    PHOTON_TRY(PhotonTest_SerializeCmd_GenTestEvent(&writer));
    PHOTON_TRY(PhotonPvu_SerializeCmd_SleepFor(5000, &writer));
    PHOTON_TRY(PhotonTest_SerializeCmd_GenTestEvent(&writer));
    cmds.size = writer.current - writer.start;

    PhotonPvuScriptDesc desc;
    desc.name = *name;
    desc.autoremove = true;
    desc.autostart = true;
    return PhotonPvu_AddScript(&desc, &cmds);
    //uint8_t data[1024];
    //PhotonWriter_Init(&writer, data, sizeof(data));
    //PHOTON_TRY(PhotonPvu_SerializeCmd_AddScript(&desc, &cmds, &writer));
    //return PhotonError_Ok;
}
