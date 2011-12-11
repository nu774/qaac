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

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__i386__) || defined(__x86_64__)
#define TARGET_RT_LITTLE_ENDIAN 1
#endif

#ifdef _MSC_VER
#define Swap16NtoB _byteswap_ushort
#define Swap16BtoN _byteswap_ushort
#define Swap32NtoB _byteswap_ulong
#define Swap32BtoN _byteswap_ulong
#elif TARGET_RT_LITTLE_ENDIAN
inline uint16_t Swap16NtoB(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
inline uint16_t Swap16BtoN(uint16_t x)
{
    return (x << 8) | (x >> 8);
}
inline uint32_t Swap32NtoB(uint32_t x)
{
    return (Swap16NtoB(x) << 16) | Swap16NtoB(x >> 16);
}
inline uint32_t Swap32BtoN(uint32_t x)
{
    return (Swap16BtoN(x) << 16) | Swap16BtoN(x >> 16);
}
#else
inline uint16_t Swap16NtoB(uint16_t x) { return x; }
inline uint16_t Swap16BtoN(uint16_t x) { return x; }
inline uint32_t Swap32NtoB(uint32_t x) { return x; }
inline uint32_t Swap32BtoN(uint32_t x) { return x; }
#endif

#ifdef __cplusplus
}
#endif

#endif
