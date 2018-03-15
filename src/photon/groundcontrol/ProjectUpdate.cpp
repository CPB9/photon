#include "photon/groundcontrol/ProjectUpdate.h"
#include "photon/model/ValueInfoCache.h"
#include "decode/parser/Project.h"
#include "decode/parser/Package.h"

#include "photongen/groundcontrol/Validator.hpp"

#include <decode/core/Utils.h>
#include <decode/parser/Project.h>
#include <decode/core/Diagnostics.h>

#include <bmcl/Result.h>
#include <bmcl/Bytes.h>
#include <bmcl/MemReader.h>

namespace photon {

ProjectUpdate::ProjectUpdate()
{
}

ProjectUpdate::~ProjectUpdate()
{
}

ProjectUpdateResult ProjectUpdate::fromProjectAndName(const decode::Project* project, bmcl::StringView name)
{
    bmcl::OptionPtr<const decode::Device> dev = project->deviceWithName(name);
    if (dev.isNone()) {
        return "no device with name '" + name.toStdString() + "'";
    }

    Rc<ProjectUpdate> update = new ProjectUpdate;
    update->_project = project;
    update->_device = dev.unwrap();
    update->_interface = new photongen::Validator(project, dev.unwrap());
    update->_cache = new ValueInfoCache(project->package());
    return Rc<const ProjectUpdate>(update);
}

ProjectUpdateResult ProjectUpdate::fromMemory(const void* src, std::size_t size)
{
    bmcl::MemReader reader(src, size);
    if (reader.sizeLeft() < 8) {
        return ProjectUpdateResult("unexpected end of stream");
    }

    uint64_t projSize = reader.readUint64Le();
    if (reader.sizeLeft() < projSize) {
        return ProjectUpdateResult("unexpected end of stream");
    }

    Rc<decode::Diagnostics> diag = new decode::Diagnostics();
    auto proj = decode::Project::decodeFromMemory(diag.get(), reader.current(), projSize);
    if (proj.isErr()) {
        return ProjectUpdateResult("error deserializing project");
    }
    reader.skip(projSize);

    auto name = decode::deserializeString(&reader);
    if (name.isErr()) {
        return name.takeErr();
    }

    return ProjectUpdate::fromProjectAndName(proj.unwrap().get(), name.unwrap());
}

ProjectUpdateResult ProjectUpdate::fromMemory(bmcl::Bytes memory)
{
    return fromMemory(memory.data(), memory.size());
}

bmcl::Buffer ProjectUpdate::serialize() const
{
    bmcl::Buffer proj = _project->encode();
    bmcl::Buffer result;
    result.reserve(8 + proj.size() + 1 + _device->name().size());
    result.writeUint64Le(proj.size());
    result.write(proj.data(), proj.size());
    decode::serializeString(_device->name(), &result);
    return result;
}

const decode::Project* ProjectUpdate::project() const
{
    return _project.get();
}

const decode::Device* ProjectUpdate::device() const
{
    return _device.get();
}

const photongen::Validator* ProjectUpdate::interface() const
{
    return _interface.get();
}

const ValueInfoCache* ProjectUpdate::cache() const
{
    return _cache.get();
}
}
