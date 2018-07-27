#if defined(PHOTON_HAS_MODULE_CLK)
# include "photongen/onboard/clk/Clk.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_TM)
# include "photongen/onboard/tm/Tm.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_INT)
# include "photongen/onboard/int/Int.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_FWT)
# include "photongen/onboard/fwt/Fwt.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_EXC)
# include "photongen/onboard/exc/Exc.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_TEST)
# include "photongen/onboard/test/Test.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_DFU)
# include "photongen/onboard/dfu/Dfu.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_NAV)
# include "photongen/onboard/nav/Nav.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_USM)
# include "photongen/onboard/usm/Usm.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_GRP)
# include "photongen/onboard/grp/Grp.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_ZCVM)
# include "photongen/onboard/zcvm/Zcvm.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_BLOG)
# include "photongen/onboard/blog/Blog.Component.h"
#endif

void Photon_Init()
{
#if defined(PHOTON_HAS_MODULE_BLOG)
    PhotonBlog_Init();
#endif
#if defined(PHOTON_HAS_MODULE_CLK)
    PhotonClk_Init();
#endif
#if defined(PHOTON_HAS_MODULE_DFU)
    PhotonDfu_Init();
#endif
#if defined(PHOTON_HAS_MODULE_TM)
    PhotonTm_Init();
#endif
#if defined(PHOTON_HAS_MODULE_PVU)
    PhotonPvu_Init();
#endif
#if defined(PHOTON_HAS_MODULE_FWT)
    PhotonFwt_Init();
#endif
#if defined(PHOTON_HAS_MODULE_EXC)
    PhotonExc_Init();
#endif
#if defined(PHOTON_HAS_MODULE_NAV)
    PhotonNav_Init();
#endif
#if defined(PHOTON_HAS_MODULE_TEST)
    PhotonTest_Init();
#endif
#if defined(PHOTON_HAS_MODULE_USM)
    PhotonUsm_Init();
#endif
#if defined(PHOTON_HAS_MODULE_GRP)
    PhotonGrp_Init();
#endif
#if defined(PHOTON_HAS_MODULE_ZCVM)
    PhotonZcvm_Init();
#endif
}

void Photon_Tick()
{
#if defined(PHOTON_HAS_MODULE_CLK)
    PhotonClk_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_BLOG)
    PhotonBlog_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_DFU)
    PhotonDfu_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_TM)
    PhotonTm_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_PVU)
    PhotonPvu_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_FWT)
    PhotonFwt_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_EXC)
    PhotonExc_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_NAV)
    PhotonNav_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_TEST)
    PhotonTest_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_USM)
    PhotonUsm_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_GRP)
    PhotonGrp_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_ZCVM)
    PhotonZcvm_Tick();
#endif
}

