#ifndef MP4V2_PLATFORM_WIN32_H
#define MP4V2_PLATFORM_WIN32_H

///////////////////////////////////////////////////////////////////////////////

// mingw needs this to enable some newer 64-bit functions
#ifdef __MINGW32__
#   undef  __MSVCRT_VERSION__
#   define __MSVCRT_VERSION__ 0x800
#endif

// set minimum win32 API requirement to Windows 2000 or higher
#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0500
#endif 
#ifndef WINVER
#   define WINVER 0x0500
#endif 

///////////////////////////////////////////////////////////////////////////////

#include "libplatform/platform_base.h"
#include <mp4v2/mp4v2.h>

///////////////////////////////////////////////////////////////////////////////

namespace mp4v2 { namespace platform {
    using namespace std;

    using ::int8_t;
    using ::int16_t;
    using ::int32_t;
    using ::int64_t;

    using ::uint8_t;
    using ::uint16_t;
    using ::uint32_t;
    using ::uint64_t;
}} // namespace mp4v2::platform

///////////////////////////////////////////////////////////////////////////////

// From http://msdn.microsoft.com/en-us/library/aa365247%28v=vs.85%29.aspx#maxpath
#define MP4V2_PATH_MAX (32767 + 1)

///////////////////////////////////////////////////////////////////////////////

// fprintf macros for unsigned types - mingw32 is a good source if more needed
#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#define PRId64  "I64d"

#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#define PRIu64  "I64u"

#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#define PRIx64  "I64x"

///////////////////////////////////////////////////////////////////////////////

// some macros for constant expressions
#define INT8_C(x)    x
#define INT16_C(x)   x
#define INT32_C(x)   x ## L
#define INT64_C(x)   x ## LL

#define UINT8_C(x)   x
#define UINT16_C(x)  x
#define UINT32_C(x)  x ## UL
#define UINT64_C(x)  x ## ULL

///////////////////////////////////////////////////////////////////////////////

#ifdef min
#   undef min
#endif

#ifdef max
#   undef max
#endif

///////////////////////////////////////////////////////////////////////////////

#define snprintf(s,n,...)  _snprintf(s,n,__VA_ARGS__)
#define strcasecmp(s1,s2)  _stricmp(s1,s2)
#define strdup(s)          _strdup(s)

///////////////////////////////////////////////////////////////////////////////

// macro clashes with symbol
#undef LC_NONE

#endif // MP4V2_PLATFORM_WIN32_H
