#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

bool
LessIgnoreCase::operator()( const string& xstr, const string& ystr ) const
{
    const string::size_type xlen = xstr.length();
    const string::size_type ylen = ystr.length();

    if( xlen < ylen ) {
        for( string::size_type i = 0; i < xlen; i++ ) {
            const char x = std::toupper( xstr[i] );
            const char y = std::toupper( ystr[i] );

            if( x < y )
                return true;
            else if ( x > y )
                return false;
        }
        return true;
    }
    else {
        for( string::size_type i = 0; i < ylen; i++ ) {
            const char x = std::toupper( xstr[i] );
            const char y = std::toupper( ystr[i] );

            if( x < y )
                return true;
            else if ( x > y )
                return false;
        }
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
