#include "libplatform/impl.h"

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
    bool         _seekg;
    bool         _seekp;
    std::fstream _fstream;
    std::string  _name;
};

///////////////////////////////////////////////////////////////////////////////

StandardFileProvider::StandardFileProvider()
    : _seekg ( false )
    , _seekp ( false )
{
}

bool
StandardFileProvider::open( const std::string& name, Mode mode )
{
    ios::openmode om = ios::binary;
    switch( mode ) {
        case MODE_UNDEFINED:
        case MODE_READ:
        default:
            om |= ios::in;
            _seekg = true;
            _seekp = false;
            break;

        case MODE_MODIFY:
            om |= ios::in | ios::out;
            _seekg = true;
            _seekp = true;
            break;

        case MODE_CREATE:
            om |= ios::in | ios::out | ios::trunc;
            _seekg = true;
            _seekp = true;
            break;
    }

    _fstream.open( name.c_str(), om );
    _name = name;
    return _fstream.fail();
}

bool
StandardFileProvider::seek( Size pos )
{
    if( _seekg )
        _fstream.seekg( pos, ios::beg );
    if( _seekp )
        _fstream.seekp( pos, ios::beg );
    return _fstream.fail();
}

bool
StandardFileProvider::read( void* buffer, Size size, Size& nin )
{
    _fstream.read( (char*)buffer, size );
    if( _fstream.fail() )
        return true;
    nin = _fstream.gcount();
    return false;
}

bool
StandardFileProvider::write( const void* buffer, Size size, Size& nout )
{
    _fstream.write( (const char*)buffer, size );
    if( _fstream.fail() )
        return true;
    nout = size;
    return false;
}

bool
StandardFileProvider::truncate( Size size )
{
    // close the file prior to truncating it
    _fstream.close();

    // truncate the file using the POSIX truncate function
    if( ::truncate( _name.c_str(), size ) != 0)
        return true;

    // reopen the file and seek to the new end
    _fstream.clear();
    _fstream.open( _name.c_str(), ios::binary | ios::in | ios::out);
    if ( _fstream.fail() )
        return true;

    return seek( size );
}

bool
StandardFileProvider::close()
{
    _fstream.close();
    return _fstream.fail();
}

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
