#ifndef MP4V2_PLATFORM_SYS_ERROR_H
#define MP4V2_PLATFORM_SYS_ERROR_H

namespace mp4v2 { namespace platform { namespace sys {

///////////////////////////////////////////////////////////////////////////////

             int         getLastError    ( );
MP4V2_EXPORT const char* getLastErrorStr ( );
             const char* getErrorStr     ( int );

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::sys

#endif // MP4V2_PLATFORM_SYS_ERROR_H
