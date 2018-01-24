#ifndef __PHOTON_ENDIAN_H__
#define __PHOTON_ENDIAN_H__

#include "photongen/onboard/Config.h"

#include <stdint.h>

/*
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#if defined (__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define PHOTON_LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define PHOTON_BIG_ENDIAN
# else
#  error Unsupported machine endianness detected.
# endif
# define PHOTON_BYTE_ORDER __BYTE_ORDER
#elif defined(_BIG_ENDIAN)
# define PHOTON_BIG_ENDIAN
# define PHOTON_BYTE_ORDER 4321
#elif defined(_LITTLE_ENDIAN)
# define PHOTON_LITTLE_ENDIAN
# define PHOTON_BYTE_ORDER 1234
#elif defined(__sparc) || defined(__sparc__) \
   || defined(_POWER) || defined(__powerpc__) \
   || defined(__ppc__) || defined(__hpux) \
   || defined(_MIPSEB) || defined(_POWER) \
   || defined(__s390__)
# define PHOTON_BIG_ENDIAN
# define PHOTON_BYTE_ORDER 4321
#elif defined(__i386__) || defined(__alpha__) \
   || defined(__ia64) || defined(__ia64__) \
   || defined(_M_IX86) || defined(_M_IA64) \
   || defined(_M_ALPHA) || defined(__amd64) \
   || defined(__amd64__) || defined(_M_AMD64) \
   || defined(__x86_64) || defined(__x86_64__) \
   || defined(_M_X64)
# define PHOTON_LITTLE_ENDIAN
# define PHOTON_BYTE_ORDER 1234
#elif defined(__IAR_SYSTEMS_ICC__)
# if __LITTLE_ENDIAN__ == 1
#  define PHOTON_LITTLE_ENDIAN
#  define PHOTON_BYTE_ORDER 1234
# else
#  define PHOTON_BIG_ENDIAN
#  define PHOTON_BYTE_ORDER 4321
# endif
#elif defined(__arm__)
# ifdef __BIG_ENDIAN
#  define PHOTON_BIG_ENDIAN
#  define PHOTON_BYTE_ORDER 4321
# else
#  define PHOTON_LITTLE_ENDIAN
#  define PHOTON_BYTE_ORDER 1234
# endif
#else
# error Unsupported system
#endif

static PHOTON_INLINE uint16_t Photon_Be16Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return ((p[0] << 8) | p[1]);
}

static PHOTON_INLINE uint32_t Photon_Be32Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]);
}

static PHOTON_INLINE uint64_t Photon_Be64Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return (((uint64_t)Photon_Be32Dec(p) << 32) | Photon_Be32Dec(p + 4));
}

static PHOTON_INLINE uint16_t Photon_Le16Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return ((p[1] << 8) | p[0]);
}

static PHOTON_INLINE uint32_t Photon_Le32Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return (((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[0]);
}

static PHOTON_INLINE uint64_t Photon_Le64Dec(const void* src)
{
    const uint8_t* p = (const uint8_t*)src;
    return (((uint64_t)Photon_Le32Dec(p + 4) << 32) | Photon_Le32Dec(p));
}

static PHOTON_INLINE void Photon_Be16Enc(void* dest, uint16_t value)
{
    uint8_t* p = (uint8_t*)dest;
    p[0] = (value >> 8) & 0xff;
    p[1] = value & 0xff;
}

static PHOTON_INLINE void Photon_Be32Enc(void* dest, uint32_t value)
{
    uint8_t* p = (uint8_t*)dest;
    p[0] = (value >> 24) & 0xff;
    p[1] = (value >> 16) & 0xff;
    p[2] = (value >> 8) & 0xff;
    p[3] = value & 0xff;
}

static PHOTON_INLINE void Photon_Be64Enc(void* dest, uint64_t value)
{
    uint8_t* p = (uint8_t*)dest;
    Photon_Be32Enc(p, value >> 32);
    Photon_Be32Enc(p + 4, value & 0xffffffff);
}

static PHOTON_INLINE void Photon_Le16Enc(void* dest, uint16_t value)
{
    uint8_t* p = (uint8_t*)dest;
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
}

static PHOTON_INLINE void Photon_Le32Enc(void* dest, uint32_t value)
{
    uint8_t* p = (uint8_t*)dest;
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
}

static PHOTON_INLINE void Photon_Le64Enc(void* dest, uint64_t value)
{
    uint8_t* p = (uint8_t*)dest;
    Photon_Le32Enc(p, value & 0xffffffff);
    Photon_Le32Enc(p + 4, value >> 32);
}

static PHOTON_INLINE uint16_t Photon_Bswap16(uint16_t value)
{
    return ((value & 0xff00) >> 8) | ((value & 0x00ff) << 8);
}

static PHOTON_INLINE uint16_t Photon_HostToBe16(uint16_t value)
{
#ifdef PHOTON_BIG_ENDIAN
    return value;
#else
    return Photon_Bswap16(value);
#endif
}

static PHOTON_INLINE uint16_t Photon_Be16ToHost(uint16_t value)
{
#ifdef PHOTON_BIG_ENDIAN
    return value;
#else
    return Photon_Bswap16(value);
#endif
}

#endif
