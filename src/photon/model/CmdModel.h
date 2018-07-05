/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.hpp"
#include "photon/model/Node.h"

namespace decode {
class Device;
}

namespace photon {

class ValueInfoCache;
class ScriptNode;

class CmdModel : public Node {
public:
    using Pointer = Rc<CmdModel>;
    using ConstPointer = Rc<const CmdModel>;

    CmdModel(const decode::Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~CmdModel();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

    const decode::Device* device() const;

private:
    std::vector<Rc<ScriptNode>> _nodes;
    Rc<const decode::Device> _dev;
};
}
