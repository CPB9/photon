#pragma once

#include "photon/Config.h"

#include <functional>

namespace photon {

struct NumberedSub {
    NumberedSub(uint32_t compNum, uint32_t msgNum)
        : compNum(compNum)
        , msgNum(msgNum)
    {
    }

    template <typename T>
    static NumberedSub fromMsg()
    {
        return NumberedSub(T::COMP_NUM, T::MSG_NUM);
    }

    uint32_t compNum;
    uint32_t msgNum;

    bool operator==(const NumberedSub& other) const
    {
        return compNum == other.compNum && msgNum == other.msgNum;
    }

    uint64_t id() const
    {
        return (uint64_t(compNum) << 32) | uint64_t(msgNum);
    }
};

struct NumberedSubHash
{
    std::size_t operator()(const NumberedSub& sub) const noexcept
    {
        return std::hash<uint64_t>{}(sub.id());
    }
};
}
