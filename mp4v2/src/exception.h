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
//      David Byron, dbyron0@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MP4V2_IMPL_MP4EXCEPTION_H
#define MP4V2_IMPL_MP4EXCEPTION_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#   define EXCEPTION(message) mp4v2::impl::Exception(message)
#   define PLATFORM_EXCEPTION(message, errno) mp4v2::impl::PlatformException(message, errno)
#else
#   define EXCEPTION(message) mp4v2::impl::Exception(message, __FILE__, __LINE__, __FUNCTION__)
#   define PLATFORM_EXCEPTION(message, errno) mp4v2::impl::PlatformException(message, errno, __FILE__, __LINE__, __FUNCTION__)
#endif

class Exception
{
public:
    explicit Exception( const string&   what_ );
    explicit Exception( const string&   what_,
                        const char      *file_,
                        int             line_,
                        const char      *function_ );
    virtual ~Exception();

    virtual string      msg() const;

public:
    const string        what;
    const string        file;
    const int           line;
    const string        function;
};

class PlatformException : public Exception
{
public:
    explicit PlatformException( const string&   what_,
                                int             errno_ );
    explicit PlatformException( const string&   what_,
                                int             errno_,
                                const char      *file_,
                                int             line_,
                                const char      *function_ );
    virtual ~PlatformException();

    virtual string      msg() const;

public:
    const int   m_errno;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4EXCEPTION_H
