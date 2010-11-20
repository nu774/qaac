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

#ifndef MP4V2_IMPL_MP4EXCEPTION_H
#define MP4V2_IMPL_MP4EXCEPTION_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4V2_EXPORT MP4Exception
{
protected:
    string _what;

public:
    const string& what;

public:
    explicit MP4Exception( const string& what );
    explicit MP4Exception( const ostringstream& what );
    ~MP4Exception();
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4EXCEPTION_H
