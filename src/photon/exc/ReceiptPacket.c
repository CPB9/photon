#include "photon/exc/ReceiptPacket.h"

#include "photon/exc/Utils.h"
#include "photon/exc/PacketType.h"
#include "photon/core/Try.h"

PhotonError PhotonExcReceiptPacket_Encode(PhotonExcReceiptPacket* self, PhotonWriter* dest)
{
    PHOTON_EXC_ENCODE_PACKET_HEADER(reserved);

    PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Receipt, &reserved));
    PHOTON_TRY(PhotonExcReceiptPacket_Serialize(self, &reserved));

    PHOTON_EXC_ENCODE_PACKET_FOOTER(reserved);
}
