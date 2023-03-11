/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MP4v2.
 *
 * The Initial Developer of the Original Code is Kona Blend.
 * Portions created by Kona Blend are Copyright (C) 2008.
 * All Rights Reserved.
 *
 * Contributor(s):
 *      Kona Blend, kona8lend@gmail.com
 */
#ifndef MP4V2_PLATFORM_H
#define MP4V2_PLATFORM_H

/*****************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#if defined( _WIN32 ) || defined( __MINGW32__ )
#   if defined( MP4V2_EXPORTS )
#       define MP4V2_EXPORT __declspec(dllexport)
#   elif defined( MP4V2_USE_DLL_IMPORT ) || !defined( MP4V2_USE_STATIC_LIB )
#       define MP4V2_EXPORT __declspec(dllimport)
#   else
#       define MP4V2_EXPORT
#   endif
#else
#   define MP4V2_EXPORT __attribute__((visibility("default")))
#endif

#if defined( __GNUC__ )
#   define MP4V2_DEPRECATED __attribute__((deprecated))
#else
#   define MP4V2_DEPRECATED
#endif

/******************************************************************************
 *
 * TODO-KB: cleanup -- absolutely no need for a C-API to fuss with reserved
 * C++ keywords. This will involve changing the public interface and current
 * plan of action:
 *
 *      typdef enum {
 *          mp4_false,
 *          mp4_true,
 *      } mp4_bool_t;
 *
 * followed by updating all public signatures and implementation.
 */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#if !defined( __cplusplus )
#ifndef bool
#if SIZEOF_BOOL == 8
typedef uint64_t bool;
#else
#if SIZEOF_BOOL == 4
typedef uint32_t bool;
#else
#if SIZEOF_BOOL == 2
typedef uint16_t bool;
#else
typedef unsigned char bool;
#endif
#endif
#endif
#ifndef false
#define false FALSE
#endif
#ifndef true
#define true TRUE
#endif
#endif
#endif

/*****************************************************************************/

#endif /* MP4V2_PLATFORM_H */
