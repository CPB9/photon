#include "photon/model/OnboardTime.h"

#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace photon {

OnboardTime OnboardTime::now()
{
    auto t = std::chrono::system_clock::now().time_since_epoch();
    return OnboardTime(std::chrono::duration_cast<std::chrono::milliseconds>(t).count());
}

std::string OnboardTime::toString() const
{
    uint64_t milliseconds = _msecs % 1000;
    std::time_t seconds = _msecs / 1000;
    std::stringstream stream;
    stream << std::put_time(std::localtime(&seconds), "%Y-%m-%d %H:%M:%S.");
    return stream.str() + std::to_string(milliseconds % 1000);
}
}
