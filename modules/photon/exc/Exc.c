#ifdef PHOTON_HAS_MODULE_PVU
#include "photongen/onboard/pvu/Pvu.Component.h"
#endif
#ifdef PHOTON_HAS_MODULE_TM
#include "photongen/onboard/tm/Tm.Component.h"
#endif
#ifdef PHOTON_HAS_MODULE_FWT
#include "photongen/onboard/fwt/Fwt.Component.h"
#endif

#include "photon/core/Logging.h"

#include <string.h>

#define _PHOTON_FNAME "exc/Exc.c"
#define PHOTON_PACKET_QUEUE_SIZE (sizeof(_photonExc.packetQueue) / sizeof(_photonExc.packetQueue[0]))

void PhotonExc_Init()
{
#ifdef PHOTON_HAS_MODULE_PVU
    PhotonPvu_Init();
#endif
#ifdef PHOTON_HAS_MODULE_TM
    PhotonTm_Init();
#endif
#ifdef PHOTON_HAS_MODULE_FWT
    PhotonFwt_Init();
#endif
    _photonExc.address = 2;
    _photonExc.slaveAddress = PHOTON_DEVICE_ID;
}

void PhotonExc_Tick()
{
}

uint64_t PhotonExc_SelfAddress()
{
    return _photonExc.address;
}

uint64_t PhotonExc_SelfSlaveAddress()
{
    return _photonExc.slaveAddress;
}

PhotonError PhotonExc_ExecCmd_SetAddress(uint64_t address)
{
    _photonExc.address = address;
    return PhotonError_Ok;
}

void PhotonExc_SetAddress(uint64_t address)
{
    _photonExc.address = address;
}

PhotonExcClientError PhotonExc_RegisterGroundControl(uint64_t id, PhotonExcDevice** device)
{
    if (_photonExc.devices.size == 16) {
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
    if (_photonExc.devices.size == 16) {
        return PhotonExcClientError_NoDescriptorsLeft;
    }
    PhotonExcDevice* d = &_photonExc.devices.data[_photonExc.devices.size];
    PhotonExcDevice_InitUav(d, id);
    _photonExc.devices.size++;
    *device = d;
    return PhotonExcClientError_Ok;
}

PhotonExcClientError PhotonExc_RegisterSlave(uint64_t id, PhotonExcDevice** device, PhotonExcTmHandler handler, void* data)
{
    if (_photonExc.devices.size == 16) {
        return PhotonExcClientError_NoDescriptorsLeft;
    }
    PhotonExcDevice* d = &_photonExc.devices.data[_photonExc.devices.size];
    PhotonExcDevice_InitSlave(d, id, handler, data);
    _photonExc.devices.size++;
    *device = d;
    return PhotonExcClientError_Ok;
}

PhotonExcClientError PhotonExc_RemoveClient(uint64_t id)
{
    (void)id;
    return PhotonExcClientError_Ok;
}

#undef _PHOTON_FNAME
