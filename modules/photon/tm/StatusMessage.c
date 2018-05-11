#include "photongen/onboard/tm/StatusMessage.h"
#include "photon/core/Assert.h"
#include "photon/core/Try.h"
#include "photon/core/Endian.h"

PhotonError PhotonTmStatusMessage_Encode(PhotonTmStatusMessage* self, PhotonGenerator gen, void* data, PhotonWriter* dest)
{
    (void)self;
    if (PhotonWriter_WritableSize(dest) < 2) {
        return PhotonError_NotEnoughSpace;
    }
    uint8_t* sizePtr = PhotonWriter_CurrentPtr(dest);
    PhotonWriter_Skip(dest, 2);
    PHOTON_TRY(gen(data, dest));
    uint8_t* currentPtr = PhotonWriter_CurrentPtr(dest);
    PHOTON_ASSERT(currentPtr > (sizePtr + 2));
    ptrdiff_t size = currentPtr - sizePtr - 2;
    PHOTON_ASSERT(size <= UINT16_MAX);
    Photon_Le16Enc(sizePtr, (uint16_t)size);
    return PhotonError_Ok;
}
