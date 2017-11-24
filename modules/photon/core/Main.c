#if defined(PHOTON_HAS_MODULE_CLK)
# include "photon/clk/Clk.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_TM)
# include "photon/tm/Tm.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_INT)
# include "photon/int/Int.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_FWT)
# include "photon/fwt/Fwt.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_EXC)
# include "photon/exc/Exc.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_TEST)
# include "photon/test/Test.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_NAV)
# include "photon/nav/Nav.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_USM)
# include "photon/usm/Usm.Component.h"
#endif
#if defined(PHOTON_HAS_MODULE_GRP)
# include "photon/grp/Grp.Component.h"
#endif

void Photon_Init()
{
#if defined(PHOTON_HAS_MODULE_CLK)
    PhotonClk_Init();
#endif
#if defined(PHOTON_HAS_MODULE_TM)
    PhotonTm_Init();
#endif
#if defined(PHOTON_HAS_MODULE_INT)
    PhotonInt_Init();
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
}

void Photon_Tick()
{
#if defined(PHOTON_HAS_MODULE_CLK)
    PhotonClk_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_TM)
    PhotonTm_Tick();
#endif
#if defined(PHOTON_HAS_MODULE_INT)
    PhotonInt_Tick();
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
}

