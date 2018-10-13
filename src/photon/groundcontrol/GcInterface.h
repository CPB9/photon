#pragma once

#include "photon/Config.hpp"
#include "photon/core/Rc.h"
#include "photon/groundcontrol/GcStructs.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {
class Device;
class BuiltinType;
class StructType;
class VariantType;
class Command;
class Component;
class Ast;
class Command;
}

namespace photon {

struct AllRoutesInfo;
class StructValueNode;
class ValueInfoCache;
class Node;
class Encoder;
class Decoder;
class ValueInfoCache;

template <typename T>
using GcInterfaceResult = bmcl::Result<Rc<T>, std::string>;

class CoreGcInterface : public RefCountable {
public:
    ~CoreGcInterface();

    static GcInterfaceResult<CoreGcInterface> create(const decode::Device* dev, const ValueInfoCache* cache);

    const decode::BuiltinType* u8Type() const;
    const decode::BuiltinType* u16Type() const;
    const decode::BuiltinType* u64Type() const;
    const decode::BuiltinType* f64Type() const;
    const decode::BuiltinType* varuintType() const;
    const decode::BuiltinType* usizeType() const;
    const decode::BuiltinType* boolType() const;
    const ValueInfoCache* cache() const;

private:
    CoreGcInterface();

    Rc<const decode::BuiltinType> _u8Type;
    Rc<const decode::BuiltinType> _u16Type;
    Rc<const decode::BuiltinType> _u64Type;
    Rc<const decode::BuiltinType> _f64Type;
    Rc<const decode::BuiltinType> _varuintType;
    Rc<const decode::BuiltinType> _usizeType;
    Rc<const decode::BuiltinType> _boolType;
    Rc<const ValueInfoCache> _cache;
};

class WaypointGcInterface : public RefCountable {
public:
    ~WaypointGcInterface();

    static GcInterfaceResult<WaypointGcInterface> create(const decode::Device* dev, const CoreGcInterface* coreIface);

    bool encodeBeginRouteCmd(std::uintmax_t id, std::uintmax_t size, Encoder* dest) const;
    bool encodeEndRouteCmd(std::uintmax_t id, Encoder* dest) const;
    bool encodeClearRouteCmd(std::uintmax_t id, Encoder* dest) const;
    bool encodeSetRoutePointCmd(std::uintmax_t id, std::uintmax_t pointIndex, const Waypoint& wp, Encoder* dest) const;
    bool encodeSetActiveRouteCmd(bmcl::Option<uintmax_t> id, Encoder* dest) const;
    bool encodeSetRouteActivePointCmd(uintmax_t id, bmcl::Option<uintmax_t> index, Encoder* dest) const;
    bool encodeSetRouteInvertedCmd(uintmax_t id, bool flag, Encoder* dest) const;
    bool encodeSetRouteClosedCmd(uintmax_t id, bool flag, Encoder* dest) const;
    bool encodeGetRouteInfoCmd(uintmax_t id, Encoder* dest) const;
    bool encodeGetRoutePointCmd(uintmax_t id, uintmax_t pointIndex, Encoder* dest) const;
    bool encodeGetRoutesInfoCmd(Encoder* dest) const;

    bool decodeGetRoutesInfoResponse(Decoder* src, AllRoutesInfo* dest) const;
    bool decodeGetRouteInfoResponse(Decoder* src, RouteInfo* dest) const;
    bool decodeGetRoutePointResponse(Decoder* src, Waypoint* dest) const;

private:
    WaypointGcInterface(const decode::Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();

    bool beginNavCmd(const decode::Command* func, Encoder* dest) const;

    Rc<const decode::Device> _dev;
    Rc<const CoreGcInterface> _coreIface;
    Rc<const decode::Ast> _navModule;
    Rc<const decode::Component> _navComponent;
    Rc<const ValueInfoCache> _cache;
    Rc<const decode::StructType> _latLonStruct;
    Rc<const decode::StructType> _posStruct;
    Rc<const decode::StructType> _vec3Struct;
    Rc<const decode::StructType> _formationEntryStruct;
    Rc<const decode::StructType> _waypointStruct;
    Rc<const decode::StructType> _routeInfoStruct;
    Rc<const decode::StructType> _allRoutesInfoStruct;
    Rc<const decode::VariantType> _actionVariant;
    Rc<const decode::VariantType> _optionalRouteIdStruct;
    Rc<const decode::VariantType> _optionalIndexStruct;
    Rc<const decode::VariantType> _optionalF64Struct;
    Rc<const decode::Command> _beginRouteCmd;
    Rc<const decode::Command> _clearRouteCmd;
    Rc<const decode::Command> _endRouteCmd;
    Rc<const decode::Command> _setRoutePointCmd;
    Rc<const decode::Command> _setRouteClosedCmd;
    Rc<const decode::Command> _setRouteInvertedCmd;
    Rc<const decode::Command> _setActiveRouteCmd;
    Rc<const decode::Command> _setRouteActivePointCmd;
    Rc<const decode::Command> _getRoutesInfoCmd;
    Rc<const decode::Command> _getRouteInfoCmd;
    Rc<const decode::Command> _getRoutePointCmd;
    std::size_t _formationArrayMaxSize;
    std::size_t _allRoutesInfoMaxSize;
};

class FileGcInterface : public RefCountable {
public:
    ~FileGcInterface();

    static GcInterfaceResult<FileGcInterface> create(const decode::Device* dev, const CoreGcInterface* coreIface);

    bool encodeBeginFile(uintmax_t id, uintmax_t size, Encoder* dest) const;
    bool encodeWriteFile(uintmax_t id, uintmax_t offset, bmcl::Bytes data, Encoder* dest) const;
    bool encodeEndFile(uintmax_t id, uintmax_t size, Encoder* dest) const;

    std::uintmax_t maxFileChunkSize() const;

private:
    FileGcInterface(const decode::Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();
    bool beginCmd(const decode::Command* func, Encoder* dest) const;

    Rc<const decode::Device> _dev;
    Rc<const CoreGcInterface> _coreIface;
    Rc<const decode::Ast> _flModule;
    Rc<const decode::Component> _flComponent;
    Rc<const decode::Command> _beginFileCmd;
    Rc<const decode::Command> _writeFileCmd;
    Rc<const decode::Command> _endFileCmd;
    std::uintmax_t _maxChunkSize;
};

class UdpGcInterface : public RefCountable {
public:
    ~UdpGcInterface();

    static GcInterfaceResult<UdpGcInterface> create(const decode::Device* dev, const CoreGcInterface* coreIface);

    bool encodeAddClient(uintmax_t id, bmcl::SocketAddressV4 address, Encoder* dest) const;

private:
    UdpGcInterface(const decode::Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();
    bool beginCmd(const decode::Command* func, Encoder* dest) const;

    Rc<const decode::Device> _dev;
    Rc<const decode::Component> _comp;
    Rc<const decode::Ast> _udpModule;
    Rc<const decode::Command> _addClientCmd;
    Rc<const decode::StructType> _ipAddressStruct;
    Rc<const CoreGcInterface> _coreIface;
};

class AllGcInterfaces : public RefCountable {
public:
    AllGcInterfaces(const decode::Device* dev, const ValueInfoCache* cache);
    ~AllGcInterfaces();

    bmcl::OptionPtr<const CoreGcInterface> coreInterface() const;
    bmcl::OptionPtr<const WaypointGcInterface> waypointInterface() const;
    bmcl::OptionPtr<const FileGcInterface> fileInterface() const;
    bmcl::OptionPtr<const UdpGcInterface> udpInterface() const;
    const std::string& errors() const;

private:
    Rc<CoreGcInterface> _coreIface;
    Rc<WaypointGcInterface> _waypointIface;
    Rc<FileGcInterface> _fileIface;
    Rc<UdpGcInterface> _udpIface;
    std::string _errors;
};
}
