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
    bool         _seekg;
    bool         _seekp;
    std::fstream _fstream;
};

///////////////////////////////////////////////////////////////////////////////

StandardFileProvider::StandardFileProvider()
    : _seekg ( false )
    , _seekp ( false )
{
}

bool
StandardFileProvider::open( std::string name, Mode mode )
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
StandardFileProvider::read( void* buffer, Size size, Size& nin, Size maxChunkSize )
{
    _fstream.read( (char*)buffer, size );
    if( _fstream.fail() )
        return true;
    nin = _fstream.gcount();
    return false;
}

bool
StandardFileProvider::write( const void* buffer, Size size, Size& nout, Size maxChunkSize )
{
    _fstream.write( (const char*)buffer, size );
    if( _fstream.fail() )
        return true;
    nout = size;
    return false;
}

bool
StandardFileProvider::close()
{
    _fstream.close();
    return _fstream.fail();
}

///////////////////////////////////////////////////////////////////////////////

FileProvider&
FileProvider::standard()
{
    return *new StandardFileProvider();
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
