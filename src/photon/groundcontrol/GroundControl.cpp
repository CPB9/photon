/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/GroundControl.h"
#include "photon/groundcontrol/Exchange.h"
#include "decode/parser/Project.h"
#include "decode/core/DataReader.h"
#include "photon/model/Value.h"
#include "photon/groundcontrol/Atoms.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/Crc.h"
#include "photon/groundcontrol/CmdState.h"
#include "photon/groundcontrol/GcCmd.h"
#include "photon/groundcontrol/ProjectUpdate.h"
#include "photon/groundcontrol/TmState.h"

#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/MemReader.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::DataReader::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::GcCmd);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Value);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::ProjectUpdate::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(std::vector<photon::Value>);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::NumberedSub);

#define GC_LOG(msg)         \
    if (_isLoggingEnabled) { \
        logMsg(msg);         \
    }

namespace photon {

GroundControl::GroundControl(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
                             const caf::actor& sink, const caf::actor& eventHandler)
    : caf::event_based_actor(cfg)
    , _sink(sink)
    , _handler(eventHandler)
    , _isRunning(false)
    , _isLoggingEnabled(false)
{
    _exc = spawn<Exchange, caf::linked>(selfAddress, destAddress, this, _sink, _handler);
    _cmd = spawn<CmdState, caf::linked>(_exc, _handler);
}

GroundControl::~GroundControl()
{
}

void GroundControl::logMsg(std::string&& msg)
{
    send(_handler, LogAtom::value, std::move(msg));
}

caf::behavior GroundControl::make_behavior()
{
    return caf::behavior{
        [this](RecvDataAtom, const bmcl::SharedBytes& data) {
            acceptData(data);
        },
        [this](SendUnreliablePacketAtom, const PacketRequest& packet) {
            sendUnreliablePacket(packet);
        },
        [this](SendReliablePacketAtom atom, const PacketRequest& packet) {
            return delegate(_exc, atom, packet);
        },
        [this](SendGcCommandAtom atom, const GcCmd& cmd) {
            return delegate(_cmd, atom, cmd);
        },
        [this](SubscribeNumberedTmAtom atom, const NumberedSub& sub, const caf::actor& dest) {
            return delegate(_exc, atom, sub, dest);
        },
        [this](SubscribeNamedTmAtom atom, const std::string& path, const caf::actor& dest) {
            return delegate(_exc, atom, path, dest);
        },
        [this](SendCustomCommandAtom atom, const std::string& compName, const std::string& cmdName, const std::vector<Value>& args) {
            return delegate(_cmd, atom, compName, cmdName, args);
        },
        [this](StartAtom) {
            _isRunning = true;
            send(_exc, StartAtom::value);
        },
        [this](StopAtom) {
            _isRunning = false;
            send(_exc, StopAtom::value);
        },
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            if (_project == update->project() && _dev == update->device()) {
                return;
            }
            updateProject(update);
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            send(_exc, EnableLoggindAtom::value, isEnabled);
            _isLoggingEnabled = isEnabled;
        },
        [this](UpdateFirmware) {
            send(_exc, UpdateFirmware::value);
        },
        [this](RequestDfuStatus) {
            return delegate(_exc, RequestDfuStatus::value);
        },
        [this](FlashDfuFirmware atom, std::uintmax_t id, const Rc<decode::DataReader>& reader) {
            return delegate(_exc, atom, id, reader);
        },
    };
}

void GroundControl::reportError(std::string&& msg)
{
    //FIXME
}

const char* GroundControl::name() const
{
    return "GroundControl";
}

void GroundControl::on_exit()
{
    destroy(_sink);
    destroy(_handler);
    destroy(_exc);
    destroy(_cmd);
}

void GroundControl::updateProject(const ProjectUpdate::ConstPointer& update)
{
    _project = update->project();
    _dev = update->device();
    send(_cmd, SetProjectAtom::value, update);
    send(_exc, SetProjectAtom::value, update);
    send(_handler, SetProjectAtom::value, update);
}

void GroundControl::sendUnreliablePacket(const PacketRequest& packet)
{
    send(_exc, SendUnreliablePacketAtom::value, packet);
}

void GroundControl::acceptData(const bmcl::SharedBytes& data)
{
    GC_LOG("gc accepting data of size " + std::to_string(data.size()));
    GC_LOG("gc total size " + std::to_string(_incoming.size()));
    _incoming.write(data.data(), data.size());
    if (!_isRunning) {
        GC_LOG("gc not running");
        return;
    }

begin:
    if (_incoming.size() == 0) {
        return;
    }
    SearchResult rv = findPacket(_incoming);
    GC_LOG("gc search result " + std::to_string(rv.junkSize) + " " + std::to_string(rv.dataSize));

    if (rv.dataSize) {
        bmcl::Bytes packet = _incoming.asBytes().slice(rv.junkSize, rv.junkSize + rv.dataSize);
        if (!acceptPacket(packet)) {
            _incoming.removeFront(rv.junkSize + 1);
        } else {
            _incoming.removeFront(rv.junkSize + rv.dataSize);
        }
        goto begin;
    } else {
        if (rv.junkSize) {
            _incoming.removeFront(rv.junkSize);
        }
    }
}

bool GroundControl::acceptPacket(bmcl::Bytes packet)
{
    if (packet.size() < 6) {
        reportError("recieved packet with size < 6");
        return false;
    }
    packet = packet.sliceFrom(2);

    uint16_t payloadSize = le16dec(packet.data());
    if ((payloadSize + 2u) != packet.size()) {
        reportError("recieved packet with invalid size");
        return false;
    }

    send(_exc, RecvPayloadAtom::value, bmcl::SharedBytes::create(packet.data() + 2, payloadSize - 2));
    return true;
}

SearchResult GroundControl::findPacket(bmcl::Bytes data)
{
    return findPacket(data.data(), data.size());
}

SearchResult GroundControl::findPacket(const void* data, std::size_t size)
{
    constexpr uint16_t separator = 0x9c3e;
    constexpr uint8_t firstSepPart = (separator & 0xff00) >> 8;
    constexpr uint8_t secondSepPart = separator & 0x00ff;

    const uint8_t* begin = (const uint8_t*)data;
    const uint8_t* it = begin;
    const uint8_t* end = it + size;
    const uint8_t* next;

beginSearch:
    while (true) {
        it = std::find(it, end, firstSepPart);
        next = it + 1;
        if (next >= end) {
            if (it >= end) {
                return SearchResult(size, 0);
            } else {
                return SearchResult(it - begin, 0);
            }
        }
        if (*next == secondSepPart) {
            break;
        }
        it++;
    }

    std::size_t junkSize = it - begin;
    it += 2; //skipped sep

    if ((it + 2) >= end) {
        return SearchResult(junkSize, 0);
    }

    //const uint8_t* packetBegin = it;

    uint16_t expectedSize = le16dec(it);
    if (expectedSize > 1024) {
        return SearchResult(junkSize + 2, 0);
    }
    if (it > end - expectedSize - 2) {
        return SearchResult(junkSize, 0);
    }

    uint16_t encodedCrc = le16dec(it + expectedSize);
    Crc16 calculatedCrc;
    calculatedCrc.update(it, expectedSize);
    if (calculatedCrc.get() != encodedCrc) {
        goto beginSearch;
    }

    return SearchResult(junkSize, 4 + expectedSize);
}

bmcl::Option<PacketAddress> GroundControl::extractPacketAddress(const void* data, std::size_t size)
{
    PacketAddress addr;
    bmcl::MemReader reader((const uint8_t*)data, size);
    if (size < 4) {
        return bmcl::None;
    }
    reader.skip(4);
    if (!reader.readVarUint(&addr.srcAddress)) {
        return bmcl::None;
    }
    if (!reader.readVarUint(&addr.destAddress)) {
        return bmcl::None;
    }
    return addr;
}

bmcl::Option<PacketAddress> GroundControl::extractPacketAddress(bmcl::Bytes data)
{
    return GroundControl::extractPacketAddress(data.data(), data.size());
}
}
