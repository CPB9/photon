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
#include "photongen/groundcontrol/dfu/Cmd.hpp"
#include "photongen/groundcontrol/dfu/Response.hpp"

#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::Rc<const photon::DfuStatus>);

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
        PacketRequest req;
        req.streamType = StreamType::Dfu;
        req.payload = bmcl::SharedBytes::create(_temp.asBytes());
        send(_exc, SendUnreliablePacketAtom::value, req);
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
                Rc<DfuStatus> status = new DfuStatus;
                if (!photongenDeserializeDfuSectorData(&status->sectorData, &reader, &state)) {
                    //TODO: report error
                    return;
                }
                if (!photongenDeserializeDfuAllSectorsDesc(&status->sectorDesc, &reader, &state)) {
                    //TODO: report error
                    return;
                }
                for (caf::response_promise& promise : _statesPromises) {
                    promise.deliver(Rc<const DfuStatus>(status));
                }
                _statesPromises.clear();
                break;
            }
        },
        [this](StartAtom) {
        },
        [this](StopAtom) {
        },
        [this](EnableLoggindAtom, bool isEnabled) {
        },
        [this](SetProjectAtom, const ProjectUpdate::ConstPointer& update) {
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

