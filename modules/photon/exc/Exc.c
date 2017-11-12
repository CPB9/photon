#include "photon/exc/Exc.Component.h"
#include "photon/pvu/Pvu.Component.h"
#include "photon/fwt/Fwt.Component.h"
#include "photon/tm/Tm.Component.h"

#include "photon/core/Logging.h"

#include <string.h>

#define _PHOTON_FNAME "exc/Exc.c"
#define PHOTON_PACKET_QUEUE_SIZE (sizeof(_photonExc.packetQueue) / sizeof(_photonExc.packetQueue[0]))

void PhotonExc_Init()
{
    PhotonPvu_Init();
    PhotonTm_Init();
    PhotonFwt_Init();
    _photonExc.address = 2;
}

void PhotonExc_Tick()
{
}

uint64_t PhotonExc_SelfAddress()
{
    return _photonExc.address;
}

PhotonError PhotonExc_SetAddress(uint64_t address)
{
    _photonExc.address = address;
    return PhotonError_Ok;
}

PhotonExcClientError PhotonExc_RegisterGroundControl(uint64_t id, PhotonExcDevice** device)
{
    if (_photonExc.devices.size == 8) {
        return PhotonExcClientError_NoDescriptorsLeft;
    }
    PhotonExcDevice* d = &_photonExc.devices.data[_photonExc.devices.size];
    PhotonExcDevice_InitGroundControl(d, id);
    _photonExc.devices.size++;
    *device = d;
    return PhotonExcClientError_Ok;
}

PhotonExcClientError PhotonExc_RegisterUav(uint64_t id, PhotonExcDevice** device)
{
    if (_photonExc.devices.size == 8) {
        return PhotonExcClientError_NoDescriptorsLeft;
    }
    PhotonExcDevice* d = &_photonExc.devices.data[_photonExc.devices.size];
    PhotonExcDevice_InitUav(d, id);
    _photonExc.devices.size++;
    *device = d;
    return PhotonExcClientError_Ok;
}

PhotonExcClientError PhotonExc_RemoveClient(uint64_t id)
{
    return PhotonExcClientError_Ok;
}

#undef _PHOTON_FNAME
