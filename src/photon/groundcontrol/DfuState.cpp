/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/DfuState.h"
#include "photon/groundcontrol/Atoms.h"
#include "photon/groundcontrol/ProjectUpdate.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/core/Try.h"
#include "decode/core/DataReader.h"
#include "decode/core/Utils.h"
#include "photongen/groundcontrol/dfu/Cmd.hpp"
#include "photongen/groundcontrol/dfu/Response.hpp"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/SharedBytes.h>
#include <bmcl/Result.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Rc<const photon::DfuStatus>);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::DataReader::Pointer);

namespace photon {

DfuState::DfuState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& eventHandler)
    : caf::event_based_actor(cfg)
    , _exc(exchange)
    , _handler(eventHandler)
{
}

DfuState::~DfuState()
{
}

template <typename C, typename... A>
void DfuState::sendCmd(C&& ser, A&&... args)
{
    CoderState state(OnboardTime::now());
    if (ser(&_temp, &state, std::forward<A>(args)...)) {
        PacketRequest req(_temp.asBytes(), StreamType::Dfu);
        send(_exc, SendUnreliablePacketAtom::value, std::move(req));
    } else {
        //TODO: report internal error
    }
    _temp.resize(0);
}

caf::behavior DfuState::make_behavior()
{
    return caf::behavior{
        [this](RecvPacketPayloadAtom, const PacketHeader& header, const bmcl::SharedBytes& packet) {
            bmcl::MemReader reader(packet.data(), packet.size());
            CoderState state(header.tickTime);
            photongen::dfu::Response resp;
            if (!photongenDeserializeDfuResponse(&resp, &reader, &state)) {
                //TODO: report error
                return;
            }
            switch (resp) {
            case photongen::dfu::Response::GetInfo:
                handleReportInfo(&reader, &state);
                break;
            case photongen::dfu::Response::BeginOk:
                handleBegin(&reader, &state);
                break;
            case photongen::dfu::Response::WriteOk:
                handleWrite(&reader, &state);
                break;
            case photongen::dfu::Response::EndOk:
                handleEnd(&reader, &state);
                break;
            case photongen::dfu::Response::Error:
                handleError(&reader, &state);
                break;
            case photongen::dfu::Response::None:
                break;
            }
        },
        [](StartAtom) {
        },
        [](StopAtom) {
        },
        [](EnableLoggindAtom, bool isEnabled) {
        },
        [](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
        },
        [this](FlashDfuFirmware, std::uintmax_t id, const decode::DataReader::Pointer& reader) {
            _flashPromise = make_response_promise();
            _fwReader = reader;
            _sectorId = id;
            beginUpload();
            return _flashPromise;
        },
        [this](RequestDfuStatus) {
            _statesPromises.emplace_back(make_response_promise());
            sendCmd([](bmcl::Buffer* dest, CoderState* state) -> bool {
                return photongenSerializeDfuCmd(photongen::dfu::Cmd::GetInfo, dest, state);
            });
            return _statesPromises.back();
        },
    };
}

void DfuState::handleReportInfo(bmcl::MemReader* reader, CoderState* state)
{
    Rc<DfuStatus> status = new DfuStatus;
    if (!photongenDeserializeDfuSectorData(&status->sectorData, reader, state)) {
        //TODO: report error
        return;
    }
    if (!photongenDeserializeDfuAllSectorsDesc(&status->sectorDesc, reader, state)) {
        //TODO: report error
        return;
    }
    for (caf::response_promise& promise : _statesPromises) {
        promise.deliver(Rc<const DfuStatus>(status));
    }
    _statesPromises.clear();
}

void DfuState::handleBegin(bmcl::MemReader* reader, CoderState* state)
{
    std::uint64_t sectorId;
    if (!reader->readVarUint(&sectorId)) {
        BMCL_CRITICAL() << "invalid begin response";
        return;
    }
    std::uint64_t dataSize;
    if (!reader->readVarUint(&dataSize)) {
        BMCL_CRITICAL() << "invalid begin response";
        return;
    }

    if (sectorId != _sectorId) {
        BMCL_CRITICAL() << "invalid sector id";
        return;
    }

    if (dataSize != _fwReader->size()) {
        BMCL_CRITICAL() << "invalid sector size";
        return;
    }
    sendNextChunk();
}

void DfuState::handleWrite(bmcl::MemReader* reader, CoderState* state)
{
    std::uint64_t offset;
    if (!reader->readVarUint(&offset)) {
        BMCL_CRITICAL() << "invalid write response";
        return;
    }
    if (offset != _fwReader->offset()) {
        BMCL_CRITICAL() << "invalid offset " << offset << " " << _fwReader->offset();
        return;
    }
    sendNextChunk();
}

void DfuState::handleEnd(bmcl::MemReader* reader, CoderState* state)
{
    std::uint64_t size;
    if (!reader->readVarUint(&size)) {
        BMCL_CRITICAL() << "invalid end response";
        return;
    }
    if (size != _fwReader->size()) {
        BMCL_CRITICAL() << "invalid total size " << size << " " << _fwReader->size();
        return;
    }
    BMCL_DEBUG() << "end...";
    _flashPromise.deliver(std::string("ok"));
}

void DfuState::handleError(bmcl::MemReader* reader, CoderState* state)
{
    auto rv = decode::deserializeString(reader);
    if (rv.isErr()) {
        BMCL_CRITICAL() << "invalid error response";
        return;
    }
    BMCL_CRITICAL() << "recieved error: " << rv.unwrap().toStdString();
}

constexpr const std::size_t chunkSize = 256;

void DfuState::beginUpload()
{
    sendCmd([this](bmcl::Buffer* dest, CoderState* state) -> bool {
        TRY(photongenSerializeDfuCmd(photongen::dfu::Cmd::BeginUpdate, dest, state));
        dest->writeVarUint(_sectorId);
        dest->writeVarUint(_fwReader->size());
        return true;
    });
}

void DfuState::endFlash()
{
    sendCmd([this](bmcl::Buffer* dest, CoderState* state) -> bool {
        TRY(photongenSerializeDfuCmd(photongen::dfu::Cmd::EndUpdate, dest, state));
        dest->writeVarUint(_sectorId);
        dest->writeVarUint(_fwReader->size());
        return true;
    });
}

void DfuState::sendNextChunk()
{
    if (_fwReader.isNull()) {
        //internal error
        endFlash();
        return;
    }
    if (!_fwReader->hasData()) {
        endFlash();
        return;
    }
    sendCmd([this](bmcl::Buffer* dest, CoderState* state) -> bool {
        TRY(photongenSerializeDfuCmd(photongen::dfu::Cmd::WriteChunk, dest, state));
        dest->writeVarUint(_fwReader->offset());
        bmcl::Bytes chunk = _fwReader->readNext(chunkSize);
        dest->writeVarUint(chunk.size());
        dest->write(chunk.data(), chunk.size());
        return true;
    });
}

const char* DfuState::name() const
{
    return "dfu";
}

void DfuState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}
}

