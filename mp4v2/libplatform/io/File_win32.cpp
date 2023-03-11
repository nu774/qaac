#include "src/impl.h"
#include "libplatform/impl.h" /* for platform_win32_impl.h which declares Utf8ToFilename */

#if _WIN32_WINNT < 0x0600
#   include <io.h> // for _lseeki64 in pre Windows Vista code

#   ifndef GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
#       define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x2
#       define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS       0x4
#   endif

    typedef int (*_fseeki64_type)(FILE*, __int64, int);

    static _fseeki64_type GetFileSeekFunction()
    {
        // find the module containing file IO functions and check if it has _fseeki64
        HMODULE crtdll = NULL;

#   if _WIN32_WINNT >= 0x0501
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &_wfopen, &crtdll);
#   else
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

        typedef BOOL (WINAPI *GetModuleHandleExA_type)(DWORD, LPCSTR, HMODULE*);

        if (GetModuleHandleExA_type GetModuleHandleExA_func = (GetModuleHandleExA_type) GetProcAddress(kernel32, "GetModuleHandleExA"))
            GetModuleHandleExA_func(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &_wfopen, &crtdll);
#   endif

        if (crtdll)
            return (_fseeki64_type) GetProcAddress(crtdll, "_fseeki64");

        return NULL;
    }

    static _fseeki64_type _fseeki64_func = GetFileSeekFunction();
#endif

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

class StandardFileProvider : public FileProvider
{
public:
    StandardFileProvider();

    bool open( const std::string& name, Mode mode );
    bool seek( Size pos );
    bool read( void* buffer, Size size, Size& nin );
    bool write( const void* buffer, Size size, Size& nout );
    bool truncate( Size size );
    bool close();
    bool getSize( Size& nout );

private:
    FILE* _file;

    /**
     * Argument for FileSystem::getFileSize()
     */
    std::string _name;
};

///////////////////////////////////////////////////////////////////////////////

StandardFileProvider::StandardFileProvider()
    : _file( NULL )
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
StandardFileProvider::open( const std::string& name, Mode mode )
{
    _name = name;

    const wchar_t *om;
    switch( mode ) {
        case MODE_UNDEFINED:
        case MODE_READ:
        default:
            om = L"rbN";
            break;

        case MODE_MODIFY:
            om = L"r+bN";
            break;

        case MODE_CREATE:
            om = L"w+bN";
            break;
    }

    win32::Utf8ToFilename filename(name);

    if (!filename.IsUTF16Valid())
    {
        // The logging is done
        return true;
    }

    ASSERT(LPCWSTR(filename));

    _file = _wfopen( filename, om );

    return (_file == NULL);
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
#if _WIN32_WINNT >= 0x0600 // Windows Vista or later
    return _fseeki64( _file, pos, SEEK_SET );
#else
    if (_fseeki64_func)
        return _fseeki64_func( _file, pos, SEEK_SET );

    // if _fseeki64 is not available, cause a cache flush
    // before calling _lseeki64 on the underlying fileno
    rewind( _file );

    return _lseeki64( _fileno( _file ), pos, SEEK_SET ) == -1;
#endif
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
StandardFileProvider::read( void* buffer, Size size, Size& nin )
{
    Size count = fread( buffer, 1, size, _file );
    if( ferror(_file) )
        return true;
    nin = count;
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
StandardFileProvider::write( const void* buffer, Size size, Size& nout )
{
    Size count = fwrite( buffer, 1, size, _file );
    if( ferror(_file) )
        return true;
    nout = count;
    return false;
}

/**
 * Truncate file size
 *
 * @param size the number of bytes to truncate the file to
 *
 * @retval false successfully truncated the file
 * @retval true error truncating the file
 */
bool
StandardFileProvider::truncate( Size size )
{
    // close the file prior to truncating it
    fclose( _file );

    // truncate the file using Windows API functions
    win32::Utf8ToFilename filename(_name);
    HANDLE handle = CreateFileW( filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( handle == INVALID_HANDLE_VALUE )
        return true;

    LARGE_INTEGER end;
    end.QuadPart = size;
    if ( !SetFilePointerEx( handle, end, NULL, FILE_BEGIN ) || !SetEndOfFile( handle ) ) {
        CloseHandle( handle );
        return true;
    }

    CloseHandle( handle );

    // reopen the file and seek to the new end
    _file = _wfopen( filename, L"r+bN" );
    if ( _file == NULL )
        return true;

    return seek( size );
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
    return fclose(_file);
}

/**
 * Get the size of a the file in bytes
 *
 * @param nout populated with the size of the file in
 * bytes if the function succeeds
 *
 * @retval false successfully got the file size
 * @retval true error getting the file size
 */
bool
StandardFileProvider::getSize( Size& nout )
{
    bool retval;

    // getFileSize will log if it fails
    retval = FileSystem::getFileSize( _name, nout );

    return retval;
}

///////////////////////////////////////////////////////////////////////////////

FileProvider&
FileProvider::standard()
{
    return *new StandardFileProvider();
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
