/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "photon/groundcontrol/Exchange.h"
#include "photon/groundcontrol/Crc.h"
#include "photon/groundcontrol/Atoms.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"
#include "photon/groundcontrol/Packet.h"
#include "photon/groundcontrol/FwtState.h"
#include "photon/groundcontrol/DfuState.h"
#include "photon/groundcontrol/TmState.h"
#include "photon/groundcontrol/ProjectUpdate.h"
#include "photon/model/OnboardTime.h"
#include "decode/parser/Project.h"
#include "decode/core/DataReader.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Buffer.h>
#include <bmcl/Result.h>
#include <bmcl/SharedBytes.h>
#include <bmcl/String.h>
#include <bmcl/Panic.h>

#include <sstream>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketHeader);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::ProjectUpdate::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::DataReader::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::StreamState*);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::NumberedSub);

#define EXC_LOG(msg)         \
    if (_isLoggingEnabled) { \
        logMsg(msg);         \
    }


namespace photon {

constexpr const std::chrono::milliseconds defaultCheckTimeout = std::chrono::milliseconds(1500);
constexpr const std::chrono::milliseconds maxCheckTimeout = defaultCheckTimeout * 4;
constexpr const unsigned checkMultiplier = 2;

StreamState::StreamState(StreamType type)
    : checkTimeout(defaultCheckTimeout)
    , currentReliableUplinkCounter(0)
    , currentUnreliableUplinkCounter(0)
    , expectedReliableDownlinkCounter(0)
    , expectedUnreliableDownlinkCounter(0)
    , type(type)
    , checkId(0)
{
}

Exchange::Exchange(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
                   const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _fwtStream(StreamType::Firmware)
    , _cmdStream(StreamType::Cmd)
    , _userStream(StreamType::User)
    , _dfuStream(StreamType::Dfu)
    , _tmStream(StreamType::Telem)
    , _gc(gc)
    , _sink(dataSink)
    , _handler(handler)
    , _selfAddress(selfAddress)
    , _deviceAddress(destAddress)
    , _isRunning(false)
    , _dataReceived(false)
    , _isLoggingEnabled(false)
{
    _fwtStream.client = spawn<FwtState, caf::linked>(this, _handler);
    _tmStream.client = spawn<TmState, caf::linked>(_handler);
    _dfuStream.client = spawn<DfuState, caf::linked>(this, _handler);
}

Exchange::~Exchange()
{
}

void Exchange::logMsg(std::string&& msg)
{
    send(_handler, LogAtom::value, std::move(msg));
}

template <typename... A>
void Exchange::sendAllStreams(A&&... args)
{
    send(_fwtStream.client, std::forward<A>(args)...);
    send(_cmdStream.client, std::forward<A>(args)...);
    send(_tmStream.client, std::forward<A>(args)...);
    send(_userStream.client, std::forward<A>(args)...);
    send(_dfuStream.client, std::forward<A>(args)...);
}

using CheckQueueAtom = caf::atom_constant<caf::atom("checkqu")>;

caf::behavior Exchange::make_behavior()
{
    return caf::behavior{
        [this](RecvPayloadAtom, const bmcl::SharedBytes& data) {
            _dataReceived = true;
            handlePayload(data.view());
        },
        [this](CheckQueueAtom, StreamType type, std::size_t id) {
            StreamState* state;
            switch (type) {
            case StreamType::Firmware:
                state = &_fwtStream;
                break;
            case StreamType::Cmd:
                state = &_cmdStream;
                break;
            case StreamType::User:
                state = &_userStream;
                break;
            case StreamType::Dfu:
                state = &_dfuStream;
                break;
            case StreamType::Telem:
                state = &_tmStream;
                break;
            }
            if (state->queue.empty()) {
                return;
            }
            QueuedPacket& queuedPacket = state->queue[0];
            if (queuedPacket.checkId != id) {
                return;
            }
            checkQueue(state);
        },
        [this](SendUnreliablePacketAtom, const PacketRequest& packet) {
            sendUnreliablePacket(packet);
        },
        [this](SendReliablePacketAtom, const PacketRequest& packet) {
            return queueReliablePacket(packet);
        },
        [this](PingAtom) {
            if (!_isRunning) {
                return;
            }
            if (!_dataReceived) {
                PacketRequest req(bmcl::Bytes(), StreamType::Cmd);
                send(this, SendUnreliablePacketAtom::value, std::move(req));
            }
            _dataReceived = false;
            //delayed_send(this, std::chrono::seconds(1), PingAtom::value); FIXME: redo
        },
        [this](SubscribeNumberedTmAtom atom, const NumberedSub& sub, const caf::actor& dest) {
            return delegate(_tmStream.client, atom, sub, dest);
        },
        [this](FlashDfuFirmware atom, std::uintmax_t id, const Rc<decode::DataReader>& reader) {
            return delegate(_dfuStream.client, atom, id, reader);
        },
        [this](SubscribeNamedTmAtom atom, const std::string& path, const caf::actor& dest) {
            return delegate(_tmStream.client, atom, path, dest);
        },
        [this](StartAtom) {
            _isRunning = true;
            _dataReceived = false;
            sendAllStreams(StartAtom::value);
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](StopAtom) {
            _isRunning = false;
            sendAllStreams(StopAtom::value);
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            _isLoggingEnabled = isEnabled;
            sendAllStreams(EnableLoggindAtom::value, isEnabled);
        },
        [this](UpdateFirmware) {
            send(_fwtStream.client, UpdateFirmware::value);
        },
        [this](RequestDfuStatus) {
            return delegate(_dfuStream.client, RequestDfuStatus::value);
        },
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
            sendAllStreams(SetProjectAtom::value, update);
            send(_gc, SetProjectAtom::value, update);
        },
    };
}

const char* Exchange::name() const
{
    return "Exchange";
}

void Exchange::on_exit()
{
    destroy(_gc);
    destroy(_sink);
    destroy(_handler);
    destroy(_fwtStream.client);
    destroy(_cmdStream.client);
    destroy(_tmStream.client);
    destroy(_userStream.client);
}

void Exchange::reportError(std::string&& msg)
{
    send(_handler, ExchangeErrorEventAtom::value, std::move(msg));
}

static bmcl::Result<PacketHeader, std::string> decodeHeader(bmcl::MemReader* reader)
{
    PacketHeader header;
    if (!reader->readVarUint(&header.srcAddress)) {
        return std::string("recieved invalid src address");
    }
    if (!reader->readVarUint(&header.destAddress)) {
        return std::string("recieved invalid dest address");
    }
    int64_t direction;
    if (!reader->readVarInt(&direction)) {
        return std::string("recieved invalid stream direction");
    }
    switch (direction) {
    case 0: //uplink
        header.streamDirection = StreamDirection::Uplink;
        break;
    case 1: //downlink
        header.streamDirection = StreamDirection::Downlink;
        break;
    }

    int64_t packetType;
    if (!reader->readVarInt(&packetType)) {
        return std::string("recieved invalid stream type");
    }
    switch (packetType) {
    case 0:
        header.packetType = PacketType::Unreliable;
        break;
    case 1:
        header.packetType = PacketType::Reliable;
        break;
    case 2:
        header.packetType = PacketType::Receipt;
        break;
    default:
        return std::string("recieved invalid stream type");
    }

    int64_t streamType;
    if (!reader->readVarInt(&streamType)) {
        return std::string("recieved invalid packet type");
    }
    switch (streamType) {
    case 0:
        header.streamType = StreamType::Firmware;
        break;
    case 1:
        header.streamType = StreamType::Cmd;
        break;
    case 2:
        header.streamType = StreamType::Telem;
        break;
    case 3:
        header.streamType = StreamType::User;
        break;
    case 4:
        header.streamType = StreamType::Dfu;
        break;
    default:
        return std::string("recieved invalid packet type");
    }

    if (reader->sizeLeft() < 2) {
        return std::string("recieved invalid counter");
    }
    header.counter = reader->readUint16Le();

    uint64_t tickTime;
    if (!reader->readVarUint(&tickTime)) {
        return std::string("recieved invalid time");
    }

    header.tickTime.setRawValue(tickTime);
    return header;
}

bool Exchange::handlePayload(bmcl::Bytes data)
{
    if (data.size() == 0) {
        reportError("recieved empty payload");
        return false;
    }

    bmcl::MemReader reader(data);

    auto rv = decodeHeader(&reader);
    if (rv.isErr()) {
        reportError(rv.takeErr());
        return false;
    }

    const PacketHeader& header = rv.unwrap();

    if (header.srcAddress != _deviceAddress) {
        reportError("recieved wrong src address");
        return false;
    }

    if (header.destAddress != _selfAddress) {
        reportError("recieved wrong dest address");
        return false;
    }

    bmcl::Bytes userData(reader.current(), reader.end());
    switch (header.streamType) {
    case StreamType::Firmware:
        return acceptPacket(header, userData, &_fwtStream);
    case StreamType::Cmd:
        return acceptPacket(header, userData, &_cmdStream);
    case StreamType::User:
        return acceptPacket(header, userData, &_userStream);
    case StreamType::Dfu:
        return acceptPacket(header, userData, &_dfuStream);
    case StreamType::Telem:
        return acceptPacket(header, userData, &_tmStream);
    default:
        reportError("recieved invalid packet type: " + std::to_string((int64_t)header.streamType));
        return false;
    }

    return false;
}

void Exchange::handleReceipt(const PacketHeader& header, ReceiptType type, bmcl::Bytes payload, StreamState* state, QueuedPacket* packet)
{
    state->checkTimeout = defaultCheckTimeout;
    const bmcl::Uuid& uuid = state->queue.front().request.requestUuid;
    PacketResponse resp(uuid, bmcl::SharedBytes::create(payload), type, header.tickTime, header.counter);
    packet->promise.deliver(std::move(resp));
    state->queue.pop_front();
    checkQueue(state);
}

bool Exchange::acceptReceipt(const PacketHeader& header, bmcl::Bytes payload, StreamState* state)
{
    bmcl::MemReader reader(payload);
    int64_t receiptType;
    if (!reader.readVarInt(&receiptType)) {
        reportError("recieved invalid receipt type " + std::to_string(receiptType));
        return false;
    }

    if (state->queue.empty()) {
        reportError("recieved receipt, but no packets queued");
        return false;
    }
    QueuedPacket& packet = state->queue[0];
    //TODO: handle errors
    switch (receiptType) {
    case 0: { //ok
        if (packet.counter == header.counter) {
            state->currentReliableUplinkCounter++;
            handleReceipt(header, ReceiptType::Ok, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    }
    case 1: //TODO: packet error
        if (packet.counter == header.counter) {
            handleReceipt(header, ReceiptType::PacketError, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            reportError("packet error");
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    case 2: //TODO: payload error
        if (packet.counter == header.counter) {
            handleReceipt(header, ReceiptType::PayloadError, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            reportError("payload error");
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    case 3: { //counter correction
        if (reader.readableSize() < 2) {
            reportError("recieved invalid counter correction");
            return false;
        }
        uint16_t newCounter = reader.readUint16Le();
        auto rv = decodeHeader(&reader);
        if (rv.isErr()) {
            reportError("failed to decode counter correction header: " + rv.unwrapErr());
        } else {
            if (!state->queue.empty() && state->queue[0].queueTime != rv.unwrap().tickTime.rawValue()) {
                reportError("recieved outdated counter correction: timeQueued(" + std::to_string(state->queue[0].queueTime) + ") timeCorr(" + std::to_string(rv.unwrap().tickTime.rawValue()) + ")");
                return true;
            }
        }
        if (newCounter == state->currentReliableUplinkCounter) {
            return true;
        }
        reportError("recieved counter correction: new(" + std::to_string(newCounter) + "), old(" + std::to_string(state->currentReliableUplinkCounter) + ")");
        state->currentReliableUplinkCounter = newCounter;
        state->checkTimeout = defaultCheckTimeout;
        checkQueue(state);
        return true;
    }
    default:
        reportError("recieved invalid receipt type");
        return false;
    }

    return true;
}

static std::string printHeader(const PacketHeader& header)
{
    std::ostringstream out;
    out << "{" << header.tickTime.rawValue() << ", ";
    out << header.srcAddress << ", ";
    out << header.destAddress << ", ";
    out << header.counter << ", ";
    out << (int)header.streamDirection << ", ";
    out << (int)header.packetType << ", ";
    out << (int)header.streamType << "}";

    return out.str();
}

bool Exchange::acceptPacket(const PacketHeader& header, bmcl::Bytes payload, StreamState* state)
{
    if (header.streamDirection != StreamDirection::Downlink) {
        reportError("invalid stream direction"); //TODO: msg
        return false;
    }
    EXC_LOG("exc recieved packet " + printHeader(header));
    switch (header.packetType) {
    case PacketType::Unreliable:
        send(state->client, RecvPacketPayloadAtom::value, header, bmcl::SharedBytes::create(payload));
        break;
    case PacketType::Reliable:
        // unsupported
        reportError("reliable downlink packets not supported"); //TODO: msg
        return false;
    case PacketType::Receipt:
        acceptReceipt(header, payload, state);
        break;
    }
    return true;
}

bmcl::SharedBytes Exchange::packPacket(const PacketRequest& req, PacketType packetType, uint16_t counter, uint64_t time)
{
    //BMCL_DEBUG() << counter;
    uint8_t header[8 + 8 + 8 + 8 + 8 + 2 + 8];
    bmcl::MemWriter headerWriter(header, sizeof(header));
    headerWriter.writeVarUint(_selfAddress);
    headerWriter.writeVarUint(_deviceAddress);
    headerWriter.writeVarInt((int64_t)StreamDirection::Uplink);
    headerWriter.writeVarInt((int64_t)packetType);
    headerWriter.writeVarInt((int64_t)req.streamType);
    headerWriter.writeUint16Le(counter);
    headerWriter.writeVarUint(time); //time

    //TODO: check overflow
    std::size_t packetSize = headerWriter.writenData().size() + req.payload.size() + 2;
    bmcl::SharedBytes packet = bmcl::SharedBytes::create(4 + packetSize);
    bmcl::MemWriter packWriter(packet.data(), packet.size());
    packWriter.writeUint16Be(0x9c3e);
    packWriter.writeUint16Le(packetSize);
    packWriter.write(headerWriter.writenData());
    packWriter.write(req.payload.view());
    Crc16 crc;
    crc.update(packWriter.writenData().sliceFrom(2));
    packWriter.writeUint16Le(crc.get());
    assert(packWriter.sizeLeft() == 0);
    return packet;
}

void Exchange::checkQueue(StreamState* state)
{
    if (state->queue.empty()) {
        return;
    }
    QueuedPacket& queuedPacket = state->queue[0];
    queuedPacket.counter = state->currentReliableUplinkCounter;
    bmcl::SharedBytes packet = packPacket(queuedPacket.request, PacketType::Reliable, queuedPacket.counter, queuedPacket.queueTime);
    send(_sink, RecvDataAtom::value, packet);
    delayed_send(this, state->checkTimeout, CheckQueueAtom::value, state->type, queuedPacket.checkId);
    if (state->checkTimeout < maxCheckTimeout) {
        state->checkTimeout *= checkMultiplier;
    }
}

void Exchange::sendUnreliablePacket(const PacketRequest& req, StreamState* state)
{
    auto time = std::chrono::system_clock::now().time_since_epoch().count();
    bmcl::SharedBytes packet = packPacket(req, PacketType::Unreliable, state->currentUnreliableUplinkCounter, time);
    state->currentUnreliableUplinkCounter++;
    send(_sink, RecvDataAtom::value, packet);
}

caf::response_promise Exchange::queueReliablePacket(const PacketRequest& packet, StreamState* state)
{
    auto time = std::chrono::system_clock::now().time_since_epoch().count();
    auto promise = make_response_promise();
    state->checkId++;
    state->queue.emplace_back(packet, state->currentReliableUplinkCounter, time, promise);
    state->queue.back().checkId = state->checkId;
    if (state->queue.size() == 1) {
        packAndSendFirstQueued(state);
        delayed_send(this, state->checkTimeout, CheckQueueAtom::value, state->type, state->queue.back().checkId);
        if (state->checkTimeout < maxCheckTimeout) {
            state->checkTimeout *= checkMultiplier;
        }
    }
    return promise;
}

void Exchange::packAndSendFirstQueued(StreamState* state)
{
    if (state->queue.empty()) {
        return;
    }

    const QueuedPacket& queuedPacket = state->queue[0];
    bmcl::SharedBytes packet = packPacket(queuedPacket.request, PacketType::Reliable, state->currentReliableUplinkCounter, queuedPacket.queueTime);
    send(_sink, RecvDataAtom::value, packet);
}

void Exchange::sendUnreliablePacket(const PacketRequest& req)
{
    switch (req.streamType) {
    case StreamType::Firmware:
        sendUnreliablePacket(req, &_fwtStream);
        return;
    case StreamType::Cmd:
        sendUnreliablePacket(req, &_cmdStream);
        return;
    case StreamType::User:
        sendUnreliablePacket(req, &_userStream);
        return;
    case StreamType::Dfu:
        sendUnreliablePacket(req, &_dfuStream);
        return;
    case StreamType::Telem:
        sendUnreliablePacket(req, &_tmStream);
        return;
    }
}

caf::response_promise Exchange::queueReliablePacket(const PacketRequest& req)
{
    switch (req.streamType) {
    case StreamType::Firmware:
        return queueReliablePacket(req, &_fwtStream);
    case StreamType::Cmd:
        return queueReliablePacket(req, &_cmdStream);
    case StreamType::User:
        return queueReliablePacket(req, &_userStream);
    case StreamType::Dfu:
        return queueReliablePacket(req, &_dfuStream);
    case StreamType::Telem:
        return queueReliablePacket(req, &_tmStream);
    }
    bmcl::panic("unreachable"); //TODO: add macro
}
}
