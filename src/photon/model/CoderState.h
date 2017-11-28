#pragma once

#include <bmcl/StringView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Buffer.h>

#include <string>

namespace photon {

class CoderState {
public:
    void setError(bmcl::StringView msg)
    {
        _error.assign(msg.begin(), msg.end());
    }

    const std::string error() const
    {
        return _error;
    }

private:
    std::string _error;
};
}
