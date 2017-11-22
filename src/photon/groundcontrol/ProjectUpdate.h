#pragma once

#include "photon/Config.h"
#include "photon/core/Rc.h"
#include "photon/groundcontrol/AllowUnsafeMessageType.h"

namespace decode {
class Project;
struct Device;
}

namespace photon {

class ValueInfoCache;

struct ProjectUpdate {
    ProjectUpdate(const decode::Project* project, const decode::Device* device, const ValueInfoCache* cache);
    ProjectUpdate(const ProjectUpdate& other);
    ProjectUpdate(ProjectUpdate&& other);
    ~ProjectUpdate();

    Rc<const decode::Project> project;
    Rc<const decode::Device> device;
    Rc<const ValueInfoCache> cache;
};
}

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(photon::ProjectUpdate);
