#ifndef __PHOTON_CORE_CRC_H__
#define __PHOTON_CORE_CRC_H__

#include "photongen/onboard/Config.h"

#include <stddef.h>
#include <stdint.h>

uint16_t Photon_Crc16(const void* src, size_t len);

#endif
