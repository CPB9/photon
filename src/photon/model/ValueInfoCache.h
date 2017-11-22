/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "photon/Config.h"
#include "photon/core/Rc.h"
#include "decode/core/HashMap.h"

#include <bmcl/StringView.h>

#include <string>

namespace decode {
class Type;
class Package;
}

namespace photon {

class ValueInfoCache : public RefCountable {
public:
    using Pointer = Rc<ValueInfoCache>;
    using ConstPointer = Rc<const ValueInfoCache>;

    struct Hasher {
        std::size_t operator()(const Rc<const decode::Type>& key) const
        {
            return std::hash<const decode::Type*>()(key.get());
        }
    };

    using MapType = decode::HashMap<Rc<const decode::Type>, std::string, Hasher>;
    using StringVecType = std::vector<bmcl::StringView>;

    ValueInfoCache(const decode::Package* package);
    ~ValueInfoCache();

    bmcl::StringView arrayIndex(std::size_t idx) const;
    bmcl::StringView nameForType(const decode::Type* type) const;

private:
    StringVecType _arrayIndexes;
    MapType _names;
};
}
