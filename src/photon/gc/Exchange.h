#pragma once

#include "photon/Config.h"

#include <bmcl/ArrayView.h>
#include <bmcl/RingBuffer.h>

#include <memory>

#include <cstdint>
#include <cstddef>

namespace photon {

class Packet;

enum class PacketAssemblyState {
    None,
    SepSkipped,
    SizeFound,
};

class Exchange {
public:
    Exchange();
    ~Exchange();

    void acceptInput(bmcl::Bytes input);
    std::size_t genOutput(void* dest, std::size_t size);

protected:
    virtual void handleIncoming(bmcl::Bytes data) = 0;
    virtual void handleJunk(bmcl::Bytes junk) = 0;

    void encodePacket(bmcl::Bytes data);

private:
    bool findSepAndSkip();
    void processInput();

    PacketAssemblyState _assemblyState;
    uint16_t _packetSize;
    bmcl::RingBuffer _input;
    bmcl::RingBuffer _output;
};
}
