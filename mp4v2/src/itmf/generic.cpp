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

namespace mp4v2 { namespace impl { namespace itmf { namespace {

///////////////////////////////////////////////////////////////////////////////

void
__dataInit( MP4ItmfData& data )
{
    data.typeSetIdentifier = 0;
    data.typeCode          = MP4_ITMF_BT_IMPLICIT;
    data.locale            = 0;
    data.value             = NULL;
    data.valueSize         = 0;
}

void
__dataClear( MP4ItmfData& data )
{
    if( data.value )
        free( data.value );
    __dataInit( data );
}

///////////////////////////////////////////////////////////////////////////////

void
__dataListInit( MP4ItmfDataList& list )
{
    list.elements = NULL;
    list.size     = 0;
}

void
__dataListClear( MP4ItmfDataList& list )
{
    if( list.elements ) {
        for( uint32_t i = 0; i < list.size; i++ )
            __dataClear( list.elements[i] );
        free( list.elements );
    }

    __dataListInit( list );
}

void
__dataListResize( MP4ItmfDataList& list, uint32_t size )
{
    __dataListClear( list );

    list.elements = (MP4ItmfData*)malloc( sizeof( MP4ItmfData ) * size );
    list.size     = size;

    for( uint32_t i = 0; i < size; i++ )
        __dataInit( list.elements[i] );
}

///////////////////////////////////////////////////////////////////////////////

void
__itemInit( MP4ItmfItem& item )
{
    item.__handle = NULL;
    item.code     = NULL;
    item.mean     = NULL;
    item.name     = NULL;

    __dataListInit( item.dataList );
}

void
__itemClear( MP4ItmfItem& item )
{
    if( item.code )
        free( item.code );
    if( item.mean )
        free( item.mean );
    if( item.name )
        free( item.name );

    __dataListClear( item.dataList );
    __itemInit( item );
}

///////////////////////////////////////////////////////////////////////////////

void
__itemListInit( MP4ItmfItemList& list )
{
    list.elements = NULL;
    list.size     = 0;
}

void
__itemListClear( MP4ItmfItemList& list )
{
    if( list.elements ) {
        for( uint32_t i = 0; i < list.size; i++ )
            __itemClear( list.elements[i] );
        free( list.elements );
    }

    __itemListInit( list );
}

void
__itemListResize( MP4ItmfItemList& list, uint32_t size )
{
    __itemListClear( list );
    if( !size )
        return;

    list.elements = (MP4ItmfItem*)malloc( sizeof( MP4ItmfItem ) * size );
    list.size     = size;

    for( uint32_t i = 0; i < size; i++ )
        __itemInit( list.elements[i] );
}

MP4ItmfItemList*
__itemListAlloc()
{
    MP4ItmfItemList& list = *(MP4ItmfItemList*)malloc( sizeof( MP4ItmfItemList ));
    __itemListInit( list );
    return &list;
}

///////////////////////////////////////////////////////////////////////////////

static bool
__itemAtomToModel( MP4ItemAtom& item_atom, MP4ItmfItem& model )
{
    __itemClear( model );
    model.__handle = &item_atom;
    model.code     = strdup( item_atom.GetType() );

    // handle special meaning atom
    if( ATOMID( item_atom.GetType() ) == ATOMID( "----" )) {
        // meaning is mandatory
        MP4MeanAtom* mean = (MP4MeanAtom*)item_atom.FindAtom( "----.mean" );
        if( !mean )
            return true;

        // copy atom UTF-8 value (not NULL-terminated) to model (NULL-terminated)
        model.mean = mean->value.GetValueStringAlloc();

        // name is optional
        MP4NameAtom* name = (MP4NameAtom*)item_atom.FindAtom( "----.name" );
        if( name ) {
            // copy atom UTF-8 value (not NULL-terminated) to model (NULL-terminated)
            model.name = name->value.GetValueStringAlloc();
        }
    }

    // pass 1: count data atoms
    const uint32_t childCount = item_atom.GetNumberOfChildAtoms();
    uint32_t dataCount = 0;
    for( uint32_t i = 0; i < childCount; i++ ) {
        if( ATOMID( item_atom.GetChildAtom( i )->GetType() ) != ATOMID( "data" ))
            continue;
        dataCount++;
    }

    // one or more data atoms is mandatory
    if( dataCount < 1 )
        return true;

    __dataListResize( model.dataList, dataCount );

    // pass 2: populate data model
    for( uint32_t i = 0, idata = 0; i < childCount; i++ ) {
        MP4Atom* atom = item_atom.GetChildAtom( i );
        if( ATOMID( atom->GetType() ) != ATOMID( "data" ))
            continue;

        MP4DataAtom& data_atom = *(MP4DataAtom*)atom;
        MP4ItmfData& data_model = model.dataList.elements[idata];

        data_model.typeSetIdentifier = data_atom.typeSetIdentifier.GetValue();
        data_model.typeCode          = (MP4ItmfBasicType)data_atom.typeCode.GetValue();
        data_model.locale            = data_atom.locale.GetValue();

        data_atom.metadata.GetValue( &data_model.value, &data_model.valueSize );
        idata++;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

static bool
__itemModelToAtom( const MP4ItmfItem& model, MP4ItemAtom& atom )
{
    if( ATOMID( atom.GetType() ) == ATOMID( "----" )) {
        ASSERT( model.mean ); // mandatory
        MP4MeanAtom& meanAtom = *(MP4MeanAtom*)MP4Atom::CreateAtom( atom.GetFile(), &atom, "mean" );
        atom.AddChildAtom( &meanAtom );
        meanAtom.value.SetValue( (const uint8_t*)model.mean, (uint32_t)strlen( model.mean ));

        if( model.name ) {
            MP4NameAtom& nameAtom = *(MP4NameAtom*)MP4Atom::CreateAtom( atom.GetFile(), &atom, "name" );
            atom.AddChildAtom( &nameAtom );
            nameAtom.value.SetValue( (const uint8_t*)model.name, (uint32_t)strlen( model.name ));
        }
    }

    for( uint32_t i = 0; i < model.dataList.size; i++ ) {
        MP4ItmfData& dataModel = model.dataList.elements[i];
        MP4DataAtom& dataAtom = *(MP4DataAtom*)MP4Atom::CreateAtom( atom.GetFile(), &atom, "data" );
        atom.AddChildAtom( &dataAtom );

        dataAtom.typeSetIdentifier.SetValue( dataModel.typeSetIdentifier );
        dataAtom.typeCode.SetValue( (itmf::BasicType)dataModel.typeCode );
        dataAtom.locale.SetValue( dataModel.locale );
        dataAtom.metadata.SetValue( dataModel.value, dataModel.valueSize );
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace anonymous

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItem*
genericItemAlloc( const string& code, uint32_t numData )
{
    MP4ItmfItem* item = (MP4ItmfItem*)malloc( sizeof( MP4ItmfItem ));
    if( !item )
        return NULL;

    __itemInit( *item );
    item->code = strdup( code.c_str() );

    // always create array size of 1
    __dataListResize( item->dataList, numData );

    return item;
}

///////////////////////////////////////////////////////////////////////////////

void
genericItemFree( MP4ItmfItem* item )
{
    if( !item )
        return;

    __itemClear( *item );
    free( item );
}

///////////////////////////////////////////////////////////////////////////////

void
genericItemListFree( MP4ItmfItemList* list )
{
    if( !list )
        return;

    __itemListClear( *list );
    free( list );
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
genericGetItems( MP4File& file )
{
    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst )
        return __itemListAlloc();

    const uint32_t itemCount = ilst->GetNumberOfChildAtoms();
    if( itemCount < 1 )
        return __itemListAlloc();

    MP4ItmfItemList& list = *__itemListAlloc();
    __itemListResize( list, itemCount );

    for( uint32_t i = 0; i < list.size; i++ )
        __itemAtomToModel( *(MP4ItemAtom*)ilst->GetChildAtom( i ), list.elements[i] );

    return &list;
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
genericGetItemsByCode( MP4File& file, const string& code )
{
    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst )
        return __itemListAlloc();

    // pass 1: filter by code and populate indexList
    const uint32_t childCount = ilst->GetNumberOfChildAtoms();
    vector<uint32_t> indexList;
    for( uint32_t i = 0; i < childCount; i++ ) {
        if( ATOMID( ilst->GetChildAtom( i )->GetType() ) != ATOMID( code.c_str() ))
            continue;
        indexList.push_back( i );
    }

    if( indexList.size() < 1 )
        return __itemListAlloc();

    MP4ItmfItemList& list = *__itemListAlloc();
    __itemListResize( list, (uint32_t)indexList.size() );

    // pass 2: process each atom
    const vector<uint32_t>::size_type max = indexList.size();
    for( vector<uint32_t>::size_type i = 0; i < max; i++ ) {
        uint32_t& aidx = indexList[i];
        __itemAtomToModel( *(MP4ItemAtom*)ilst->GetChildAtom( aidx ), list.elements[i] );
    }

    return &list;
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
genericGetItemsByMeaning( MP4File& file, const string& meaning, const string& name )
{
    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst )
        return __itemListAlloc();

    // pass 1: filter by code and populate indexList
    const uint32_t childCount = ilst->GetNumberOfChildAtoms();
    vector<uint32_t> indexList;
    for( uint32_t i = 0; i < childCount; i++ ) {
        MP4Atom& atom = *ilst->GetChildAtom( i );
        if( ATOMID( atom.GetType() ) != ATOMID( "----" ))
            continue;

        // filter-out meaning mismatch
        MP4MeanAtom* meanAtom = (MP4MeanAtom*)atom.FindAtom( "----.mean" );
        if( !meanAtom )
            continue;
        if( meanAtom->value.CompareToString( meaning ))
            continue;

        if( !name.empty() ) {
            // filter-out name mismatch
            MP4MeanAtom* nameAtom = (MP4MeanAtom*)atom.FindAtom( "----.name" );
            if( !nameAtom )
                continue;
            if( nameAtom->value.CompareToString( name ))
                continue;
        }

        indexList.push_back( i );
    }

    if( indexList.size() < 1 )
        return __itemListAlloc();

    MP4ItmfItemList& list = *__itemListAlloc();
    __itemListResize( list, (uint32_t)indexList.size() );

    // pass 2: process each atom
    const vector<uint32_t>::size_type max = indexList.size();
    for( vector<uint32_t>::size_type i = 0; i < max; i++ ) {
        uint32_t& aidx = indexList[i];
        __itemAtomToModel( *(MP4ItemAtom*)ilst->GetChildAtom( aidx ), list.elements[i] );
    }

    return &list;
}

///////////////////////////////////////////////////////////////////////////////

bool
genericAddItem( MP4File& file, const MP4ItmfItem* item )
{
    if( !item )
        return false;

    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst ) {
        file.AddDescendantAtoms( "moov", "udta.meta.ilst" );
        ilst = file.FindAtom( "moov.udta.meta.ilst" );
        ASSERT( ilst );
    }

    MP4ItemAtom& itemAtom = *(MP4ItemAtom*)MP4Atom::CreateAtom( file, ilst, item->code );
    ilst->AddChildAtom( &itemAtom );

    return __itemModelToAtom( *item, itemAtom );
}

///////////////////////////////////////////////////////////////////////////////

bool
genericSetItem( MP4File& file, const MP4ItmfItem* item )
{
    if( !item || !item->__handle )
        return false;

    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst )
        return false;

    MP4ItemAtom* const old = static_cast<MP4ItemAtom*>(item->__handle);
    const uint32_t childCount = ilst->GetNumberOfChildAtoms();
    uint32_t fidx = numeric_limits<uint32_t>::max();
    for( uint32_t i = 0; i < childCount; i++ ) {
        MP4Atom* atom = ilst->GetChildAtom( i );
        if( atom == old ) {
            fidx = i;
            break;
        }
    }

    if( fidx == numeric_limits<uint32_t>::max() )
        return false;

    ilst->DeleteChildAtom( old );
    delete old;

    MP4ItemAtom& itemAtom = *(MP4ItemAtom*)MP4Atom::CreateAtom( file, ilst, item->code );
    ilst->InsertChildAtom( &itemAtom, fidx );

    return __itemModelToAtom( *item, itemAtom );
}

///////////////////////////////////////////////////////////////////////////////

bool
genericRemoveItem( MP4File& file, const MP4ItmfItem* item )
{
    if( !item || !item->__handle )
        return false;

    MP4Atom* ilst = file.FindAtom( "moov.udta.meta.ilst" );
    if( !ilst )
        return false;

    MP4ItemAtom* const old = static_cast<MP4ItemAtom*>(item->__handle);
    ilst->DeleteChildAtom( old );
    delete old;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::itmf
