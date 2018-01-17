#pragma once

#include "photon/Config.hpp"

namespace photon {

struct TmFeatures {
public:
    TmFeatures()
        : hasPosition(false)
        , hasOrientation(false)
        , hasVelocity(false)
    {
    }

    bool hasPosition;
    bool hasOrientation;
    bool hasVelocity;
};

}
