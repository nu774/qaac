#ifndef MP4V2_PLATFORM_WARNING_H
#define MP4V2_PLATFORM_WARNING_H

///////////////////////////////////////////////////////////////////////////////

// TODO-KB: clean code to avoid disabling warnings

#if defined( __GNUC__ ) && ( __GNUC__ >= 4 ) && ( __GNUC_MINOR__ >= 2 )
#   pragma GCC diagnostic ignored "-Wwrite-strings"
#elif defined( _MSC_VER )
#   pragma warning( disable: 4244 )
#   pragma warning( disable: 4251 )
#   pragma warning( disable: 4800 )
#   pragma warning( disable: 4996 )
#endif

///////////////////////////////////////////////////////////////////////////////

// this macro is used to mark printf-style functions for GCC to examine
// the format string and arguments and issue warnings if needed

#if defined( __GNUC__ )
#   define MP4V2_WFORMAT_PRINTF(i,j)  __attribute__((format(__printf__,i,j)))
#else
#   define MP4V2_WFORMAT_PRINTF(i,j)
#endif

///////////////////////////////////////////////////////////////////////////////

#endif // MP4V2_PLATFORM_WARNING_H
