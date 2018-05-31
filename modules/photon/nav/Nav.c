#include "photongen/onboard/nav/Nav.Component.h"
#include "photon/core/Logging.h"

#define _PHOTON_FNAME "nav/Nav.c"

#ifdef PHOTON_STUB

PhotonNavRouteInfo info;
bool isActive;
#define ROUTE_SIZE 100
static PhotonNavWaypoint tmpRoute[ROUTE_SIZE];

#endif

void PhotonNav_Init()
{
    _photonNav.relativeAltitude.type = PhotonNavOptionalF64Type_None;
#ifdef PHOTON_STUB
    info.isEditing = false;
    info.id = 0;
    info.size = 1;
    tmpRoute[0].action.type = PhotonNavActionType_None;
    tmpRoute[0].position.latLon.latitude = 1;
    tmpRoute[0].position.latLon.longitude = 2;
    tmpRoute[0].position.altitude = 3;
    info.isClosed = false;
    info.isInverted = false;
    info.maxSize = ROUTE_SIZE;
    info.activePoint.type = PhotonNavOptionalIndexType_None;
    isActive = false;
    _photonNav.latLon.latitude = 44.499096;
    _photonNav.latLon.longitude = 33.966287;
    _photonNav.orientation.heading = 30;
    _photonNav.orientation.pitch = 5;
    _photonNav.orientation.roll = 2;
    _photonNav.velocity.x = 10;
    _photonNav.velocity.y = 10;
    _photonNav.velocity.z = 10;
#else
    _photonNav.latLon.latitude = 0;
    _photonNav.latLon.longitude = 0;
    _photonNav.orientation.heading = 0;
    _photonNav.orientation.pitch = 0;
    _photonNav.orientation.roll = 0;
    _photonNav.velocity.x = 0;
    _photonNav.velocity.y = 0;
    _photonNav.velocity.z = 0;
#endif
}

void PhotonNav_Tick()
{
#ifdef PHOTON_STUB
    if (_photonNav.relativeAltitude.type == PhotonNavOptionalF64Type_None) {
        _photonNav.relativeAltitude.type = PhotonNavOptionalF64Type_Some;
    } else {
        _photonNav.relativeAltitude.type = PhotonNavOptionalF64Type_None;
    }
    _photonNav.latLon.latitude += 0.01;
    _photonNav.latLon.longitude += 0.01;
    _photonNav.orientation.heading += 0.1;
    _photonNav.orientation.pitch += 0.1;
    _photonNav.orientation.roll += 0.1;

    if(_photonNav.latLon.latitude > 70.0) {
        _photonNav.latLon.latitude = -70.0;
    }
    if(_photonNav.latLon.longitude > 180.0) {
        _photonNav.latLon.longitude = -180.0;
    }
#endif
}

#ifdef PHOTON_STUB

PhotonError PhotonNav_ExecCmd_GetRoutesInfo(PhotonNavAllRoutesInfo* rv)
{
    if (isActive) {
        rv->activeRoute.type = PhotonNavOptionalRouteIdType_Some;
        rv->activeRoute.data.someNavOptionalRouteId.id = 0;
    } else {
        rv->activeRoute.type = PhotonNavOptionalRouteIdType_None;
    }
    PHOTON_INFO("Sending route info");
    rv->info.size = 1;
    rv->info.data[0] = info;

    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_SetActiveRoute(const PhotonNavOptionalRouteId* routeId)
{
    if (routeId->type == PhotonNavOptionalRouteIdType_None) {
        isActive = false;
        return PhotonError_Ok;
    }
    if (routeId->data.someNavOptionalRouteId.id != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Setting active route");
    isActive = true;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_SetRouteActivePoint(PhotonNavRouteId routeId, const PhotonNavOptionalIndex* pointIndex)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Setting route active point");
    info.activePoint = *pointIndex;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_BeginRoute(PhotonNavRouteId routeId, uint64_t size)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    if (size > info.maxSize) {
        PHOTON_CRITICAL("Invalid route size");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Beginning route");
    info.isEditing = true;
    info.size = size;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_ClearRoute(PhotonNavRouteId routeId)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    if (!info.isEditing) {
        PHOTON_CRITICAL("Can't clear route while not editing");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Clearing route");
    info.size = 0;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_SetRoutePoint(PhotonNavRouteId routeId, uint64_t pointIndex, const PhotonNavWaypoint* waypoint)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    if (pointIndex >= info.maxSize) {
        PHOTON_CRITICAL("Invalid route size");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Setting route point");
    tmpRoute[pointIndex] = *waypoint;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_EndRoute(PhotonNavRouteId routeId)
{
    if (!info.isEditing) {
        PHOTON_CRITICAL("Can't end route while not editing");
        return PhotonError_InvalidValue;
    }
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Ending route");
    info.isEditing = false;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_SetRouteInverted(PhotonNavRouteId routeId, bool isInverted)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Setting route inverted");
    info.isInverted = isInverted;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_SetRouteClosed(PhotonNavRouteId routeId, bool isClosed)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Setting route closed");
    info.isClosed = isClosed;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_GetRouteInfo(PhotonNavRouteId routeId, PhotonNavRouteInfo* rv)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Getting route info");
    *rv = info;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ExecCmd_GetRoutePoint(PhotonNavRouteId routeId, uint64_t pointIndex, PhotonNavWaypoint* rv)
{
    if (routeId != 0) {
        PHOTON_CRITICAL("Invalid route id");
        return PhotonError_InvalidValue;
    }
    if (pointIndex >= info.maxSize) {
        PHOTON_CRITICAL("Invalid point index");
        return PhotonError_InvalidValue;
    }
    PHOTON_INFO("Getting route point");
    *rv = tmpRoute[pointIndex];
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
