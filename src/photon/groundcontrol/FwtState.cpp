/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/FwtState.h"
#include "photon/groundcontrol/MemIntervalSet.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/core/Diagnostics.h"
#include "decode/parser/Project.h"
#include "photon/model/ValueInfoCache.h"
#include "decode/core/Utils.h"
#include "photon/groundcontrol/Atoms.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/ProjectUpdate.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>
#include <bmcl/SharedBytes.h>
#include <bmcl/Bytes.h>
#include <bmcl/Sha3.h>
#include <bmcl/FixedArrayView.h>

#include <caf/atom.hpp>

#include <chrono>
#include <random>

#define FWT_LOG(msg)         \
    if (_isLoggingEnabled) { \
        logMsg(msg);         \
    }

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(std::string);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::ProjectUpdate::ConstPointer);

namespace photon {

FwtState::FwtState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& eventHandler)
    : caf::event_based_actor(cfg)
    , _isRunning(false)
    , _isDownloading(false)
    , _isLoggingEnabled(false)
    , _checkId(0)
    , _exc(exchange)
    , _handler(eventHandler)
{
}

FwtState::~FwtState()
{
    stopDownload();
}

caf::behavior FwtState::make_behavior()
{
    return caf::behavior{
        [this](RecvPacketPayloadAtom, const PacketHeader& header, const bmcl::SharedBytes& packet) {
            (void)header;
            acceptData(packet.view());
        },
        [this](FwtHashAtom) {
            handleHashAction();
        },
        [this](FwtCheckAtom, std::size_t id) {
            handleCheckAction(id);
        },
        [this](StartAtom) {
            _isRunning = true;
        },
        [this](UpdateFirmware) {
            startDownload();
        },
        [this](StopAtom) {
            _isRunning = false;
            stopDownload();
        },
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            setProject(update->project(), update->device(), update->cache());
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            _isLoggingEnabled = isEnabled;
        },
    };
}

bool FwtState::hashMatches(const HashContainer& hash, bmcl::Bytes data)
{
    decode::Project::HashType state;
    state.update(data);
    auto calculatedHash = state.finalize();
    if (hash.size() != calculatedHash.size()) {
        return false;
    }
    return hash == bmcl::Bytes(calculatedHash.data(), calculatedHash.size());
}

void FwtState::setProject(const decode::Project* proj, const decode::Device* dev, const ValueInfoCache* cache)
{
    FWT_LOG("setting project");
    if (_project == proj && _device == dev) {
        FWT_LOG("no need to update project");
        return;
    }
    _project = proj;
    _device = dev;
    if (_downloadedHash.isNone()) {
        FWT_LOG("project set");
        decode::Project::HashType state;
        auto buf = _project->encode();
        state.update(buf);
        auto calculatedHash = state.finalize();
        assert(calculatedHash.size() == 64);
        _downloadedHash.emplace();
        std::memcpy(_downloadedHash.unwrap().data(), calculatedHash.data(), 64);
        if (_isRunning) {
            startDownload();
        }
        return;
    }
    auto buf = _project->encode();
    if (!hashMatches(_downloadedHash.unwrap(), buf)) {
        FWT_LOG("project hash mismatch");
        _downloadedHash = bmcl::None;
        if (_isRunning) {
            startDownload();
        }
    }
}

void FwtState::startDownload()
{
    if (_isDownloading) {
        return;
    }
    FWT_LOG("starting firmware download");
    stopDownload();
    _isDownloading = true;
    send(_handler, FirmwareDownloadStartedEventAtom::value);
    send(this, FwtHashAtom::value);
}

void FwtState::stopDownload()
{
    if (_isDownloading) {
        FWT_LOG("stopping firmware download");
    }
    _acceptedChunks.clear();
    _desc.resize(0);
    _deviceName.clear();
    _hash = bmcl::None;
    _isDownloading = false;
}

void FwtState::restartChunkDownload()
{
    _acceptedChunks.clear();
    checkIntervals();
    scheduleCheck();
}

void FwtState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}

const char* FwtState::name() const
{
    return "FwtState";
}

template <typename C, typename... A>
inline void FwtState::packAndSendPacket(C&& enc, A&&... args)
{
    bmcl::MemWriter writer(_temp, sizeof(_temp));
    (this->*enc)(&writer, std::forward<A>(args)...);
    PacketRequest req(writer.writenData(), StreamType::Firmware);
    send(_exc, SendUnreliablePacketAtom::value, std::move(req));
}

void FwtState::scheduleHash()
{
    auto hashTimeout = std::chrono::milliseconds(500);
    delayed_send(this, hashTimeout, FwtHashAtom::value);
}

void FwtState::scheduleCheck()
{
    _checkId++;
    auto checkTimeout = std::chrono::milliseconds(500);
    delayed_send(this, checkTimeout, FwtCheckAtom::value, _checkId);
}

void FwtState::handleHashAction()
{
    if (!_isDownloading) {
        return;
    }
    if (_hash.isSome()) {
        return;
    }
    packAndSendPacket(&FwtState::genHashCmd);
    scheduleHash();
}

void FwtState::handleCheckAction(std::size_t id)
{
    if (!_isDownloading) {
        return;
    }
    if (id != _checkId) {
        return;
    }
    //TODO: handle event
    checkIntervals();
    scheduleCheck();
}

void FwtState::logMsg(std::string&& msg)
{
    send(_handler, LogAtom::value, std::move(msg));
}

void FwtState::reportFirmwareError(std::string&& msg)
{
    send(_handler, FirmwareErrorEventAtom::value, std::move(msg));
}

//Hash = 0
//Chunk = 1

void FwtState::acceptData(bmcl::Bytes packet)
{
    if (!_isDownloading) {
        FWT_LOG("ignoring fwt packet while stopped");
        return;
    }
    bmcl::MemReader reader(packet.begin(), packet.size());
    int64_t answerType;
    if (!reader.readVarInt(&answerType)) {
        reportFirmwareError("Recieved firmware chunk with invalid size");
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
        acceptStopResponse(&reader);
        return;
    default:
        reportFirmwareError("Recieved invalid fwt response: " + std::to_string(answerType));
        return;
    }
}

void FwtState::acceptChunkResponse(bmcl::MemReader* src)
{
    if (_hash.isNone()) {
        FWT_LOG("recieved firmware chunk before recieving hash");
        return;
    }

    uint64_t start;
    if (!src->readVarUint(&start)) {
        reportFirmwareError("Recieved firmware chunk with invalid start offset");
        return;
    }

    //TODO: check overflow
    MemInterval os(start, start + src->sizeLeft());

    if (os.start() > _desc.size()) {
        reportFirmwareError("Recieved firmware chunk with start offset > firmware size");
        return;
    }

    if (os.end() > _desc.size()) {
        reportFirmwareError("Recieved firmware chunk with end offset > firmware end");
        return;
    }

    src->read(_desc.data() + os.start(), os.size());

    FWT_LOG("recieved firmware chunk (" + std::to_string(os.start()) + ", " + std::to_string(os.end()) + ")");
    _acceptedChunks.add(os);
    send(_handler, FirmwareProgressEventAtom::value, std::size_t(_acceptedChunks.dataSize()), std::size_t(_desc.size()));
    checkIntervals();
    scheduleCheck();
}

void FwtState::checkIntervals()
{
    FWT_LOG("checking firmware intervals");
    if (_acceptedChunks.size() == 0) {
        packAndSendPacket(&FwtState::genChunkCmd, MemInterval(0, _desc.size()));
        return;
    }
    if (_acceptedChunks.size() == 1) {
        MemInterval chunk = _acceptedChunks.at(0);
        if (chunk.start() == 0) {
            if (chunk.size() == _desc.size()) {
                readFirmware();
                return;
            }
            packAndSendPacket(&FwtState::genChunkCmd, MemInterval(chunk.end(), _desc.size()));
            return;
        }
        packAndSendPacket(&FwtState::genChunkCmd, MemInterval(0, chunk.start()));
        return;
    }
    std::size_t lastOffset = 0;
    std::vector<MemInterval> chunks;
    size_t maxChunks = 10; //TODO: define
    chunks.reserve(maxChunks);

    for (std::size_t i = 0; i < _acceptedChunks.size(); i++) {
        MemInterval chunk1 = _acceptedChunks.at(i);
        if (chunk1.start() > lastOffset) {
            chunks.emplace_back(lastOffset, chunk1.start());
        }
        lastOffset = chunk1.end();
        if (chunks.size() >= maxChunks) {
            break;
        }
    }

    packAndSendPacket(&FwtState::genChunksCmd, chunks);
}

void FwtState::readFirmware()
{
    if (!_isDownloading) {
        return;
    }
    FWT_LOG("parsing firmware");
    _checkId = 0;
    send(_handler, FirmwareDownloadFinishedEventAtom::value);

    if (!hashMatches(_hash.unwrap(), _desc)) {
        reportFirmwareError("Invalid firmware hash");
        stopDownload();
        return;
    }

    Rc<decode::Diagnostics> diag = new decode::Diagnostics();
    auto project = decode::Project::decodeFromMemory(diag.get(), _desc.data(), _desc.size());
    if (project.isErr()) {
        reportFirmwareError("Firmware decode failed");
        //TODO: restart download
        stopDownload();
        return;
    }

    _downloadedHash = _hash.unwrap();
    _project = project.unwrap();
    auto update = ProjectUpdate::fromProjectAndName(_project.get(), _deviceName);
    if (update.isErr()) {
        reportFirmwareError("Project update error: " + update.unwrapErr());
        //TODO: restart download
        stopDownload();
        return;
    }
    _device = update.unwrap()->device();

    send(_exc, SetProjectAtom::value, update.take());
    stopDownload();
}

void FwtState::acceptHashResponse(bmcl::MemReader* src)
{
    if (_hash.isSome()) {
        //reportFirmwareError("Recieved additional hash response");
        return;
    }

    uint64_t descSize;
    if (!src->readVarUint(&descSize)) {
        reportFirmwareError("Recieved hash response with invalid firmware size");
        scheduleHash();
        return;
    }

    auto name = decode::deserializeString(src);
    if (name.isErr()) {
        reportFirmwareError("Recieved hash response with invalid device name");
        return;
    }
    _deviceName = name.unwrap().toStdString();

    //TODO: check descSize for overflow
    if (src->readableSize() != 64) {
        reportFirmwareError("Recieved hash response with invalid hash size: " + std::to_string(src->readableSize()));
        scheduleHash();
        return;
    }

    _hash.emplace();
    src->read(_hash.unwrap().data(), 64);

    _desc.resize(descSize);
    _acceptedChunks.clear();

    send(_handler, FirmwareSizeRecievedEventAtom::value, std::size_t(_desc.size()));
    send(_handler, FirmwareHashDownloadedEventAtom::value, name.unwrap().toStdString(), bmcl::SharedBytes::create(_hash.unwrap()));

    if (_downloadedHash.isNone()) {
        restartChunkDownload();
        return;
    }
    if (_downloadedHash.unwrap() != _hash.unwrap()) {
        restartChunkDownload();
        return;
    }

    if (!_project || !_device) {
        restartChunkDownload();
        return;
    }
    auto buf = _project->encode();
    if (!hashMatches(_downloadedHash.unwrap(), buf)) {
        FWT_LOG("firmware hash mismatch");
        restartChunkDownload();
        return;
    }

    send(_handler, FirmwareDownloadFinishedEventAtom::value);
    auto update = ProjectUpdate::fromProjectAndName(_project.get(), _deviceName);
    if (update.isErr()) {
        reportFirmwareError("Project update error: " + update.unwrapErr());
        //TODO: restart download
        stopDownload();
        return;
    }

    send(_exc, SetProjectAtom::value, update.unwrap());
    _device = update.unwrap()->device();
    FWT_LOG("hash matches, no need to download");
    stopDownload();
}

void FwtState::acceptStopResponse(bmcl::MemReader* src)
{
}

//RequestHash = 0
//RequestChunk = 1
//Stop = 2

void FwtState::genHashCmd(bmcl::MemWriter* dest)
{
    FWT_LOG("sending firmware hash request");
    BMCL_ASSERT(dest->writeVarInt(0));
}

void FwtState::genChunkCmd(bmcl::MemWriter* dest, MemInterval os)
{
    FWT_LOG("sending firmware chunk request");
    BMCL_ASSERT(dest->writeVarInt(1));

    BMCL_ASSERT(dest->writeVarUint(os.start()));
    BMCL_ASSERT(dest->writeVarUint(os.end()));
}

void FwtState::genChunksCmd(bmcl::MemWriter* dest, const std::vector<MemInterval>& chunks)
{
    FWT_LOG("sending firmware chunks request");
    BMCL_ASSERT(dest->writeVarInt(1));

    for (MemInterval os : chunks) {
        BMCL_ASSERT(dest->writeVarUint(os.start()));
        BMCL_ASSERT(dest->writeVarUint(os.end()));
    }
}

void FwtState::genStopCmd(bmcl::MemWriter* dest)
{
    FWT_LOG("sending firmware stop request");
    BMCL_ASSERT(dest->writeVarInt(2));
}
}
