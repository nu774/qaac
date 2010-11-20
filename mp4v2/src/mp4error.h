/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 */

#ifndef MP4V2_IMPL_MP4ERROR_H
#define MP4V2_IMPL_MP4ERROR_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4V2_EXPORT MP4Error {
public:
    MP4Error();
    MP4Error( int err, const char* where = NULL );
    MP4Error( const char *format, const char *where, ... );
    MP4Error( int err, const char* format, const char* where, ... );
    ~MP4Error();

    void Print( FILE* pFile = stderr );

    int         m_errno;
    const char* m_errstring;
    const char* m_where;
    bool        m_free;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4ERROR_H
