#include "photon/nav/Nav.Component.h"

#ifdef PHOTON_STUB

bool isUploading;
static size_t routeSize;
#define ROUTE_SIZE 100
static PhotonNavWaypoint tmpRoute[ROUTE_SIZE];

#endif

void PhotonNav_Init()
{
#ifdef PHOTON_STUB
    isUploading = false;
    routeSize = 0;
    _photonNav.latLon.latitude = 44.499096;
    _photonNav.latLon.longitude = 33.966287;
    _photonNav.orientation.heading = 30;
    _photonNav.orientation.pitch = 5;
    _photonNav.orientation.roll = 2;
#else
    _photonNav.latLon.latitude = 0;
    _photonNav.latLon.longitude = 0;
    _photonNav.orientation.heading = 0;
    _photonNav.orientation.pitch = 0;
    _photonNav.orientation.roll = 0;
    _photonNav.relativeAltitude.type = PhotonNavRelativeAltitudeType_None;
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

    if(_photonNav.latLon.latitude > 90.0)
    {
        _photonNav.latLon.latitude = -90.0;
    }
    if(_photonNav.latLon.longitude > 180.0)
    {
        _photonNav.latLon.longitude = -180.0;
    }
#endif
}

#ifdef PHOTON_STUB

PhotonError PhotonNav_BeginRoute(size_t routeIndex)
{
    if (routeIndex != 0) {
        return PhotonError_InvalidValue;
    }
    isUploading = true;
    return PhotonError_Ok;
}

PhotonError PhotonNav_SetRoutePoint(size_t routeIndex, size_t pointIndex, const PhotonNavWaypoint* waypoint)
{
    if (routeIndex != 0) {
        return PhotonError_InvalidValue;
    }
    if (pointIndex >= ROUTE_SIZE) {
        return PhotonError_InvalidValue;
    }
    tmpRoute[pointIndex] = *waypoint;
    return PhotonError_Ok;
}

PhotonError PhotonNav_EndRoute(size_t routeIndex)
{
    if (!isUploading) {
        return PhotonError_InvalidValue;
    }
    if (routeIndex != 0) {
        return PhotonError_InvalidValue;
    }
    isUploading = false;
    return PhotonError_Ok;
}

#endif
