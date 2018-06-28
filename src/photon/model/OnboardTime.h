#pragma once

#include "photon/Config.hpp"

#include <cstdint>
#include <string>

namespace photon {

class OnboardTime {
public:
    OnboardTime()
        : _msecs(0)
    {
    }

    explicit OnboardTime(uint64_t onboardMsecs)
        : _msecs(onboardMsecs)
    {
    }

    void setRawValue(uint64_t rawValue)
    {
        _msecs = rawValue;
    }

    uint64_t rawValue() const
    {
        return _msecs;
    }

    static OnboardTime now();

    std::string toString() const;

    bool operator==(const OnboardTime& other) const
    {
        return _msecs == other._msecs;
    }

private:
    uint64_t _msecs;
};
}
