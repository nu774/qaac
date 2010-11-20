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

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4FtypAtom::MP4FtypAtom()
    : MP4Atom( "ftyp" )
    , majorBrand       ( *new MP4StringProperty( "majorBrand" ))
    , minorVersion     ( *new MP4Integer32Property( "minorVersion" ))
    , compatibleBrands ( *new MP4StringProperty( "compatibleBrands", false, false, true ))
{
    majorBrand.SetFixedLength( 4 );
    compatibleBrands.SetFixedLength( 4 );

    AddProperty( &majorBrand );
    AddProperty( &minorVersion );
    AddProperty( &compatibleBrands );
}

void MP4FtypAtom::Generate()
{
    MP4Atom::Generate();

    majorBrand.SetValue( "mp42" );
    minorVersion.SetValue( 0 );

    compatibleBrands.SetCount( 2 );
    compatibleBrands.SetValue( "mp42", 0 );
    compatibleBrands.SetValue( "isom", 1 );
}

void MP4FtypAtom::Read()
{
    compatibleBrands.SetCount( (m_size - 8) / 4 ); // brands array fills rest of atom
    MP4Atom::Read();
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
