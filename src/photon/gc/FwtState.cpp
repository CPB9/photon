#include "photon/gc/FwtState.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

#include <chrono>
#include <random>

namespace photon {

const unsigned maxChunkSize = 200;

struct StartCmdRndGen {
    StartCmdRndGen()
    {
        std::random_device device;
        engine.seed(device());
    }

    uint64_t generate()
    {
        last = dist(engine);
        return last;
    }

    std::default_random_engine engine;
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t last;
};

FwtState::FwtState()
    : _isTransfering(false)
    , _hasStartCommandPassed(false)
    , _hasDownloaded(false)
    , _startCmdState(new StartCmdRndGen)
{
}

FwtState::~FwtState()
{
}

void FwtState::startDownload()
{
    _isTransfering = true;
    _hash = bmcl::None;
    _hasStartCommandPassed = false;
    _hasDownloaded = false;
}

bool FwtState::isTransfering()
{
    return _isTransfering;
}

//Hash = 0
//Chunk = 1

void FwtState::acceptResponse(bmcl::Bytes packet)
{
    if (!_isTransfering) {
        return;
    }
    bmcl::MemReader reader(packet.begin(), packet.size());
    uint64_t answerType;
    if (!reader.readVarUint(&answerType)) {
        //TODO: report error
        return;
    }

    switch (answerType) {
    case 0:
        acceptHashResponse(&reader);
        return;
    case 1:
        acceptChunkResponse(&reader);
        return;
    case 2:
        acceptStartResponse(&reader);
        return;
    case 3:
        acceptStopResponse(&reader);
        return;
    default:
        //TODO: report error
        return;
    }
}

void FwtState::acceptChunkResponse(bmcl::MemReader* src)
{
    if (_hash.isNone()) {
        //TODO: report error, recieved chunk earlier then hash
        return;
    }

    if (!_hasStartCommandPassed) {
        //TODO: report error
        return;
    }

    uint64_t start;
    if (!src->readVarUint(&start)) {
        //TODO: report error
        return;
    }

    //TODO: check overflow
    MemInterval os(start, start + src->sizeLeft());

    if (os.start() > _desc.size()) {
        //TODO: report error
        return;
    }

    if (os.end() > _desc.size()) {
        //TODO: report error
        return;
    }

    src->read(_desc.data() + os.start(), os.size());

    _acceptedChunks.add(os);
}

void FwtState::acceptHashResponse(bmcl::MemReader* src)
{
    uint64_t descSize;
    if (!src->readVarUint(&descSize)) {
        //TODO: report error
        return;
    }
    //TODO: check descSize for overflow
    if (src->readableSize() != 64) {
        //TODO: report error
        return;
    }

    if (_hash.isSome()) {
        //TODO: log
        return;
    }

    _hash.emplace();
    src->read(_hash.unwrap().data(), 64);

    _desc.resize(descSize);
    _acceptedChunks.clear();
    _startCmdState->generate();
}

void FwtState::acceptStartResponse(bmcl::MemReader* src)
{
    uint64_t startRandomId;
    if (!src->readVarUint(&startRandomId)) {
        //TODO: report error
        return;
    }
    if (startRandomId != _startCmdState->last) {
        //TODO: report error
        return;
    }
    _hasStartCommandPassed = true;
}

void FwtState::acceptStopResponse(bmcl::MemReader* src)
{
}

//RequestHash = 0
//RequestChunk = 1
//Start = 2
//Stop = 3

void FwtState::genHashCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarUint(0));
}

void FwtState::genChunkCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarUint(1));
    MemInterval os(0, 0);
    //TODO: calc nextChunk

    BMCL_ASSERT(dest->writeVarUint(os.start()));
    BMCL_ASSERT(dest->writeVarUint(os.end()));
}

void FwtState::genStartCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarUint(2));
    BMCL_ASSERT(dest->writeVarUint(_startCmdState->last));
}

void FwtState::genStopCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarUint(3));
}

inline bmcl::Bytes FwtState::encodePacket(void (FwtState::*gen)(bmcl::MemWriter*))
{
    bmcl::MemWriter writer(_temp, sizeof(_temp));
    (this->*gen)(&writer);
    return bmcl::Bytes(writer.start(), writer.sizeUsed());
}

bmcl::Option<bmcl::Bytes> FwtState::nextPacket()
{
    if (!_isTransfering) {
        return bmcl::None;
    }
    if (_hash.isNone()) {
        return encodePacket(&FwtState::genHashCmd);
    }
    if (!_hasStartCommandPassed) {
        return encodePacket(&FwtState::genStartCmd);
    }
}
}
