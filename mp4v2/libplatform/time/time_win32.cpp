#include "libplatform/impl.h"
#include <sys/timeb.h>

namespace mp4v2 { namespace platform { namespace time {

///////////////////////////////////////////////////////////////////////////////

milliseconds_t
getLocalTimeMilliseconds()
{
    __timeb64 buf;
    _ftime64( &buf );
    return milliseconds_t( buf.time ) * 1000 + buf.millitm;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::time
