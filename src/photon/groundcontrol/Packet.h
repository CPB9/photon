#pragma once

#include "photon/Config.hpp"
#include "photon/model/OnboardTime.h"

#include <bmcl/SharedBytes.h>

#include <cstdint>

namespace photon {

enum class StreamDirection {
    Uplink = 0,
    Downlink = 1,
};

enum class StreamType : uint8_t {
    Firmware = 0,
    Cmd = 1,
    User = 2,
    Dfu = 3,
    Telem = 4,
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
    ReceiptType type;
    bmcl::SharedBytes payload;
    OnboardTime tickTime;
    uint16_t counter;
};

struct PacketRequest {
    bmcl::SharedBytes payload;
    StreamType streamType;
};
}
