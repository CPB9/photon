#include "photon/nav/Nav.Component.h"
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
    _photonNav.relativeAltitude.type = PhotonNavRelativeAltitudeType_None;
#ifdef PHOTON_STUB
    info.isEditing = false;
    info.id = 0;
    info.size = 0;
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
    _photonNav.latLon.latitude += 0.01;
    _photonNav.latLon.longitude += 0.01;
    _photonNav.orientation.heading += 0.1;
    _photonNav.orientation.pitch += 0.1;
    _photonNav.orientation.roll += 0.1;

    if(_photonNav.latLon.latitude > 90.0) {
        _photonNav.latLon.latitude = -90.0;
    }
    if(_photonNav.latLon.longitude > 180.0) {
        _photonNav.latLon.longitude = -180.0;
    }
#endif
}

#ifdef PHOTON_STUB

PhotonError PhotonNav_GetRoutesInfo(PhotonNavAllRoutesInfo* rv)
{
    if (isActive) {
        rv->activeRoute.type = PhotonNavOptionalRouteIdType_Some;
        rv->activeRoute.data.someOptionalRouteId.id = 0;
    } else {
        rv->activeRoute.type = PhotonNavOptionalRouteIdType_None;
    }
    rv->info.size = 1;
    rv->info.data[0] = info;

    return PhotonError_Ok;
}

PhotonError PhotonNav_SetActiveRoute(const PhotonNavOptionalRouteId* routeId)
{
    if (routeId->type == PhotonNavOptionalRouteIdType_None) {
        isActive = false;
        return PhotonError_Ok;
    }
    if (routeId->data.someOptionalRouteId.id != 0) {
        return PhotonError_InvalidValue;
    }
    isActive = true;
    return PhotonError_Ok;
}

PhotonError PhotonNav_SetRouteActivePoint(uint64_t routeId, const PhotonNavOptionalIndex* pointIndex)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    info.activePoint = *pointIndex;
    return PhotonError_Ok;
}

PhotonError PhotonNav_BeginRoute(uint64_t routeId)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    info.isEditing = true;
    return PhotonError_Ok;
}

PhotonError PhotonNav_ClearRoute(uint64_t routeId)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    if (!info.isEditing) {
        return PhotonError_InvalidValue;
    }
    info.size = 0;
    return PhotonError_Ok;
}

PhotonError PhotonNav_SetRoutePoint(uint64_t routeId, uint64_t pointIndex, const PhotonNavWaypoint* waypoint)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    if (pointIndex >= ROUTE_SIZE) {
        return PhotonError_InvalidValue;
    }
    tmpRoute[pointIndex] = *waypoint;
    return PhotonError_Ok;
}

PhotonError PhotonNav_EndRoute(uint64_t routeId)
{
    if (!info.isEditing) {
        return PhotonError_InvalidValue;
    }
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    info.isEditing = false;
    return PhotonError_Ok;
}

PhotonError PhotonNav_SetRouteInverted(uint64_t routeId, bool isInverted)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    info.isInverted = isInverted;
    return PhotonError_Ok;
}

PhotonError PhotonNav_SetRouteClosed(uint64_t routeId, bool isClosed)
{
    if (routeId != 0) {
        return PhotonError_InvalidValue;
    }
    info.isClosed = isClosed;
    return PhotonError_Ok;
}

#endif

#undef _PHOTON_FNAME
