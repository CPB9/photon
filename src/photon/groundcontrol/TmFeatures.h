#pragma once

#include "photon/Config.h"

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
