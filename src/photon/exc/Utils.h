#ifndef __PHOTON_EXC_UTILS_H__
#define __PHOTON_EXC_UTILS_H__

#include "photon/core/Error.h"
#include "photon/core/Writer.h"
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
    uint8_t* sizePtr = PhotonWriter_CurrentPtr(&reserved);                  \
    PhotonWriter_Skip(&reserved, 2);

#define PHOTON_EXC_ENCODE_PACKET_FOOTER(dest, reserved)                     \
    PhotonWriter_SetCurrentPtr(dest, PhotonWriter_CurrentPtr(&reserved));   \
                                                                            \
    uint8_t* crcPtr = PhotonWriter_CurrentPtr(dest);                        \
    PHOTON_ASSERT(crcPtr > (sizePtr + 2));                                  \
    ptrdiff_t size = crcPtr - sizePtr;                                      \
    PHOTON_ASSERT(size <= UINT16_MAX);                                      \
    Photon_Le16Enc(sizePtr, (uint16_t)size);                                \
                                                                            \
    uint16_t crc = Photon_Crc16(sizePtr, size - 2);                         \
    PhotonWriter_WriteU16Le(dest, crc);                                     \
                                                                            \
    return PhotonError_Ok;

#endif
