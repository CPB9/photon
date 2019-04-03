#include "photongen/onboard/core/Reader.h"
#include "photon/core/Assert.h"
#include "photon/core/Endian.h"
#include "photon/core/Try.h"

#include <string.h>

void PhotonReader_Init(PhotonReader* self, const void* data, size_t size)
{
    const uint8_t* ptr = (const uint8_t*)data;
    self->current = ptr;
    self->start = ptr;
    self->end = ptr + size;
}

bool PhotonReader_IsAtEnd(const PhotonReader* self)
{
    return self->current == self->end;
}

const uint8_t* PhotonReader_CurrentPtr(const PhotonReader* self)
{
    return self->current;
}

size_t PhotonReader_ReadableSize(const PhotonReader* self)
{
    return self->end - self->current;
}

void PhotonReader_Skip(PhotonReader* self, size_t size)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= size);
    self->current += size;
}

void PhotonReader_Read(PhotonReader* self, void* dest, size_t size)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= size);
    memcpy(dest, self->current, size);
    self->current += size;
}

void PhotonReader_Slice(PhotonReader* self, size_t length, PhotonReader* dest)
{
    dest->start = self->current;
    self->current += length;
    dest->current = dest->start;
    dest->end = dest->current + length;
}

void PhotonReader_SliceToEnd(PhotonReader* self, PhotonReader* dest)
{
    const uint8_t* cur = self->current;
    const uint8_t* end = self->end;
    dest->current = cur;
    dest->start = cur;
    dest->end = end;
}

char PhotonReader_ReadChar(PhotonReader* self)
{
    return PhotonReader_ReadU8(self);
}

uint8_t PhotonReader_ReadU8(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 1);
    uint8_t value = *self->current;
    self->current++;
    return value;
}

uint16_t PhotonReader_ReadU16Be(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 2);
    uint16_t value = Photon_Be16Dec(self->current);
    self->current += 2;
    return value;
}

uint16_t PhotonReader_ReadU16Le(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 2);
    uint16_t value = Photon_Le16Dec(self->current);
    self->current += 2;
    return value;
}

uint32_t PhotonReader_ReadU32Be(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 4);
    uint32_t value = Photon_Be32Dec(self->current);
    self->current += 4;
    return value;
}

uint32_t PhotonReader_ReadU32Le(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 4);
    uint32_t value = Photon_Le32Dec(self->current);
    self->current += 4;
    return value;
}

uint64_t PhotonReader_ReadU64Be(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 8);
    uint64_t value = Photon_Be64Dec(self->current);
    self->current += 8;
    return value;
}

uint64_t PhotonReader_ReadU64Le(PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 8);
    uint64_t value = Photon_Le64Dec(self->current);
    self->current += 8;
    return value;
}

float PhotonReader_ReadF32Le(PhotonReader* self)
{
    union {
        float f;
        uint32_t u;
    } u;
    u.u = Photon_Le32Dec(self->current);
    self->current += 4;
    return u.f;
}

double PhotonReader_ReadF64Le(PhotonReader* self)
{
    union {
        double f;
        uint64_t u;
    } u;
    u.u = Photon_Le64Dec(self->current);
    self->current += 8;
    return u.f;
}

uint8_t PhotonReader_PeekU8(const PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 1);
    return *self->current;
}

uint16_t PhotonReader_PeakU16Be(const PhotonReader* self)
{
    PHOTON_ASSERT(PhotonReader_ReadableSize(self) >= 2);
    return Photon_Be16Dec(self->current);
}

size_t PhotonReader_ReadUSizeLe(PhotonReader* self)
{
#if UINTPTR_MAX == 0xffffffff
    return PhotonReader_ReadU32Le(self);
#elif UINTPTR_MAX == 0xffffffffffffffff
    return PhotonReader_ReadU64Le(self);
#else
# error "Invalid architecture"
#endif
}

size_t PhotonReader_ReadUSizeBe(PhotonReader* self)
{
#if UINTPTR_MAX == 0xffffffff
    return PhotonReader_ReadU32Be(self);
#elif UINTPTR_MAX == 0xffffffffffffffff
    return PhotonReader_ReadU64Be(self);
#else
# error "Invalid architecture"
#endif
}

void* PhotonReader_ReadPtrLe(PhotonReader* self)
{
#if SIZE_MAX == 0xffffffff
    return (void*)PhotonReader_ReadU32Le(self);
#elif SIZE_MAX == 0xffffffffffffffff
    return (void*)PhotonReader_ReadU64Le(self);
#else
# error "Invalid architecture"
#endif
}

void* PhotonReader_ReadPtrBe(PhotonReader* self)
{
#if SIZE_MAX == 0xffffffff
    return (void*)PhotonReader_ReadU32Be(self);
#elif SIZE_MAX == 0xffffffffffffffff
    return (void*)PhotonReader_ReadU64Be(self);
#else
# error "Invalid architecture"
#endif
}

#define RETURN_IF_SIZE_LESS(size)                   \
    if (PhotonReader_ReadableSize(self) < size) {   \
        return PhotonError_NotEnoughData;           \
    }

PhotonError PhotonReader_ReadVaruint(PhotonReader* self, uint64_t* dest)
{
    RETURN_IF_SIZE_LESS(1);
    uint8_t head = *self->current;
    if (head <= 240) {
        *dest = head;
        self->current += 1;
        return PhotonError_Ok;
    }
    if (head <= 248) {
        RETURN_IF_SIZE_LESS(2);
        *dest = 240 + 256 * ((uint64_t)head - 241) + (uint64_t)self->current[1];
        self->current += 2;
        return PhotonError_Ok;
    }
    if (head == 249) {
        RETURN_IF_SIZE_LESS(3);
        *dest = 2288 + 256 * ((uint64_t)self->current[1]) + (uint64_t)self->current[2];
        self->current += 3;
        return PhotonError_Ok;
    }
    if (head == 250) {
        RETURN_IF_SIZE_LESS(4);
        *dest = (uint64_t)self->current[1] << 16 | (uint64_t)self->current[2] << 8 | (uint64_t)self->current[3];
        self->current += 4;
        return PhotonError_Ok;
    }
    if (head == 251) {
        RETURN_IF_SIZE_LESS(5);
        *dest = (uint64_t)self->current[1] << 24 | (uint64_t)self->current[2] << 16 | (uint64_t)self->current[3] << 8 | (uint64_t)self->current[4];
        self->current += 5;
        return PhotonError_Ok;
    }
    if (head == 252) {
        RETURN_IF_SIZE_LESS(6);
        *dest = (uint64_t)self->current[1] << 32 | (uint64_t)self->current[2] << 24 | (uint64_t)self->current[3] << 16 | (uint64_t)self->current[4] << 8 | (uint64_t)self->current[5];
        self->current += 6;
        return PhotonError_Ok;
    }
    if (head == 253) {
        RETURN_IF_SIZE_LESS(7);
        *dest = (uint64_t)self->current[1] << 40 | (uint64_t)self->current[2] << 32 | (uint64_t)self->current[3] << 24 | (uint64_t)self->current[4] << 16 | (uint64_t)self->current[5] << 8 | (uint64_t)self->current[6];
        self->current += 7;
        return PhotonError_Ok;
    }
    if (head == 254) {
        RETURN_IF_SIZE_LESS(8);
        *dest = (uint64_t)self->current[1] << 48 | (uint64_t)self->current[2] << 40 | (uint64_t)self->current[3] << 32 | (uint64_t)self->current[4] << 24 | (uint64_t)self->current[5] << 16 | (uint64_t)self->current[6] << 8 | (uint64_t)self->current[7];
        self->current += 8;
        return PhotonError_Ok;
    }
    RETURN_IF_SIZE_LESS(9);
    *dest = (uint64_t)self->current[1] << 56 | (uint64_t)self->current[2] << 48 | (uint64_t)self->current[3] << 40 | (uint64_t)self->current[4] << 32 | (uint64_t)self->current[5] << 24 | (uint64_t)self->current[6] << 16 | (uint64_t)self->current[7] << 8 | (uint64_t)self->current[8];
    self->current += 9;
    return PhotonError_Ok;
}

#undef RETURN_IF_SIZE_LESS

PhotonError PhotonReader_ReadVarint(PhotonReader* self, int64_t* dest)
{
    uint64_t n;
    PHOTON_TRY(PhotonReader_ReadVaruint(self, &n));
    *dest = (n >> 1) ^ -(int64_t)(n & 1);
    return PhotonError_Ok;
}
