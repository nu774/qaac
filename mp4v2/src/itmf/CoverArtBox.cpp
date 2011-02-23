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

#include "impl.h"

namespace mp4v2 { namespace impl { namespace itmf {

///////////////////////////////////////////////////////////////////////////////

CoverArtBox::Item::Item()
    : type     ( BT_UNDEFINED )
    , buffer   ( NULL )
    , size     ( 0 )
    , autofree ( false )
{
}

///////////////////////////////////////////////////////////////////////////////

CoverArtBox::Item::Item( const Item& rhs )
    : type     ( BT_UNDEFINED )
    , buffer   ( NULL )
    , size     ( 0 )
    , autofree ( false )
{
    operator=( rhs );
}

///////////////////////////////////////////////////////////////////////////////

CoverArtBox::Item::~Item()
{
    reset();
}

///////////////////////////////////////////////////////////////////////////////

CoverArtBox::Item&
CoverArtBox::Item::operator=( const Item& rhs )
{
    type     = rhs.type;
    size     = rhs.size;
    autofree = rhs.autofree;

    if( rhs.autofree ) {
        buffer = (uint8_t*)MP4Malloc(rhs.size);
        memcpy( buffer, rhs.buffer, rhs.size );
    }
    else {
        buffer = rhs.buffer;
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////////////

void
CoverArtBox::Item::reset()
{
    if( autofree && buffer )
        MP4Free( buffer );

    type     = BT_UNDEFINED;
    buffer   = NULL;
    size     = 0;
    autofree = false;
}

///////////////////////////////////////////////////////////////////////////////

bool
CoverArtBox::add( MP4FileHandle hFile, const Item& item )
{
    MP4File& file = *((MP4File*)hFile);

    const char* const covr_name = "moov.udta.meta.ilst.covr";
    MP4Atom* covr = file.FindAtom( covr_name );
    if( !covr ) {
        file.AddDescendantAtoms( "moov", "udta.meta.ilst.covr" );

        covr = file.FindAtom( covr_name );
        if( !covr )
            return true;
    }

    // use empty data atom if one exists
    MP4Atom* data = NULL;
    uint32_t index = 0;
    const uint32_t atomc = covr->GetNumberOfChildAtoms();
    for( uint32_t i = 0; i < atomc; i++ ) {
        MP4Atom* atom = covr->GetChildAtom( i );

        MP4BytesProperty* metadata = NULL;
        if( !atom->FindProperty( "data.metadata", (MP4Property**)&metadata ))
            continue;

        if( metadata->GetCount() )
            continue;

        data = atom;
        index = i;
        break;
    }

    // no empty atom found, create one.
    if( !data ) {
        data = MP4Atom::CreateAtom( file, covr, "data" );
        covr->AddChildAtom( data );
        data->Generate();
        index = covr->GetNumberOfChildAtoms() - 1;
    }

    return set( hFile, item, index );
}

///////////////////////////////////////////////////////////////////////////////

bool
CoverArtBox::get( MP4FileHandle hFile, Item& item, uint32_t index )
{
    item.reset();
    MP4File& file = *((MP4File*)hFile);

    MP4Atom* covr = file.FindAtom( "moov.udta.meta.ilst.covr" );
    if( !covr )
        return true;

    if( !(index < covr->GetNumberOfChildAtoms()) )
        return true;

    MP4DataAtom* data = static_cast<MP4DataAtom*>( covr->GetChildAtom( index ));
    if( !data )
        return true;

    MP4BytesProperty* metadata = NULL;
    if ( !data->FindProperty( "data.metadata", (MP4Property**)&metadata ))
        return true;

    metadata->GetValue( &item.buffer, &item.size );
    item.autofree = true;
    item.type = data->typeCode.GetValue();

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
CoverArtBox::list( MP4FileHandle hFile, ItemList& out )
{
    out.clear();
    MP4File& file = *((MP4File*)hFile);
    MP4ItmfItemList* itemList = genericGetItemsByCode( file, "covr" ); // alloc

    if( itemList->size ) {
        MP4ItmfDataList& dataList = itemList->elements[0].dataList;
        out.resize( dataList.size );
        for( uint32_t i = 0; i < dataList.size; i++ )
            get( hFile, out[i], i );
    }

    genericItemListFree( itemList ); // free
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
CoverArtBox::remove( MP4FileHandle hFile, uint32_t index )
{
    MP4File& file = *((MP4File*)hFile);

    MP4Atom* covr = file.FindAtom( "moov.udta.meta.ilst.covr" );
    if( !covr )
        return true;

    // wildcard mode: delete covr and all images
    if( index == numeric_limits<uint32_t>::max() ) {
        covr->GetParentAtom()->DeleteChildAtom( covr );
        delete covr;
        return false;
    }

    if( !(index < covr->GetNumberOfChildAtoms()) )
        return true;

    MP4Atom* data = covr->GetChildAtom( index );
    if( !data )
        return true;

    // delete single image
    covr->DeleteChildAtom( data );
    delete data;

    // delete empty covr
    if( covr->GetNumberOfChildAtoms() == 0 ) {
        covr->GetParentAtom()->DeleteChildAtom( covr );
        delete covr;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
CoverArtBox::set( MP4FileHandle hFile, const Item& item, uint32_t index )
{
    MP4File& file = *((MP4File*)hFile);

    MP4Atom* covr = file.FindAtom( "moov.udta.meta.ilst.covr" );
    if( !covr )
        return true;

    if( !(index < covr->GetNumberOfChildAtoms()) )
        return true;

    MP4DataAtom* data = static_cast<MP4DataAtom*>( covr->GetChildAtom( index ));
    if( !data )
        return true;

    MP4BytesProperty* metadata = NULL;
    if ( !data->FindProperty( "data.metadata", (MP4Property**)&metadata ))
        return true;

    // autodetect type if BT_UNDEFINED
    const BasicType final_type = (item.type == BT_UNDEFINED)
        ? computeBasicType( item.buffer, item.size )
        : item.type;

    // set type; note: not really flags due to b0rked atom structure
    data->typeCode.SetValue( final_type );
    metadata->SetValue( item.buffer, item.size );

    return false;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::itmf
