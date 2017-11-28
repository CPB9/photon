#pragma once

#include <bmcl/StringView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Buffer.h>

#include <string>

namespace photon {

class CoderState {
public:
    void setError(bmcl::StringView msg)
    {
        _error.assign(msg.begin(), msg.end());
    }

    const std::string error() const
    {
        return _error;
    }

private:
    std::string _error;
};
}

template <typename T>
bool photongenSerialize(const T& self, bmcl::Buffer* dest, photon::CoderState* state);

template <typename T>
bool photongenDeserialize(T* self, bmcl::MemReader* src, photon::CoderState* state);

inline bool photongenSerialize(bool self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeUint8(self);
    return true;
}

inline bool photongenSerialize(uint8_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeUint8(self);
    return true;
}

inline bool photongenSerialize(uint16_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeUint16Le(self);
    return true;
}

inline bool photongenSerialize(uint32_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeUint32Le(self);
    return true;
}

inline bool photongenSerialize(uint64_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeUint64Le(self);
    return true;
}

inline bool photongenSerialize(int8_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeInt8(self);
    return true;
}

inline bool photongenSerialize(int16_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeInt16Le(self);
    return true;
}

inline bool photongenSerialize(int32_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeInt32Le(self);
    return true;
}

inline bool photongenSerialize(int64_t self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeInt16Le(self);
    return true;
}

inline bool photongenSerialize(float self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeFloat32Le(self);
    return true;
}

inline bool photongenSerialize(double self, bmcl::Buffer* dest, photon::CoderState* state)
{
    dest->writeFloat64Le(self);
    return true;
}


inline bool photongenDeserialize(bool* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 1) {
        return false;
    }
    *self = src->readUint8();
    return true;
}

inline bool photongenDeserialize(uint8_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 1) {
        return false;
    }
    *self = src->readUint8();
    return true;
}

inline bool photongenDeserialize(uint16_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 2) {
        return false;
    }
    *self = src->readUint16Le();
    return true;
}

inline bool photongenDeserialize(uint32_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 4) {
        return false;
    }
    *self = src->readUint32Le();
    return true;
}

inline bool photongenDeserialize(uint64_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 8) {
        return false;
    }
    *self = src->readUint64Le();
    return true;
}

inline bool photongenDeserialize(int8_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 1) {
        return false;
    }
    *self = src->readInt8();
    return true;
}

inline bool photongenDeserialize(int16_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 2) {
        return false;
    }
    *self = src->readInt16Le();
    return true;
}

inline bool photongenDeserialize(int32_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 4) {
        return false;
    }
    *self = src->readInt32Le();
    return true;
}

inline bool photongenDeserialize(int64_t* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 8) {
        return false;
    }
    *self = src->readInt64Le();
    return true;
}

inline bool photongenDeserialize(float* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 4) {
        return false;
    }
    *self = src->readFloat32Le();
    return true;
}

inline bool photongenDeserialize(double* self, bmcl::MemReader* src, photon::CoderState* state)
{
    if (src->sizeLeft() < 8) {
        return false;
    }
    *self = src->readFloat64Le();
    return true;
}
