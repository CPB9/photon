#pragma once

#include "photon/Config.hpp"
#include "decode/core/Id.h"

#include <functional>

namespace photon {

struct NumberedSub {
    NumberedSub(decode::Id msgNum)
        : msgNum(msgNum)
    {
    }

    decode::Id msgNum;

    bool operator==(const NumberedSub& other) const
    {
        return msgNum == other.msgNum;
    }

    decode::Id id() const
    {
        return msgNum;
    }
};

struct NumberedSubHash
{
    std::size_t operator()(const NumberedSub& sub) const noexcept
    {
        return std::hash<decode::Id>{}(sub.id());
    }
};
}
