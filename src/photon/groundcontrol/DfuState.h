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

#include "photongen/groundcontrol/dfu/SectorData.hpp"
#include "photongen/groundcontrol/dfu/AllSectorsDesc.hpp"

#include <bmcl/Fwd.h>
#include <bmcl/Buffer.h>

#include <caf/event_based_actor.hpp>

namespace decode {
class DataReader;
}

namespace photon {

class ProjectUpdate;

struct DfuStatus : public RefCountable {
    photongen::dfu::SectorData sectorData;
    photongen::dfu::AllSectorsDesc sectorDesc;
};

class DfuState : public caf::event_based_actor {
public:
    DfuState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& eventHandler);
    ~DfuState();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    template <typename C, typename... A>
    void sendCmd(C&& ser, A&&... args);

    void handleReportInfo(bmcl::MemReader* reader, CoderState* state);
    void handleBegin(bmcl::MemReader* reader, CoderState* state);
    void handleWrite(bmcl::MemReader* reader, CoderState* state);
    void handleEnd(bmcl::MemReader* reader, CoderState* state);
    void handleError(bmcl::MemReader* reader, CoderState* state);

    void beginUpload();
    void sendNextChunk();
    void endFlash();

    caf::actor _exc;
    caf::actor _handler;
    Rc<const ProjectUpdate> _projectUpdate;
    bmcl::Buffer _temp;
    std::vector<caf::response_promise> _statesPromises;
    Rc<decode::DataReader> _fwReader;
    std::uintmax_t _sectorId;
    caf::response_promise _flashPromise;
};
}

