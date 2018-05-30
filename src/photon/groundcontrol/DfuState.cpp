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

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);

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

caf::behavior DfuState::make_behavior()
{
    return caf::behavior{
        [this](RecvPacketPayloadAtom, const PacketHeader& header, const bmcl::SharedBytes& packet) {
            BMCL_DEBUG() << "recieved dfu packet";
        },
        [this](StartAtom) {
            _temp.writeVarInt(0);
            PacketRequest req;
            req.streamType = StreamType::Dfu;
            req.payload = bmcl::SharedBytes::create(_temp.asBytes());
            send(_exc, SendUnreliablePacketAtom::value, req);
            _temp.resize(0);
        },
        [this](StopAtom) {
        },
        [this](EnableLoggindAtom, bool isEnabled) {
        },
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
        },
    };
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

