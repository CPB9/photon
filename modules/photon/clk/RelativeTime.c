#include "photon/clk/RelativeTime.h"

#define RETURN_IF_SIZE_LESS(size)                   \
    if (PhotonWriter_WritableSize(dest) < size) {   \
        return PhotonError_NotEnoughSpace;          \
    }

PhotonError PhotonClkRelativeTime_Encode(PhotonClkRelativeTime self, PhotonWriter* dest)
{
    uint8_t* current = PhotonWriter_CurrentPtr(dest);
    if (self <= 127) {
        RETURN_IF_SIZE_LESS(1);
        current[0] = (uint8_t)(self);
        PhotonWriter_Skip(dest, 1);
        return PhotonError_Ok;
    }
    if (self <= 16383) {
        RETURN_IF_SIZE_LESS(2);
        current[0] = (uint8_t)(0x80 | (self >> 7));
        current[1] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 2);
        return PhotonError_Ok;
    }
    if (self <= 2097151) {
        RETURN_IF_SIZE_LESS(3);
        current[0] = (uint8_t)(0x80 | (self >> 14));
        current[1] = (uint8_t)(0x80 | (self >> 7));
        current[2] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 3);
        return PhotonError_Ok;
    }
    if (self <= 268435455) {
        RETURN_IF_SIZE_LESS(4);
        current[0] = (uint8_t)(0x80 | (self >> 21));
        current[1] = (uint8_t)(0x80 | (self >> 14));
        current[2] = (uint8_t)(0x80 | (self >> 7));
        current[3] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 4);
        return PhotonError_Ok;
    }
    if (self <= 34359738367) {
        RETURN_IF_SIZE_LESS(5);
        current[0] = (uint8_t)(0x80 | (self >> 28));
        current[1] = (uint8_t)(0x80 | (self >> 21));
        current[2] = (uint8_t)(0x80 | (self >> 14));
        current[3] = (uint8_t)(0x80 | (self >> 7));
        current[4] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 5);
        return PhotonError_Ok;
    }
    if (self <= 4398046511103) {
        RETURN_IF_SIZE_LESS(6);
        current[0] = (uint8_t)(0x80 | (self >> 35));
        current[1] = (uint8_t)(0x80 | (self >> 28));
        current[2] = (uint8_t)(0x80 | (self >> 21));
        current[3] = (uint8_t)(0x80 | (self >> 14));
        current[4] = (uint8_t)(0x80 | (self >> 7));
        current[5] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 6);
        return PhotonError_Ok;
    }
    if (self <= 562949953421311) {
        RETURN_IF_SIZE_LESS(7);
        current[0] = (uint8_t)(0x80 | (self >> 42));
        current[1] = (uint8_t)(0x80 | (self >> 35));
        current[2] = (uint8_t)(0x80 | (self >> 28));
        current[3] = (uint8_t)(0x80 | (self >> 21));
        current[4] = (uint8_t)(0x80 | (self >> 14));
        current[5] = (uint8_t)(0x80 | (self >> 7));
        current[6] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 7);
        return PhotonError_Ok;
    }
    if (self <= 72057594037927935) {
        RETURN_IF_SIZE_LESS(8);
        current[0] = (uint8_t)(0x80 | (self >> 49));
        current[1] = (uint8_t)(0x80 | (self >> 42));
        current[2] = (uint8_t)(0x80 | (self >> 35));
        current[3] = (uint8_t)(0x80 | (self >> 28));
        current[4] = (uint8_t)(0x80 | (self >> 21));
        current[5] = (uint8_t)(0x80 | (self >> 14));
        current[6] = (uint8_t)(0x80 | (self >> 7));
        current[7] = (uint8_t)(0x7f & (self));
        PhotonWriter_Skip(dest, 8);
        return PhotonError_Ok;
    }
    RETURN_IF_SIZE_LESS(9);
    current[0] = (uint8_t)(0x80 | (self >> 56));
    current[1] = (uint8_t)(0x80 | (self >> 49));
    current[2] = (uint8_t)(0x80 | (self >> 42));
    current[3] = (uint8_t)(0x80 | (self >> 35));
    current[4] = (uint8_t)(0x80 | (self >> 28));
    current[5] = (uint8_t)(0x80 | (self >> 21));
    current[6] = (uint8_t)(0x80 | (self >> 14));
    current[7] = (uint8_t)(0x80 | (self >> 7));
    current[8] = (uint8_t)(0x7f & (self));
    PhotonWriter_Skip(dest, 9);
    return PhotonError_Ok;
}
