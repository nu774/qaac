#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

File::File( const std::string& name_, Mode mode_, FileProvider* provider_ )
    : _name     ( name_ )
    , _isOpen   ( false )
    , _mode     ( mode_ )
    , _size     ( 0 )
    , _position ( 0 )
    , _provider ( provider_ ? *provider_ : standard() )
    , name      ( _name )
    , isOpen    ( _isOpen )
    , mode      ( _mode )
    , size      ( _size )
    , position  ( _position )
{
}

///////////////////////////////////////////////////////////////////////////////

File::~File()
{
    close();
    delete &_provider;
}

///////////////////////////////////////////////////////////////////////////////

void
File::setMode( Mode mode_ )
{
    _mode = mode_;
}

void
File::setName( const std::string& name_ )
{
    _name = name_;
}

///////////////////////////////////////////////////////////////////////////////

bool
File::open( const std::string& name_, Mode mode_ )
{
    if( _isOpen )
        return true;

    if( !name_.empty() )
        setName( name_ );
    if( mode_ != MODE_UNDEFINED )
        setMode( mode_ );

    if( _provider.open( _name, _mode ))
        return true;

    if( _provider.getSize( _size ))
        return true;

    _isOpen = true;
    return false;
}

bool
File::seek( Size pos )
{
    if( !_isOpen )
        return true;

    if( _provider.seek( pos ))
        return true;
    _position = pos;
    return false;
}

bool
File::read( void* buffer, Size size, Size& nin )
{
    nin = 0;

    if( !_isOpen )
        return true;

    if( _provider.read( buffer, size, nin ))
        return true;

    _position += nin;
    if( _position > _size )
        _size = _position;

    return false;
}

bool
File::write( const void* buffer, Size size, Size& nout )
{
    nout = 0;

    if( !_isOpen )
        return true;

    if( _provider.write( buffer, size, nout ))
        return true;

    _position += nout;
    if( _position > _size )
        _size = _position;

    return false;
}

bool
File::truncate( Size size )
{
    if( !_isOpen )
        return true;

    if( _provider.truncate( size ))
        return true;

    _size = size;

    return false;
}

bool
File::close()
{
    if( !_isOpen )
        return false;
    if( _provider.close() )
        return true;

    _isOpen = false;
    return false;
}

bool
File::getSize( Size& nout )
{
    if( !_isOpen )
        return false;

    return _provider.getSize( nout );
}

///////////////////////////////////////////////////////////////////////////////

CustomFileProvider::CustomFileProvider( const MP4FileProvider& provider )
    : _handle( NULL )
{
    memcpy( &_call, &provider, sizeof(MP4FileProvider) );
}

bool
CustomFileProvider::open( const std::string& name, Mode mode )
{
    _name = name;

    MP4FileMode fm;
    switch( mode ) {
        case MODE_READ:   fm = FILEMODE_READ;   break;
        case MODE_MODIFY: fm = FILEMODE_MODIFY; break;
        case MODE_CREATE: fm = FILEMODE_CREATE; break;

        case MODE_UNDEFINED:
        default:
            fm = FILEMODE_UNDEFINED;
            break;
    }

    _handle = _call.open( _name.c_str(), fm );
    return _handle == NULL;
}

bool
CustomFileProvider::seek( Size pos )
{
    return _call.seek( _handle, pos );
}

bool
CustomFileProvider::read( void* buffer, Size size, Size& nin )
{
    return _call.read( _handle, buffer, size, &nin, 0 );
}

bool
CustomFileProvider::write( const void* buffer, Size size, Size& nout )
{
    return _call.write( _handle, buffer, size, &nout, 0 );
}

bool
CustomFileProvider::truncate( Size size )
{
    return true;
}

bool
CustomFileProvider::close()
{
    return _call.close( _handle );
}

bool
CustomFileProvider::getSize( Size& nout )
{
    return FileSystem::getFileSize( _name, nout );
}

///////////////////////////////////////////////////////////////////////////////

CallbacksFileProvider::CallbacksFileProvider( const MP4IOCallbacks& callbacks, void* handle )
    : _handle( handle )
{
    memcpy( &_call, &callbacks, sizeof(MP4IOCallbacks) );
}

bool
CallbacksFileProvider::open( const std::string& name, Mode mode )
{
    return seek(0);
}

bool
CallbacksFileProvider::seek( Size pos )
{
    return _call.seek( _handle, pos );
}

bool
CallbacksFileProvider::read( void* buffer, Size size, Size& nin )
{
    return _call.read( _handle, buffer, size, &nin );
}

bool
CallbacksFileProvider::write( const void* buffer, Size size, Size& nout )
{
    return _call.write( _handle, buffer, size, &nout );
}

bool
CallbacksFileProvider::truncate( Size size )
{
    if( !_call.truncate )
        return true;

    return _call.truncate( _handle, size );
}

bool
CallbacksFileProvider::close()
{
    return false;
}

bool
CallbacksFileProvider::getSize( Size& nout )
{
    Size size = _call.size( _handle );
    if (size == -1)
        return true;

    nout = size;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
