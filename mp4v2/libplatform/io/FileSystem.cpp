#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

void
FileSystem::pathnameCleanup( string& name )
{
    string bad;

    // fold repeating directory separators
    bad = DIR_SEPARATOR;
    bad += DIR_SEPARATOR;
    for( string::size_type pos = name.find( bad );
         pos != string::npos;
         pos = name.find( bad, pos ) )
    {
        name.replace( pos, bad.length(), DIR_SEPARATOR );
    }

    // replace occurances of /./ with /
    bad = DIR_SEPARATOR;
    bad += '.';
    bad += DIR_SEPARATOR;
    for( string::size_type pos = name.find( bad );
         pos != string::npos;
         pos = name.find( bad, pos ) )
    {
        name.replace( pos, bad.length(), DIR_SEPARATOR );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
FileSystem::pathnameOnlyExtension( string& name )
{
    // compute basename
    string::size_type dot_pos = name.rfind( '.' );
    string::size_type slash_pos = name.rfind( DIR_SEPARATOR );

    // dot_pos must be after slash_pos
    if( slash_pos != string::npos && dot_pos < slash_pos )
        dot_pos = string::npos;

    // return empty-string if no dot
    if( dot_pos == string::npos ) {
        name.resize( 0 );
        return;
    }

    name = name.substr( dot_pos + 1 );
    pathnameCleanup( name );
}

///////////////////////////////////////////////////////////////////////////////

void
FileSystem::pathnameStripExtension( string& name )
{
    pathnameCleanup( name );

    // compute basename
    string::size_type dot_pos = name.rfind( '.' );
    string::size_type slash_pos = name.rfind( DIR_SEPARATOR );

    // dot_pos must be after slash_pos
    if( slash_pos != string::npos && dot_pos < slash_pos )
        dot_pos = string::npos;

    // chop extension
    if( dot_pos != string::npos )
        name.resize( dot_pos );
}

///////////////////////////////////////////////////////////////////////////////

void
FileSystem::pathnameTemp( string& name, string dir, string prefix, string suffix )
{
    ostringstream buf;

    if( !dir.empty() ) {
        buf << dir;

        // add dir separator if needed
        // TODO there's a platform specific bug here, if someone passes in a pathname ending
        // in '\', which would be legitimate on Windows.
        if( dir[dir.length()-1] != '/' )
            buf << '/';
    }

    buf << prefix;
    buf << setfill('0') << setw(8) << number::random32();
    buf << suffix;

    name = buf.str();
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io
