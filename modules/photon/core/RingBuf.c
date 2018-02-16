#include "photongen/onboard/core/RingBuf.h"
#include "photon/core/Assert.h"

#include <string.h>

#define min(a, b) (((a)<(b))?(a):(b))

void PhotonRingBuf_Init(PhotonRingBuf* self, void* data, size_t size)
{
    PHOTON_ASSERT(size > 0);
    self->data = (uint8_t*)data;
    self->size = size;
    self->freeSpace = size;
    self->readOffset = 0;
    self->writeOffset = 0;
}

void PhotonRingBuf_Peek(const PhotonRingBuf* self, void* dest, size_t size, size_t offset)
{
    size_t readOffset = self->readOffset + offset;
    if (readOffset >= self->size) {
        readOffset -= self->size;
    }
    PHOTON_ASSERT(size + offset <= self->size - self->freeSpace);
    if (readOffset < self->writeOffset) { /* ----------r**************w---------- */
        memcpy(dest, self->data + readOffset, size);
    } else { /* *********w---------------r************ */
        size_t rightData = self->size - readOffset;
        size_t firstChunkSize = min(size, rightData);
        memcpy(dest, self->data + readOffset, firstChunkSize);
        if (size > firstChunkSize) {
            size_t secondChunkSize = size - firstChunkSize;
            memcpy((uint8_t*)dest + firstChunkSize, self->data, secondChunkSize);
        }
    }
}

uint8_t PhotonRingBuf_PeekU8(const PhotonRingBuf* self, size_t offset)
{
    PHOTON_ASSERT(offset + 1 <= self->size - self->freeSpace);
    size_t readOffset = self->readOffset + offset;
    if (readOffset >= self->size) {
        readOffset -= self->size;
    }
    return *(self->data + readOffset);
}

void PhotonRingBuf_Erase(PhotonRingBuf* self, size_t size)
{
    PHOTON_ASSERT(self->size - self->freeSpace >= size);
    self->freeSpace += size;
    self->readOffset += size;
    if (self->readOffset >= self->size) {
        self->readOffset -= self->size;
    }
}

void PhotonRingBuf_Clear(PhotonRingBuf* self)
{
    self->freeSpace = self->size;
    self->readOffset = 0;
    self->writeOffset = 0;
}

void PhotonRingBuf_Read(PhotonRingBuf* self, void* dest, size_t size)
{
    PhotonRingBuf_Peek(self, dest, size, 0);
    PhotonRingBuf_Erase(self, size);
}

static void extend(PhotonRingBuf* self, size_t size)
{
    self->writeOffset += size;
    if (self->writeOffset >= self->size) {
        self->writeOffset -= self->size;
    }
    self->freeSpace -= size;
}

void PhotonRingBuf_Write(PhotonRingBuf* self, const void* data, size_t size)
{
    PHOTON_ASSERT(size <= self->size);
    if (self->freeSpace < size) {
        PhotonRingBuf_Erase(self, size - self->freeSpace);
    }
    if (self->readOffset > self->writeOffset) { /* *********w---------------r************ */
        memcpy(self->data + self->writeOffset, data, size);
    } else { /* ----------r**************w---------- */
        size_t rightData = self->size - self->writeOffset;
        size_t firstChunkSize = min(size, rightData);
        memcpy(self->data + self->writeOffset, data, firstChunkSize);
        if (size > firstChunkSize) {
            size_t secondChunkSize = size - firstChunkSize;
            memcpy(self->data, (const uint8_t*)data + firstChunkSize, secondChunkSize);
        }
    }
    extend(self, size);
}

const uint8_t* PhotonRingBuf_CurrentReadPtr(const PhotonRingBuf* self)
{
    return self->data + self->readOffset;
}


size_t PhotonRingBuf_ReadableSize(const PhotonRingBuf* self)
{
    return self->size - self->freeSpace;
}

size_t PhotonRingBuf_WritableSize(const PhotonRingBuf* self)
{
    return self->freeSpace;
}

size_t PhotonRingBuf_LinearReadableSize(const PhotonRingBuf* self)
{
    if (self->readOffset < self->writeOffset) {
        return self->writeOffset - self->readOffset;
    }
    if (self->freeSpace == self->size) {
        return 0;
    }
    return self->size - self->readOffset;
}

uint8_t* PhotonRingBuf_CurrentWritePtr(const PhotonRingBuf* self)
{
    return self->data + self->writeOffset;
}

size_t PhotonRingBuf_LinearWritableSize(const PhotonRingBuf* self)
{
    if (self->readOffset > self->writeOffset) {
        return self->readOffset - self->writeOffset;
    }
    if (self->freeSpace == 0) {
        return 0;
    }
    return self->size - self->writeOffset;
}

void PhotonRingBuf_AdvanceWritePtr(PhotonRingBuf* self, size_t size)
{
    self->freeSpace -= size;
    self->writeOffset += size;
    if (self->writeOffset >= self->size) {
        self->writeOffset -= self->size;
    }
}

void PhotonRingBuf_AdvanceReadPtr(PhotonRingBuf* self, size_t size)
{
    self->freeSpace -= size;
    self->readOffset += size;
    if (self->readOffset >= self->size) {
        self->readOffset -= self->size;
    }
}

PhotonMemChunks PhotonRingBuf_ReadableChunks(const PhotonRingBuf* self)
{
    PhotonMemChunks chunks;

    if (self->freeSpace == self->size) {
        /* ************************wr************ */
        chunks.first.data = self->data + self->readOffset;
        chunks.first.size = 0;
        chunks.second = chunks.first;
    } else if (self->readOffset < self->writeOffset) {
        /* ---------r***************w------------ */
        chunks.first.data = self->data + self->readOffset;
        chunks.first.size = self->writeOffset - self->readOffset;
        chunks.second.data = self->data + self->writeOffset;
        chunks.second.size = 0;
    } else if (self->readOffset > self->writeOffset) {
        /* *********w---------------r************ */
        chunks.first.data = self->data + self->readOffset;
        chunks.first.size = self->size - self->readOffset;
        chunks.second.data = self->data;
        chunks.second.size = self->writeOffset;
    } else {
        /* ------------------------wr------------ */
        chunks.first.data = self->data + self->readOffset;
        chunks.first.size = self->size - self->readOffset;
        chunks.second.data = self->data;
        chunks.second.size = self->writeOffset;
    }

    return chunks;
}
