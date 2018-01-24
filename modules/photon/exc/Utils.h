#ifndef __PHOTON_EXC_UTILS_H__
#define __PHOTON_EXC_UTILS_H__

#include "photongen/onboard/Config.h"

#include "photongen/onboard/core/Error.h"
#include "photongen/onboard/core/Writer.h"
#include "photon/core/Assert.h"
#include "photon/core/Endian.h"
#include "photon/core/Crc.h"

#include <stddef.h>
#include <stdint.h>

// [u16(size), data, u16(crc)]
// size = sizeof(data) + sizeof(u16)

#define PHOTON_EXC_ENCODE_PACKET_HEADER(dest, reserved)                     \
    if (PhotonWriter_WritableSize(dest) < 4) {                              \
        return PhotonError_NotEnoughSpace;                                  \
    }                                                                       \
                                                                            \
    PhotonWriter reserved;                                                  \
    PhotonWriter_SliceFromBack(dest, 2, &reserved);                         \
                                                                            \
    uint8_t* l_sizePtr = PhotonWriter_CurrentPtr(&reserved);                \
    PhotonWriter_Skip(&reserved, 2);

#define PHOTON_EXC_ENCODE_PACKET_FOOTER(dest, reserved)                     \
    PhotonWriter_SetCurrentPtr(dest, PhotonWriter_CurrentPtr(&reserved));   \
                                                                            \
    uint8_t* l_crcPtr = PhotonWriter_CurrentPtr(dest);                      \
    PHOTON_ASSERT(l_crcPtr > (l_sizePtr + 2));                              \
    ptrdiff_t l_size = l_crcPtr - l_sizePtr;                                \
    PHOTON_ASSERT(l_size <= UINT16_MAX);                                    \
    Photon_Le16Enc(l_sizePtr, (uint16_t)l_size);                            \
                                                                            \
    uint16_t l_crc = Photon_Crc16(l_sizePtr, l_size);                       \
    PhotonWriter_WriteU16Le(dest, l_crc);                                   \

#endif
