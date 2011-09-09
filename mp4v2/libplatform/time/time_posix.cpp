#include "libplatform/impl.h"
#include <sys/time.h>

namespace mp4v2 { namespace platform { namespace time {

///////////////////////////////////////////////////////////////////////////////

milliseconds_t
getLocalTimeMilliseconds()
{
    timeval buf;
    if( gettimeofday( &buf, 0 ))
        memset( &buf, 0, sizeof( buf ));
    return milliseconds_t( buf.tv_sec ) * 1000 + buf.tv_usec / 1000;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::time
