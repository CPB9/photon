#include "photon/exc/TmPacket.h"

#include "photon/core/Address.h"
#include "photon/core/Assert.h"
#include "photon/core/Crc.h"
#include "photon/core/Endian.h"
#include "photon/core/StreamType.h"
#include "photon/core/Time.h"
#include "photon/core/Try.h"

// [u16(size), data, u16(crc)]
// size = sizeof(data) + sizeof(u16)

PhotonError PhotonExcTmPacket_Encode(PhotonExcTmPacket* self, PhotonGenerator gen, void* data, PhotonWriter* dest)
{
    if (PhotonWriter_WritableSize(dest) < 4) {
        return PhotonError_NotEnoughSpace;
    }

    PhotonWriter reserved;
    PhotonWriter_SliceFromBack(dest, 2, &reserved);

    uint8_t* sizePtr = PhotonWriter_CurrentPtr(&reserved);
    PhotonWriter_Skip(&reserved, 2);

    PHOTON_TRY(PhotonStreamType_Serialize(PhotonStreamType_Telemetry, &reserved));
    PHOTON_TRY(PhotonAddress_Serialize(&self->address, &reserved));
    PHOTON_TRY(PhotonTime_Serialize(&self->time, &reserved));
    PHOTON_TRY(gen(data, &reserved));

    PhotonWriter_SetCurrentPtr(dest, PhotonWriter_CurrentPtr(&reserved));

    uint8_t* crcPtr = PhotonWriter_CurrentPtr(dest);
    PHOTON_ASSERT(crcPtr > (sizePtr + 2));
    ptrdiff_t size = crcPtr - sizePtr; // -2 + 2
    PHOTON_ASSERT(size <= UINT16_MAX);
    Photon_Le16Enc(sizePtr, (uint16_t)size);

    uint16_t crc = Photon_Crc16(sizePtr, size - 2);
    PhotonWriter_WriteU16Le(dest, crc);

    return PhotonError_Ok;
}
