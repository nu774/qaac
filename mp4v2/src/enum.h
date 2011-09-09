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

#ifndef MP4V2_IMPL_ENUM_H
#define MP4V2_IMPL_ENUM_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

/// class Template to support enums with to/from string conversions.
///
/// This class template is meant only to add support for enums which have
/// useful string equivalents. The model is that each enum value has
/// two string equivalents: compact and formal. <b>compact</b> is a short,
/// compact string which usually excludes spaces and punctuation and makes it
/// suitable for use with command-line arguments or any situation where
/// superfluous characters make parsing needlessly complicated. <b>formal</b>
/// is a string suitable for use in human-readable situations where spaces,
/// punctuation and even case is desirable.
///
/// For end usability, enums will have the full list of enums available
/// which is suitable for help-usage scenerios. And all values will be
/// convertable from enum to string, or from string to enum. When converting
/// from enum to string, you may optionally specify a boolean value which
/// will return the <b>formal</b> string value; otherwise a <b>compact</b>
/// value is returned.
///
/// Conversion from string to enum (integral) value will always assume
/// <b>compact</b> string is used as it makes little sense to convert formal
/// strings to enum. Furthermore, the string conversion is optimized to
/// ignore case, and in the case an exact full-string match is not found,
/// a <b>best-match</b> is then checked for. Basically this means that if
/// enough beginning characters are used to match exactly 1 string-enum,
/// it is considered a match.
///
/// The template has 2 strict requirements. First, the enum must be a true
/// enum type; ie: not just some integer constants. Second, the enum must have
/// a value which indicates an undefined or illegal value; which is used as
/// a return value by string-to-enum conversion to indicate the string did
/// not match.
///
/// This template implementation itself should never be exposed. That is
/// to say, the .tcc file must not be used by code outside this library.
///
/// WARNING: since enum types are typically made static file scope,
/// care must be taken to make sure Entry data[] initialization occurs
/// in the <b>same file</b> and <b>before</b> instantiation.
///
template <typename T, T UNDEFINED>
class Enum
{
public:
    struct MP4V2_EXPORT Entry
    {
        T type;
        const string compact;
        const string formal;
    };

    typedef map<string,const Entry*,LessIgnoreCase> MapToType;
    typedef map<T,const Entry*> MapToString;

public:
    static const Entry data[];

private:
    MapToType   _mapToType;
    MapToString _mapToString;

public:
    const MapToType&   mapToType;
    const MapToString& mapToString;

public:
    Enum();
    ~Enum();

    T       toType   ( const string& ) const;
    string  toString ( T, bool = false ) const;
    string& toString ( T, string&, bool = false ) const;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#include "enum.tcc"

#endif // MP4V2_IMPL_ENUM_H
