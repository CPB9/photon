#pragma once

#include <bmcl/StringView.h>

namespace photon {

class CoderState {
public:
    void setError(bmcl::StringView msg);
};

}
