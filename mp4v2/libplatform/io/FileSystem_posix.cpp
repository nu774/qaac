#include "libplatform/impl.h"
#include <sys/stat.h>

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::exists( const std::string& path_ )
{
    struct stat buf;
    return stat( path_.c_str(), &buf ) == 0;
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isDirectory( const std::string& path_ )
{
    struct stat buf;
    if( stat( path_.c_str(), &buf ))
        return false;
    return S_ISDIR( buf.st_mode );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isFile( const std::string& path_ )
{
    struct stat buf;
    if( stat( path_.c_str(), &buf ))
        return false;
    return S_ISREG( buf.st_mode );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::getFileSize( const std::string& path_, File::Size& size_ )
{
    size_ = 0;
    struct stat buf;
    if( stat( path_.c_str(), &buf ))
        return true;
    size_ = buf.st_size;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::rename( const std::string& from, const std::string& to )
{
    return ::rename( from.c_str(), to.c_str() ) != 0;
}

///////////////////////////////////////////////////////////////////////////////

string FileSystem::DIR_SEPARATOR  = "/";
string FileSystem::PATH_SEPARATOR = ":";

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
