///////////////////////////////////////////////////////////////////////////////
//
//  The contents of this file are subject to the Mozilla Public License
//  Version 1.1 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.mozilla.org/MPL/
//
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations
//  under the License.
// 
//  The Original Code is MP4v2.
// 
//  The Initial Developer of the Original Code is Kona Blend.
//  Portions created by Kona Blend are Copyright (C) 2008.
//  All Rights Reserved.
//
//  Contributors:
//      Kona Blend, kona8lend@@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MP4V2_IMPL_ENUM_TCC
#define MP4V2_IMPL_ENUM_TCC

#include <sstream>

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

template <typename T, T UNDEFINED>
Enum<T,UNDEFINED>::Enum()
    : mapToType   ( _mapToType )
    , mapToString ( _mapToString )
{
    for( const Entry* p = data; p->type != UNDEFINED; p++ ) {
        _mapToType.insert( typename MapToType::value_type( p->compact, p ));
        _mapToString.insert( typename MapToString::value_type( p->type, p ));
    }
}

//////////////////////////////////////////////////////////////////////////////

template <typename T, T UNDEFINED>
Enum<T,UNDEFINED>::~Enum()
{
}

//////////////////////////////////////////////////////////////////////////////

template <typename T, T UNDEFINED>
string
Enum<T,UNDEFINED>::toString( T value, bool formal ) const
{
    string buffer;
    return toString( value, buffer, formal );
}

//////////////////////////////////////////////////////////////////////////////

template <typename T, T UNDEFINED>
string&
Enum<T,UNDEFINED>::toString( T value, string& buffer, bool formal ) const
{
    const typename MapToString::const_iterator found = _mapToString.find( value );
    if( found != _mapToString.end() ) {
        const Entry& entry = *(found->second);
        buffer = formal ? entry.formal : entry.compact;
        return buffer;
    }

    ostringstream oss;
    oss << "UNDEFINED(" << value << ")";
    buffer = oss.str();
    return buffer;
}

//////////////////////////////////////////////////////////////////////////////

template <typename T, T UNDEFINED>
T
Enum<T,UNDEFINED>::toType( const string& value ) const
{
    // if number perform enum lookup
    int ivalue;
    istringstream iss( value );
    iss >> ivalue;
    if( iss.rdstate() == ios::eofbit ) {
        const typename MapToString::const_iterator found = _mapToString.find( static_cast<T>(ivalue) );
        if( found != _mapToString.end() )
            return found->second->type;
    }

    // exact match
    const typename MapToType::const_iterator found = _mapToType.find( value );
    if( found != _mapToType.end() )
        return found->second->type;

    // partial match
    int matches = 0;
    T matched = static_cast<T>( 0 );

    const typename MapToType::const_iterator ie = _mapToType.end();
    for( typename MapToType::const_iterator it = _mapToType.begin(); it != ie; it++ ) {
        const Entry& entry = *(it->second);
        if( entry.compact.find( value ) == 0 ) {
            matches++;
            matched = entry.type;
        }
    }

    return (matches == 1) ? matched : UNDEFINED;
}

//////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_ENUM_TCC
