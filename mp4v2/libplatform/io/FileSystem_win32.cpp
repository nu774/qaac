#include "src/impl.h"
#include "libplatform/impl.h" /* for platform_win32_impl.h which declares Utf8ToFilename */
#include <windows.h>

namespace mp4v2 {
    using namespace impl;
}

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

static DWORD    getAttributes ( string   path_ );

/**
 * Call GetFileAttributesW throw exceptions for errors
 *
 * @param path_ the path to get attributes for
 *
 * @retval INVALID_FILE_ATTRIBUTES @p path_ doesn't exist
 * @retval anything else the attributes of @p path_
 */
static DWORD
getAttributes ( string  path_ )
{
    win32::Utf8ToFilename filename(path_);

    if (!filename.IsUTF16Valid())
    {
        // throw an exception to avoid changing the
        // signature of this function and dealing with all
        // the places it's called.
        ostringstream msg;
        msg << "can't convert file to UTF-16(" << filename.utf8 << ")";
        throw new Exception(msg.str(),__FILE__,__LINE__,__FUNCTION__);
    }

    DWORD attributes = ::GetFileAttributesW(filename);
    if( attributes == INVALID_FILE_ATTRIBUTES )
    {
        DWORD last_err = GetLastError();

        // Distinguish between an error and the path not existing
        if ((last_err == ERROR_FILE_NOT_FOUND) || (last_err == ERROR_PATH_NOT_FOUND))
        {
            return attributes;
        }

        // Anything else is an error
        ostringstream msg;
        msg << "GetFileAttributes(" << filename.utf8 << ") failed (" << last_err << ")";
        throw new Exception(msg.str(),__FILE__,__LINE__,__FUNCTION__);
    }

    // path exists so return its attributes
    return attributes;
}

bool
FileSystem::exists( string path_ )
{
    return( getAttributes(path_) != INVALID_FILE_ATTRIBUTES );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isDirectory( string path_ )
{
    DWORD attributes = getAttributes( path_ );
    if( attributes == INVALID_FILE_ATTRIBUTES )
        return false;

    return ( ( attributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isFile( string path_ )
{
    DWORD attributes = getAttributes( path_ );
    if( attributes == INVALID_FILE_ATTRIBUTES )
        return false;

    return ( ( attributes & FILE_ATTRIBUTE_DIRECTORY ) != FILE_ATTRIBUTE_DIRECTORY );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::getFileSize( string path_, File::Size& size_ )
{
    win32::Utf8ToFilename filename(path_);

    if (!filename.IsUTF16Valid())
    {
        // The logging is done
        return true;
    }

    size_ = 0;
    WIN32_FILE_ATTRIBUTE_DATA data = {0};
    if( !GetFileAttributesExW( filename, GetFileExInfoStandard, (LPVOID)&data ) )
    {
        log.errorf("%s: GetFileAttributesExW(%s) failed (%d)",__FUNCTION__,filename.utf8.c_str(),
                   GetLastError());
        return true;
    }

    size_ = ( (File::Size)data.nFileSizeHigh << 32 ) | data.nFileSizeLow;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::rename( string from, string to )
{
    win32::Utf8ToFilename from_file(from);
    win32::Utf8ToFilename to_file(to);

    if (!from_file.IsUTF16Valid() || !to_file.IsUTF16Valid())
    {
        return true;
    }

    if (!::MoveFileExW( from_file, to_file,
                        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
    {
        log.errorf("%s: MoveFileExW(%s,%s) failed (%d)",__FUNCTION__,from_file.utf8.c_str(),to_file.utf8.c_str(),
                   GetLastError());
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

string FileSystem::DIR_SEPARATOR  = "\\";
string FileSystem::PATH_SEPARATOR = ";";

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
