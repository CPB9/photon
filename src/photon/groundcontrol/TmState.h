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
#include "photon/groundcontrol/TmFeatures.h"
#include "photon/groundcontrol/NumberedSub.h"

#include <bmcl/Fwd.h>

#include <caf/event_based_actor.hpp>
namespace decode {
class Project;
class Device;
} // namespace decode
namespace photon {

struct PacketHeader;
class TmModel;
class ValueNode;

template <typename T>
class NumericValueNode;

class TmState : public caf::event_based_actor {
public:

    TmState(caf::actor_config& cfg, const caf::actor& handler);
    ~TmState();

    caf::behavior make_behavior() override;
    void on_exit() override;

    static std::size_t hash(const NumberedSub& sub);

private:
    struct NamedSub {
        NamedSub(const ValueNode* node, const std::string& path, const caf::actor& dest);
        ~NamedSub();

        Rc<const ValueNode> node;
        std::string path;
        caf::actor actor;
    };

    void acceptData(const PacketHeader& header, bmcl::Bytes packet);
    template <typename T>
    void initTypedNode(const char* name, Rc<T>* dest);
    void pushTmUpdates();
    bool subscribeTm(const std::string& path, const caf::actor& dest);
    bool subscribeTm(const NumberedSub& sub, const caf::actor& dest);
    void reportError(std::string&& msg);
    void logMsg(std::string&& msg);

    Rc<const decode::Project> _project;
    Rc<const decode::Device> _dev;

    Rc<TmModel> _model;
    caf::actor _handler;
    std::vector<NamedSub> _namedSubs;
    std::unordered_map<NumberedSub, std::vector<caf::actor>, NumberedSubHash> _numberedSubs;
    uint64_t _updateCount;
    bool _isLoggingEnabled;
};
}
