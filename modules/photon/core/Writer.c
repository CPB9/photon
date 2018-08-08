#include "photongen/onboard/core/Writer.h"
#include "photon/core/Assert.h"
#include "photon/core/Try.h"
#include "photon/core/Endian.h"

#include <string.h>

void PhotonWriter_Init(PhotonWriter* self, void* dest, size_t size)
{
    uint8_t* ptr = (uint8_t*)dest;
    self->current = ptr;
    self->start = ptr;
    self->end = ptr + size;
}

bool PhotonWriter_IsAtEnd(const PhotonWriter* self)
{
    return self->current == self->end;
}

size_t PhotonWriter_WritableSize(const PhotonWriter* self)
{
    return self->end - self->current;
}

uint8_t* PhotonWriter_CurrentPtr(const PhotonWriter* self)
{
    return self->current;
}

void PhotonWriter_SetCurrentPtr(PhotonWriter* self, uint8_t* ptr)
{
    PHOTON_ASSERT(ptr >= self->start && ptr <= self->end);
    self->current = ptr;
}

void PhotonWriter_Write(PhotonWriter* self, const void* src, size_t size)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= size);
    memcpy(self->current, src, size);
    self->current += size;
}

void PhotonWriter_Skip(PhotonWriter* self, size_t size)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= size);
    self->current += size;
}

void PhotonWriter_SliceFromBack(PhotonWriter* self, size_t size, PhotonWriter* dest)
{
    dest->start = self->current;
    dest->current = dest->start;
    dest->end = self->end - size;
    self->current = (uint8_t*) dest->end;
}

void PhotonWriter_WriteChar(PhotonWriter* self, char value)
{
    PhotonWriter_WriteU8(self, value);
}

void PhotonWriter_WriteU8(PhotonWriter* self, uint8_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 1);
    *self->current = value;
    self->current++;
}

void PhotonWriter_WriteU16Be(PhotonWriter* self, uint16_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 2);
    Photon_Be16Enc(self->current, value);
    self->current += 2;
}

void PhotonWriter_WriteU16Le(PhotonWriter* self, uint16_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 2);
    Photon_Le16Enc(self->current, value);
    self->current += 2;
}

void PhotonWriter_WriteU32Be(PhotonWriter* self, uint32_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 4);
    Photon_Be32Enc(self->current, value);
    self->current += 4;
}

void PhotonWriter_WriteU32Le(PhotonWriter* self, uint32_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 4);
    Photon_Le32Enc(self->current, value);
    self->current += 4;
}

void PhotonWriter_WriteU64Be(PhotonWriter* self, uint64_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 8);
    Photon_Be64Enc(self->current, value);
    self->current += 8;
}

void PhotonWriter_WriteU64Le(PhotonWriter* self, uint64_t value)
{
    PHOTON_ASSERT(PhotonWriter_WritableSize(self) >= 8);
    Photon_Le64Enc(self->current, value);
    self->current += 8;
}

void PhotonWriter_WriteF32Le(PhotonWriter* self, float value)
{
    union {
        float f;
        uint32_t v;
    } u;
    u.f = value;
    Photon_Le32Enc(self->current, u.v);
    self->current += 4;
}

void PhotonWriter_WriteF64Le(PhotonWriter* self, double value)
{
    union {
        double f;
        uint64_t v;
    } u;
    u.f = value;
    Photon_Le64Enc(self->current, u.v);
    self->current += 8;
}

void PhotonWriter_WriteUSizeLe(PhotonWriter* self, size_t value)
{
#if UINTPTR_MAX == 0xffffffff
    PhotonWriter_WriteU32Le(self, value);
#elif UINTPTR_MAX == 0xffffffffffffffff
    PhotonWriter_WriteU64Le(self, value);
#else
# error "Invalid architecture"
#endif
}

void PhotonWriter_WriteUSizeBe(PhotonWriter* self, size_t value)
{
#if UINTPTR_MAX == 0xffffffff
    PhotonWriter_WriteU32Be(self, value);
#elif UINTPTR_MAX == 0xffffffffffffffff
    PhotonWriter_WriteU64Be(self, value);
#else
# error "Invalid architecture"
#endif
}

void PhotonWriter_WritePtrLe(PhotonWriter* self, const void* value)
{
#if SIZE_MAX == 0xffffffff
    PhotonWriter_WriteU32Le(self, (uint32_t)value);
#elif SIZE_MAX == 0xffffffffffffffff
    PhotonWriter_WriteU64Le(self, (uint64_t)value);
#else
# error "Invalid architecture"
#endif
}

void PhotonWriter_WritePtrBe(PhotonWriter* self, const void* value)
{
#if SIZE_MAX == 0xffffffff
    PhotonWriter_WriteU32Be(self, (uint32_t)value);
#elif SIZE_MAX == 0xffffffffffffffff
    PhotonWriter_WriteU64Be(self, (uint64_t)value);
#else
# error "Invalid architecture"
#endif
}

#define RETURN_IF_SIZE_LESS(size)                   \
    if (PhotonWriter_WritableSize(self) < size) {   \
        return PhotonError_NotEnoughSpace;          \
    }

PhotonError PhotonWriter_WriteVaruint(PhotonWriter* self, uint64_t value)
{
    if (value <= 240) {
        RETURN_IF_SIZE_LESS(1);
        self->current[0] = (uint8_t)(value);
        self->current += 1;
        return PhotonError_Ok;
    }
    if (value <= 2287) {
        RETURN_IF_SIZE_LESS(2);
        self->current[0] = (uint8_t)((value - 240) / 256 + 241);
        self->current[1] = (uint8_t)((value - 240) % 256);
        self->current += 2;
        return PhotonError_Ok;
    }
    if (value <= 67823) {
        RETURN_IF_SIZE_LESS(3);
        self->current[0] = 249;
        self->current[1] = (uint8_t)((value - 2288) / 256);
        self->current[2] = (uint8_t)((value - 2288) % 256);
        self->current += 3;
        return PhotonError_Ok;
    }
    if (value <= 16777215) {
        RETURN_IF_SIZE_LESS(4);
        self->current[0] = 250;
        self->current[1] = (uint8_t)(value >> 16);
        self->current[2] = (uint8_t)(value >> 8);
        self->current[3] = (uint8_t)(value);
        self->current += 4;
        return PhotonError_Ok;
    }
    if (value <= 4294967295) {
        RETURN_IF_SIZE_LESS(5);
        self->current[0] = 251;
        self->current[1] = (uint8_t)(value >> 24);
        self->current[2] = (uint8_t)(value >> 16);
        self->current[3] = (uint8_t)(value >> 8);
        self->current[4] = (uint8_t)(value);
        self->current += 5;
        return PhotonError_Ok;
    }
    if (value <= 1099511627775) {
        RETURN_IF_SIZE_LESS(6);
        self->current[0] = 252;
        self->current[1] = (uint8_t)(value >> 32);
        self->current[2] = (uint8_t)(value >> 24);
        self->current[3] = (uint8_t)(value >> 16);
        self->current[4] = (uint8_t)(value >> 8);
        self->current[5] = (uint8_t)(value);
        self->current += 6;
        return PhotonError_Ok;
    }
    if (value <= 281474976710655) {
        RETURN_IF_SIZE_LESS(7);
        self->current[0] = 253;
        self->current[1] = (uint8_t)(value >> 40);
        self->current[2] = (uint8_t)(value >> 32);
        self->current[3] = (uint8_t)(value >> 24);
        self->current[4] = (uint8_t)(value >> 16);
        self->current[5] = (uint8_t)(value >> 8);
        self->current[6] = (uint8_t)(value);
        self->current += 7;
        return PhotonError_Ok;
    }
    if (value <= 72057594037927935) {
        RETURN_IF_SIZE_LESS(8);
        self->current[0] = 254;
        self->current[1] = (uint8_t)(value >> 48);
        self->current[2] = (uint8_t)(value >> 40);
        self->current[3] = (uint8_t)(value >> 32);
        self->current[4] = (uint8_t)(value >> 24);
        self->current[5] = (uint8_t)(value >> 16);
        self->current[6] = (uint8_t)(value >> 8);
        self->current[7] = (uint8_t)(value);
        self->current += 8;
        return PhotonError_Ok;
    }
    RETURN_IF_SIZE_LESS(9);
    self->current[0] = 255;
    self->current[1] = (uint8_t)(value >> 56);
    self->current[2] = (uint8_t)(value >> 48);
    self->current[3] = (uint8_t)(value >> 40);
    self->current[4] = (uint8_t)(value >> 32);
    self->current[5] = (uint8_t)(value >> 24);
    self->current[6] = (uint8_t)(value >> 16);
    self->current[7] = (uint8_t)(value >> 8);
    self->current[8] = (uint8_t)(value);
    self->current += 9;
    return PhotonError_Ok;
}

#undef RETURN_IF_SIZE_LESS

PhotonError PhotonWriter_WriteVarint(PhotonWriter* self, int64_t value)
{
    return PhotonWriter_WriteVaruint(self, (value << 1) ^ (value >> 63));
}
