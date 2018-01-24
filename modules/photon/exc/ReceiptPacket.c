#include "photongen/onboard/exc/ReceiptPacket.h"

#include "photon/Utils.h"
#include "photongen/onboard/exc/PacketType.h"
#include "photon/core/Try.h"

PhotonError PhotonExcReceiptPacket_Encode(PhotonExcReceiptPacket* self, PhotonWriter* dest)
{
    PHOTON_EXC_ENCODE_PACKET_HEADER(dest, reserved);

    PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Receipt, &reserved));
    PHOTON_TRY(PhotonExcReceiptPacket_Serialize(self, &reserved));

    PHOTON_EXC_ENCODE_PACKET_FOOTER(dest, reserved);
    return PhotonError_Ok;
}
