/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

//
//  EndianPortable.h
//
//  Copyright 2011 Apple Inc. All rights reserved.
//

#ifndef _EndianPortable_h
#define _EndianPortable_h

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__i386__) || defined(__x86_64__)
#define TARGET_RT_LITTLE_ENDIAN 1
#endif

#ifndef _MSC_VER
inline uint16_t _byteswap_ushort(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
inline uint32_t _byteswap_ulong(uint32_t x)
{
    return (_byteswap_ushort(x) << 16) | _byteswap_ushort(x >> 16);
}
inline uint64_t _byteswap_uint64(uint64_t x)
{
    uint64_t hi = _byteswap_ulong(x);
    return hi << 32 | _byteswap_ulong(x >> 32);
}
#endif
inline float _byteswap_float(float x)
{
    union { float f; int32_t i; } u;
    u.f = x;
    u.i = _byteswap_ulong(u.i);
    return u.f;
}

inline double _byteswap_double(double x)
{
    union { double f; int64_t i; } u;
    u.f = x;
    u.i = _byteswap_uint64(u.i);
    return u.f;
}

inline void Swap16(uint16_t *x) { *x = _byteswap_ushort(*x); }
inline void Swap32(uint32_t *x) { *x = _byteswap_ulong(*x); }
inline void Swap24(uint8_t *x)
{
    uint8_t tmp = x[0]; x[0] = x[2]; x[2] = tmp;
}
inline uint16_t Identity16(uint16_t x) { return x; }
inline uint32_t Identity32(uint32_t x) { return x; }
inline uint64_t Ideneity64(uint64_t x) { return x; }
inline float IdentityFloat(float x) { return x; }
inline double IdentityDouble(double x) { return x; }

#if TARGET_RT_LITTLE_ENDIAN
#define Swap16NtoB _byteswap_ushort
#define Swap16BtoN _byteswap_ushort
#define Swap32NtoB _byteswap_ulong
#define Swap32BtoN _byteswap_ulong
#define Swap64NtoB _byteswap_uint64
#define Swap64BtoN _byteswap_uint64
#define SwapFloat32BtoN _byteswap_float
#define SwapFloat32NtoB _byteswap_float
#define SwapFloat64BtoN _byteswap_double
#define SwapFloat64NtoB _byteswap_double
#else
#define Swap16NtoB Identity16
#define Swap16BtoB Identity16
#define Swap32NtoB Identity32
#define Swap32BtoN Identity32
#define Swap64NtoB Identity64
#define Swap64BtoN Identity64
#define SwapFloat32BtoN IdentityFloat
#define SwapFloat32NtoB IdentityFloat
#define SwapFloat64BtoN IdentityDouble
#define SwapFloat64NtoB IdentityDouble
#endif

#ifdef __cplusplus
}
#endif

#endif
