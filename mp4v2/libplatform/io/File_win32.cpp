#include "src/impl.h"
#include "libplatform/impl.h" /* for platform_win32_impl.h which declares Utf8ToFilename */
#include <windows.h>

namespace mp4v2 {
    using namespace impl;
}

/**
 * Set this to 1 to compile in extra debugging
 */
#define EXTRA_DEBUG 0

/**
 * @def LOG_PRINTF
 *
 * call log.printf if EXTRA_DEBUG is defined to 1.  Do
 * nothing otherwise
 */
#if EXTRA_DEBUG
#define LOG_PRINTF(X) log.printf X
#else
#define LOG_PRINTF(X)
#endif

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

    /**
     * The UTF-8 encoded file name
     */
    std::string _name;
};

///////////////////////////////////////////////////////////////////////////////

StandardFileProvider::StandardFileProvider()
    : _handle( INVALID_HANDLE_VALUE )
{
}

/**
 * Open a file
 *
 * @param name the name of a file to open
 * @param mode the mode to open @p name
 *
 * @retval false successfully opened @p name
 * @retval true error opening @p name
 */
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

    win32::Utf8ToFilename filename(name);

    if (!filename.IsUTF16Valid())
    {
        // The logging is done
        return true;
    }

    ASSERT(LPCWSTR(filename));
    _handle = CreateFileW( filename, access, share, NULL, crdisp, flags, NULL );
    if (_handle == INVALID_HANDLE_VALUE)
    {
        log.errorf("%s: CreateFileW(%s) failed (%d)",__FUNCTION__,filename.utf8.c_str(),GetLastError());
        return true;
    }

    /*
    ** Make a copy of the name for future log messages, etc.
    */
    log.verbose2f("%s: CreateFileW(%s) succeeded",__FUNCTION__,filename.utf8.c_str());

    _name = filename.utf8;
    return false;
}

/**
 * Seek to an offset in the file
 *
 * @param pos the offset from the beginning of the file to
 * seek to
 *
 * @retval false successfully seeked to @p pos
 * @retval true error seeking to @p pos
 */
bool
StandardFileProvider::seek( Size pos )
{
    LARGE_INTEGER n;

    ASSERT(_handle != INVALID_HANDLE_VALUE);

    n.QuadPart = pos;
    if (!SetFilePointerEx( _handle, n, NULL, FILE_BEGIN ))
    {
        log.errorf("%s: SetFilePointerEx(%s,%" PRId64 ") failed (%d)",__FUNCTION__,_name.c_str(),
                                pos,GetLastError());
        return true;
    }

    return false;
}

/**
 * Read from the file
 *
 * @param buffer populated with at most @p size bytes from
 * the file
 *
 * @param size the maximum number of bytes to read
 *
 * @param nin the 
 *
 * @retval false successfully read from the file
 * @retval true error reading from the file
 */
bool
StandardFileProvider::read( void* buffer, Size size, Size& nin, Size maxChunkSize )
{
    DWORD nread = 0;

    ASSERT(_handle != INVALID_HANDLE_VALUE);

    // ReadFile takes a DWORD for number of bytes to read so
    // make sure we're not asking for more than fits.
    // MAXDWORD from WinNT.h.
    ASSERT(size <= MAXDWORD);
    if( ReadFile( _handle, buffer, (DWORD)(size & MAXDWORD), &nread, NULL ) == 0 )
    {
        log.errorf("%s: ReadFile(%s,%d) failed (%d)",__FUNCTION__,_name.c_str(),
                   (DWORD)(size & MAXDWORD),GetLastError());
        return true;
    }
    LOG_PRINTF((MP4_LOG_VERBOSE3,"%s: ReadFile(%s,%d) succeeded: read %d byte(s)",__FUNCTION__,
               _name.c_str(),(DWORD)(size & MAXDWORD),nread));
    nin = nread;
    return false;
}

/**
 * Write to the file
 *
 * @param buffer the data to write
 *
 * @param size the number of bytes of @p buffer to write
 *
 * @param nout populated with the number of bytes actually
 * written if the function succeeds
 *
 * @retval false successfully wrote to the file
 * @retval true error writing to the file
 */
bool
StandardFileProvider::write( const void* buffer, Size size, Size& nout, Size maxChunkSize )
{
    DWORD nwrote = 0;

    ASSERT(_handle != INVALID_HANDLE_VALUE);

    // ReadFile takes a DWORD for number of bytes to read so
    // make sure we're not asking for more than fits.
    // MAXDWORD from WinNT.h.
    ASSERT(size <= MAXDWORD);
    if( WriteFile( _handle, buffer, (DWORD)(size & MAXDWORD), &nwrote, NULL ) == 0 )
    {
        log.errorf("%s: WriteFile(%s,%d) failed (%d)",__FUNCTION__,_name.c_str(),
                   (DWORD)(size & MAXDWORD),GetLastError());
        return true;
    }
    log.verbose2f("%s: WriteFile(%s,%d) succeeded: wrote %d byte(s)",__FUNCTION__,
                  _name.c_str(),(DWORD)(size & MAXDWORD),nwrote);
    nout = nwrote;
    return false;
}

/**
 * Close the file
 *
 * @retval false successfully closed the file
 * @retval true error closing the file
 */
bool
StandardFileProvider::close()
{
    BOOL retval;

    retval = CloseHandle( _handle );
    if (!retval)
    {
        log.errorf("%s: CloseHandle(%s) failed (%d)",__FUNCTION__,
                   _name.c_str(),GetLastError());
    }

    // Whether we succeeded or not, clear the handle and
    // forget the name
    _handle = INVALID_HANDLE_VALUE;
    _name.clear();

    // CloseHandle return 0/false to indicate failure, but
    // we return 0/false to indicate success, so negate.
    return !retval;
}

///////////////////////////////////////////////////////////////////////////////

FileProvider&
FileProvider::standard()
{
    return *new StandardFileProvider();
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
