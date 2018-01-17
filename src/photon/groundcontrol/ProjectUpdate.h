#pragma once

#include "photon/Config.hpp"
#include "photon/core/Rc.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {
class Project;
class Device;
}

namespace photongen {
class Validator;
}

namespace photon {

class ValueInfoCache;
class ProjectUpdate;

using ProjectUpdateResult = bmcl::Result<Rc<const ProjectUpdate>, std::string>;

class ProjectUpdate : public RefCountable {
public:
    using Pointer = Rc<ProjectUpdate>;
    using ConstPointer = Rc<const ProjectUpdate>;

    ~ProjectUpdate();

    static ProjectUpdateResult fromProjectAndName(const decode::Project* project, bmcl::StringView name);
    static ProjectUpdateResult fromMemory(const void* src, std::size_t size);
    static ProjectUpdateResult fromMemory(bmcl::Bytes memory);

    bmcl::Buffer serialize() const;

    const decode::Project* project() const;
    const decode::Device* device() const;
    const photongen::Validator* interface() const;
    const ValueInfoCache* cache() const;

private:
    Rc<const decode::Project> _project;
    Rc<const decode::Device> _device;
    Rc<const photongen::Validator> _interface;
    Rc<const ValueInfoCache> _cache;

private:
    ProjectUpdate();
};
}
