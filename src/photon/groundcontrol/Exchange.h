/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/core/Rc.h"
#include "decode/core/HashMap.h"
#include "photon/groundcontrol/Packet.h"

#include <bmcl/Fwd.h>
#include <bmcl/Option.h>

#include <caf/event_based_actor.hpp>
#include <caf/response_promise.hpp>

#include <deque>
#include <chrono>

namespace photon {

class Client;
class Package;
struct QueuedPacket;

struct QueuedPacket {
    using TimePoint = uint64_t;

    QueuedPacket(const PacketRequest& req, uint16_t counter, TimePoint time, const caf::response_promise& promise)
        : request(req)
        , counter(counter)
        , queueTime(time)
        , promise(promise)
        , checkId(0)
    {
    }

    PacketRequest request;
    uint16_t counter;
    TimePoint queueTime;
    caf::response_promise promise;
    std::size_t checkId;
};

struct StreamState {
    StreamState(StreamType type);

    std::deque<QueuedPacket> queue;
    std::chrono::milliseconds checkTimeout;
    uint16_t currentReliableUplinkCounter;
    uint16_t currentUnreliableUplinkCounter;
    uint16_t expectedReliableDownlinkCounter;
    uint16_t expectedUnreliableDownlinkCounter;
    caf::actor client;
    StreamType type;
    std::size_t checkId;
};

class Exchange : public caf::event_based_actor {
public:
    Exchange(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
             const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler);
    ~Exchange();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    template <typename... A>
    void sendAllStreams(A&&... args);

    bmcl::SharedBytes packPacket(const PacketRequest& req, PacketType streamType, uint16_t counter, uint64_t time);

    void reportError(std::string&& msg);
    void sendUnreliablePacket(const PacketRequest& packet);
    void sendUnreliablePacket(const PacketRequest& packet, StreamState* state);
    caf::response_promise queueReliablePacket(const PacketRequest& packet);
    caf::response_promise queueReliablePacket(const PacketRequest& packet, StreamState* state);

    void packAndSendFirstQueued(StreamState* state);

    bool handlePayload(bmcl::Bytes data);
    void checkQueue(StreamState* stream);

    bool acceptPacket(const PacketHeader& header, bmcl::Bytes payload, StreamState* state);
    bool acceptReceipt(const PacketHeader& header, bmcl::Bytes payload, StreamState* state);

    void handleReceipt(const PacketHeader& header, ReceiptType type, bmcl::Bytes payload, StreamState* state, QueuedPacket* packet);

    void logMsg(std::string&& msg);

    StreamState _fwtStream;
    StreamState _cmdStream;
    StreamState _userStream;
    StreamState _dfuStream;
    StreamState _tmStream;
    caf::actor _gc;
    caf::actor _sink;
    caf::actor _handler;
    uint64_t _selfAddress;
    uint64_t _deviceAddress;
    bool _isRunning;
    bool _dataReceived;
    bool _isLoggingEnabled;
};
}
