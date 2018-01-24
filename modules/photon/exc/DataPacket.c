#include "photongen/onboard/exc/DataPacket.h"

#include "photongen/onboard/exc/PacketType.h"
#include "photongen/onboard/exc/Utils.h"
#include "photon/core/Try.h"

PhotonError PhotonExcDataPacket_Encode(PhotonExcDataPacket* self, PhotonGenerator gen, void* data, PhotonWriter* dest)
{
    PHOTON_EXC_ENCODE_PACKET_HEADER(dest, reserved);

    PHOTON_TRY(PhotonExcPacketType_Serialize(PhotonExcPacketType_Data, &reserved));
    PHOTON_TRY(PhotonExcDataPacket_Serialize(self, &reserved));
    PHOTON_TRY(gen(data, &reserved));

    PHOTON_EXC_ENCODE_PACKET_FOOTER(dest, reserved);
    return PhotonError_Ok;
}
