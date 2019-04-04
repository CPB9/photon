#pragma once

#include "photon/Config.hpp"
#include "photon/model/OnboardTime.h"

#include <bmcl/SharedBytes.h>
#include <bmcl/Bytes.h>
#include <bmcl/Uuid.h>

#include <cstdint>

namespace photon {

enum class StreamDirection : uint8_t {
    Uplink = 0,
    Downlink = 1,
};

enum class StreamType : uint8_t {
    Firmware = 0,
    Cmd = 1,
    Telem = 2,
    User = 3,
    Dfu = 4,
};

enum class PacketType : uint8_t {
    Unreliable = 0,
    Reliable = 1,
    Receipt = 2,
};

enum class ReceiptType : uint8_t {
    Ok = 0,
    PacketError = 1,
    PayloadError = 2,
    CounterCorrection = 3,
};

struct PacketHeader {
    OnboardTime tickTime;
    uint64_t srcAddress;
    uint64_t destAddress;
    uint16_t counter;
    StreamDirection streamDirection;
    PacketType packetType;
    StreamType streamType;
};

struct PacketResponse {
    PacketResponse(const bmcl::Uuid& uuid, const bmcl::SharedBytes& data, ReceiptType type, OnboardTime time, uint16_t counter)
        : requestUuid(uuid)
        , payload(data)
        , type(type)
        , tickTime(time)
        , counter(counter)
    {
    }
    bmcl::Uuid requestUuid;
    bmcl::SharedBytes payload;
    ReceiptType type;
    OnboardTime tickTime;
    uint16_t counter;
};

struct PacketRequest {
    PacketRequest(const bmcl::Uuid& uuid, bmcl::Bytes data, StreamType streamType)
        : requestUuid(uuid)
        , payload(bmcl::SharedBytes::create(data))
        , streamType(streamType)
    {
    }

    PacketRequest(bmcl::Bytes data, StreamType streamType)
        : requestUuid(bmcl::Uuid::createNil())
        , payload(bmcl::SharedBytes::create(data))
        , streamType(streamType)
    {
    }

    bmcl::Uuid requestUuid;
    bmcl::SharedBytes payload;
    StreamType streamType;
};
}
