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
//  Portions created by David Byron are Copyright (C) 2009.
//  All Rights Reserved.
//
//  Contributors:
//      Kona Blend, kona8lend@@gmail.com
//      David Byron, dbyron0@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

Exception::Exception( const string&     what_,
                      const char        *file_,
                      int               line_,
                      const char        *function_ )
    : what(what_)
    , file(file_)
    , line(line_)
    , function(function_)
{
    ASSERT(file_);
    ASSERT(function_);
}

///////////////////////////////////////////////////////////////////////////////

Exception::~Exception()
{
}

///////////////////////////////////////////////////////////////////////////////

string
Exception::msg() const
{
    ostringstream retval;

    retval << function << ": " << what << " (" << file << "," << line << ")";

    return retval.str();
}

///////////////////////////////////////////////////////////////////////////////

PlatformException::PlatformException( const string&     what_,
                                      int               errno_,
                                      const char        *file_,
                                      int               line_,
                                      const char        *function_ )
    : Exception(what_,file_,line_,function_)
    , m_errno(errno_)
{
}

///////////////////////////////////////////////////////////////////////////////

PlatformException::~PlatformException()
{
}

///////////////////////////////////////////////////////////////////////////////

string
PlatformException::msg() const
{
    ostringstream retval;

    retval << function << ": " << what << ": errno: " << m_errno << " (" <<
        file << "," << line << ")";

    return retval.str();
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
