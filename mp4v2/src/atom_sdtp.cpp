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

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4SdtpAtom::MP4SdtpAtom(MP4File &file)
    : MP4FullAtom ( file, "sdtp" )
    , data        ( *new MP4BytesProperty( *this, "data" ))
{
    AddProperty( &data );
}

///////////////////////////////////////////////////////////////////////////////

void MP4SdtpAtom::Read()
{
    data.SetValueSize( m_size - 4 );
    MP4FullAtom::Read();
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
