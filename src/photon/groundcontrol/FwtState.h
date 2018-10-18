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
#include "photon/groundcontrol/MemIntervalSet.h"

#include <bmcl/Fwd.h>
#include <bmcl/Buffer.h>
#include <bmcl/Option.h>

#include <caf/event_based_actor.hpp>

#include <array>
#include <cstring>

namespace decode {
class Project;
class Device;
}

namespace photon {

class Sender;
class Scheduler;
class ValueInfoCache;

class FwtState : public caf::event_based_actor {
public:
    FwtState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& eventHandler);
    ~FwtState();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    using HashContainer = std::array<uint8_t, 64>;

    void acceptData(bmcl::Bytes packet);
    void handleHashAction();
    void handleCheckAction(std::size_t id);

    template <typename C, typename... A>
    void packAndSendPacket(C&& enc, A&&... args);

    void acceptChunkResponse(bmcl::MemReader* src);
    void acceptHashResponse(bmcl::MemReader* src);
    void acceptStopResponse(bmcl::MemReader* src);

    void genHashCmd(bmcl::MemWriter* dest);
    void genChunkCmd(bmcl::MemWriter* dest, MemInterval os);
    void genChunksCmd(bmcl::MemWriter* dest, const std::vector<MemInterval>& chunks);
    void genStopCmd(bmcl::MemWriter* dest);

    void scheduleHash();
    void scheduleCheck();

    void checkIntervals();
    void readFirmware();

    void reportFirmwareError(std::string&& msg);
    void logMsg(std::string&& msg);

    void startDownload();
    void stopDownload();
    void restartChunkDownload();
    void setProject(const decode::Project* proj, const decode::Device* dev, const ValueInfoCache* cache);

    bool hashMatches(const HashContainer& hash, bmcl::Bytes data);

    bmcl::Option<HashContainer> _hash;
    bmcl::Option<HashContainer> _downloadedHash;

    MemIntervalSet _acceptedChunks;
    bmcl::Buffer _desc;
    std::string _deviceName;

    bool _isRunning;
    bool _isDownloading;
    bool _isLoggingEnabled;
    std::size_t _checkId;
    uint8_t _temp[100];
    caf::actor _exc;
    caf::actor _handler;
    Rc<const decode::Project> _project;
    Rc<const decode::Device> _device;
    Rc<const ValueInfoCache> _cache;
};
}
