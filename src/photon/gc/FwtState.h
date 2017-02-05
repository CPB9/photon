#pragma once

#include "photon/Config.h"
#include "photon/gc/MemIntervalSet.h"

#include <bmcl/ArrayView.h>
#include <bmcl/Buffer.h>
#include <bmcl/Option.h>

#include <array>

namespace bmcl {
class MemWriter;
class MemReader;
}

namespace photon {

struct StartCmdRndGen;

class FwtState {
public:
    FwtState();
    ~FwtState();

    void startDownload();

    bool isTransfering();
    void acceptResponse(bmcl::Bytes packet);
    bmcl::Option<bmcl::Bytes> nextPacket();

private:
    bmcl::Bytes encodePacket(void (FwtState::*gen)(bmcl::MemWriter*));

    void acceptChunkResponse(bmcl::MemReader* src);
    void acceptHashResponse(bmcl::MemReader* src);
    void acceptStartResponse(bmcl::MemReader* src);
    void acceptStopResponse(bmcl::MemReader* src);

    void genHashCmd(bmcl::MemWriter* dest);
    void genChunkCmd(bmcl::MemWriter* dest);
    void genStartCmd(bmcl::MemWriter* dest);
    void genStopCmd(bmcl::MemWriter* dest);

    bmcl::Option<std::array<uint8_t, 64>> _hash;

    MemIntervalSet _acceptedChunks;
    bmcl::Buffer _desc;

    bool _isTransfering;
    bool _hasStartCommandPassed;
    bool _hasDownloaded;
    std::unique_ptr<StartCmdRndGen> _startCmdState;
    uint8_t _temp[20];
};
}
