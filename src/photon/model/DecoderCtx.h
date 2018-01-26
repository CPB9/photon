#pragma once

#include "photon/Config.hpp"
#include "photon/model/OnboardTime.h"

namespace photon {

class DecoderCtx {
public:
    DecoderCtx(OnboardTime time)
        : _time(time)
    {
    }

    OnboardTime dataTimeOfOrigin() const
    {
        return _time;
    }

private:
    OnboardTime _time;
};
}
