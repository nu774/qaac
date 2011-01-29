#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

class StandardFileProvider : public FileProvider
{
public:
    StandardFileProvider();

    bool open( std::string name, Mode mode );
    bool seek( Size pos );
    bool read( void* buffer, Size size, Size& nin, Size maxChunkSize );
    bool write( const void* buffer, Size size, Size& nout, Size maxChunkSize );
    bool close();

private:
    HANDLE _handle;
};

///////////////////////////////////////////////////////////////////////////////

StandardFileProvider::StandardFileProvider()
    : _handle( INVALID_HANDLE_VALUE )
{
}

bool
StandardFileProvider::open( std::string name, Mode mode )
{
    DWORD access = 0;
    DWORD share  = 0;
    DWORD crdisp = 0;
    DWORD flags  = FILE_ATTRIBUTE_NORMAL;

    switch( mode ) {
        case MODE_UNDEFINED:
        case MODE_READ:
        default:
            access |= GENERIC_READ;
            share  |= FILE_SHARE_READ;
            crdisp |= OPEN_EXISTING;
            break;

        case MODE_MODIFY:
            access |= GENERIC_READ | GENERIC_WRITE;
            share  |= FILE_SHARE_READ;
            crdisp |= OPEN_EXISTING;
            break;

        case MODE_CREATE:
            access |= GENERIC_READ | GENERIC_WRITE;
            share  |= FILE_SHARE_READ;
            crdisp |= CREATE_ALWAYS;
            break;
    }

    _handle = CreateFileW( win32::Utf8ToWideChar( name ), access, share, NULL, crdisp, flags, NULL );
    return _handle == INVALID_HANDLE_VALUE;
}

bool
StandardFileProvider::seek( Size pos )
{
    LARGE_INTEGER n;
    n.QuadPart = pos;
    return SetFilePointerEx( _handle, n, NULL, FILE_BEGIN ) == 0;
}

bool
StandardFileProvider::read( void* buffer, Size size, Size& nin, Size maxChunkSize )
{
    DWORD nread = 0;
    if( ReadFile( _handle, buffer, (DWORD)size, &nread, NULL ) == 0 )
        return true;
    nin = nread;
    return false;
}

bool
StandardFileProvider::write( const void* buffer, Size size, Size& nout, Size maxChunkSize )
{
    DWORD nwrote = 0;
    if( WriteFile( _handle, buffer, (DWORD)size, &nwrote, NULL ) == 0 )
        return true;
    nout = nwrote;
    return false;
}

bool
StandardFileProvider::close()
{
    bool success = ( CloseHandle( _handle ) == 0 );
    _handle = INVALID_HANDLE_VALUE;
    return success;
}

///////////////////////////////////////////////////////////////////////////////

FileProvider&
FileProvider::standard()
{
    return *new StandardFileProvider();
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
