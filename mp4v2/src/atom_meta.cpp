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
 *      M. Bakker     mbakker at nero.com
 *
 * Apple iTunes META data
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4DataAtom::MP4DataAtom(MP4File &file)
    : MP4Atom ( file, "data" )
    , typeReserved      ( *new MP4Integer16Property( *this, "typeReserved" ))
    , typeSetIdentifier ( *new MP4Integer8Property( *this, "typeSetIdentifier" ))
    , typeCode          ( *new MP4BasicTypeProperty( *this, "typeCode" ))
    , locale            ( *new MP4Integer32Property( *this, "locale" ))
    , metadata          ( *new MP4BytesProperty( *this, "metadata" ))
{
    AddProperty( &typeReserved );
    AddProperty( &typeSetIdentifier );
    AddProperty( &typeCode );
    AddProperty( &locale );
    AddProperty( &metadata );
}

void
MP4DataAtom::Read()
{
    // calculate size of the metadata from the atom size
    metadata.SetValueSize( m_size - 8 );
    MP4Atom::Read();
}

///////////////////////////////////////////////////////////////////////////////

MP4FullAtom::MP4FullAtom( MP4File &file, const char* type )
    : MP4Atom ( file, type )
    , version ( *new MP4Integer8Property( *this, "version" ))
    , flags   ( *new MP4Integer24Property( *this, "flags" ))
{
    AddProperty( &version );
    AddProperty( &flags );
}

///////////////////////////////////////////////////////////////////////////////

MP4ItemAtom::MP4ItemAtom( MP4File &file, const char* type )
    : MP4Atom( file, type )
{
    ExpectChildAtom( "mean", Optional, OnlyOne );
    ExpectChildAtom( "name", Optional, OnlyOne );
    ExpectChildAtom( "data", Required, Many );
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfHdlrAtom::MP4ItmfHdlrAtom(MP4File &file)
    : MP4FullAtom ( file, "hdlr" )
    , reserved1   ( *new MP4Integer32Property( *this, "reserved1" ))
    , handlerType ( *new MP4BytesProperty( *this, "handlerType", 4 ))
    , reserved2   ( *new MP4BytesProperty( *this, "reserved2", 12 ))
    , name        ( *new MP4BytesProperty( *this, "name", 1 ))
{
    AddProperty( &reserved1 );
    AddProperty( &handlerType );
    AddProperty( &reserved2 );
    AddProperty( &name );

    const uint8_t htData[] = { 'm', 'd', 'i', 'r' };
    handlerType.SetValue( htData, sizeof( htData ));

    const uint8_t nameData[] = { 0 };
    name.SetValue( nameData, sizeof( nameData ));
}

void
MP4ItmfHdlrAtom::Read()
{
    name.SetValueSize( m_size - 24 );
    MP4FullAtom::Read();
}

///////////////////////////////////////////////////////////////////////////////

MP4MeanAtom::MP4MeanAtom(MP4File &file)
    : MP4FullAtom ( file, "mean" )
    , value       ( *new MP4BytesProperty( *this, "value" ))
{
    AddProperty( &value );
}

void
MP4MeanAtom::Read()
{
    value.SetValueSize( m_size - 4 );
    MP4Atom::Read();
}

///////////////////////////////////////////////////////////////////////////////

MP4NameAtom::MP4NameAtom(MP4File &file)
    : MP4FullAtom ( file, "name" )
    , value       ( *new MP4BytesProperty( *this, "value" ))
{
    AddProperty( &value );
}

void
MP4NameAtom::Read()
{
    value.SetValueSize( m_size - 4 );
    MP4FullAtom::Read();
}

///////////////////////////////////////////////////////////////////////////////

MP4UdtaElementAtom::MP4UdtaElementAtom( MP4File &file, const char* type )
    : MP4Atom ( file, type )
    , value   ( *new MP4BytesProperty( *this, "value" ))
{
    AddProperty( &value );
}

void
MP4UdtaElementAtom::Read()
{
    // calculate size of the metadata from the atom size
    value.SetValueSize( m_size );
    MP4Atom::Read();
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
