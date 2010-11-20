#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::exists( string path_ )
{
    return ( ::GetFileAttributesW( win32::Utf8ToWideChar( path_ ) ) != INVALID_FILE_ATTRIBUTES );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isDirectory( string path_ )
{
	DWORD attributes = ::GetFileAttributesW( win32::Utf8ToWideChar( path_ ) );
    if( attributes == INVALID_FILE_ATTRIBUTES )
        return false;

    return ( ( attributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isFile( string path_ )
{
    DWORD attributes = ::GetFileAttributesW( win32::Utf8ToWideChar( path_ ) );
    if( attributes == INVALID_FILE_ATTRIBUTES )
        return false;

    return ( ( attributes & FILE_ATTRIBUTE_DIRECTORY ) != FILE_ATTRIBUTE_DIRECTORY );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::getFileSize( string path_, File::Size& size_ )
{
    size_ = 0;
    WIN32_FILE_ATTRIBUTE_DATA data = {0};
    if( !GetFileAttributesExW( win32::Utf8ToWideChar( path_ ), GetFileExInfoStandard, (LPVOID)&data ) )
        return true;
    size_ = ( (File::Size)data.nFileSizeHigh << 32 ) | data.nFileSizeLow;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::rename( string from, string to )
{
    return ::MoveFileExW(
        win32::Utf8ToWideChar( from ),
        win32::Utf8ToWideChar( to ),
        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) == FALSE;
}

///////////////////////////////////////////////////////////////////////////////

string FileSystem::DIR_SEPARATOR  = "\\";
string FileSystem::PATH_SEPARATOR = ";";

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
