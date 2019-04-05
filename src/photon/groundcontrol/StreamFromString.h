#pragma once

#include "photon/Config.hpp"

#include <bmcl/Fwd.h>

#include <string>

namespace caf {
class actor;
class actor_system;
}

namespace asio {
class io_context;
}

namespace photon {

bmcl::Result<caf::actor, std::string> spawnActorFromDeviceString(caf::actor_system& system, asio::io_context& ioService, const std::string& devString);
}
