#include "photon/tm/Tm.Component.h"
#include "photon/core/Writer.h"

static uint8_t buf[4096];

int main()
{
    PhotonTm_Init();
    PhotonWriter dest;
    PhotonWriter_Init(&dest, buf, sizeof(buf));
    PhotonTm_CollectStatusMessages(&dest);
}
