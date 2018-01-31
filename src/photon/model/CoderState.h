#pragma once

#include "photon/Config.hpp"
#include "photon/model/OnboardTime.h"

#include <bmcl/StringView.h>

#include <string>

namespace photon {

class CoderState {
public:
    CoderState(OnboardTime time)
        : _time(time)
    {
    }

    void setError(bmcl::StringView msg)
    {
        _error.assign(msg.begin(), msg.end());
    }

    const std::string& error() const
    {
        return _error;
    }

    OnboardTime dataTimeOfOrigin() const
    {
        return _time;
    }

private:
    std::string _error;
    OnboardTime _time;
};
}
