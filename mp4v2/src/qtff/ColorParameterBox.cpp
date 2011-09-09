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

namespace mp4v2 { namespace impl { namespace qtff {

///////////////////////////////////////////////////////////////////////////////

namespace {
    const string BOX_CODE = "colr";

    bool findColorParameterBox( MP4FileHandle file, MP4Atom& coding, MP4Atom*& colr );
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::add( MP4FileHandle file, uint16_t trackIndex, const Item& item )
{
    MP4Atom* coding;

    if( !MP4_IS_VALID_FILE_HANDLE( file ))
        throw new Exception( "invalid file handle", __FILE__, __LINE__, __FUNCTION__ );

    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* colr;
    if( !findColorParameterBox( file, *coding, colr ))
        throw new Exception( "colr-box already exists", __FILE__, __LINE__, __FUNCTION__ );

    colr = MP4Atom::CreateAtom( *((MP4File *)file), coding, BOX_CODE.c_str() );
    coding->AddChildAtom( colr );
    colr->Generate();

    MP4StringProperty*    type;
    MP4Integer16Property* primariesIndex;
    MP4Integer16Property* transferFunctionIndex;
    MP4Integer16Property* matrixIndex;

    if( colr->FindProperty( "colr.colorParameterType", (MP4Property**)&type ))
        type->SetValue( "nclc" );

    if( colr->FindProperty( "colr.primariesIndex", (MP4Property**)&primariesIndex ))
        primariesIndex->SetValue( item.primariesIndex );

    if( colr->FindProperty( "colr.transferFunctionIndex", (MP4Property**)&transferFunctionIndex ))
        transferFunctionIndex->SetValue( item.transferFunctionIndex );

    if( colr->FindProperty( "colr.matrixIndex", (MP4Property**)&matrixIndex ))
        matrixIndex->SetValue( item.matrixIndex );

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::add( MP4FileHandle file, MP4TrackId trackId, const Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return add( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::get( MP4FileHandle file, uint16_t trackIndex, Item& item )
{
    item.reset();

    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* colr;
    if( findColorParameterBox( file, *coding, colr ))
        throw new Exception( "colr-box not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Integer16Property* primariesIndex;
    MP4Integer16Property* transferFunctionIndex;
    MP4Integer16Property* matrixIndex;

    if( colr->FindProperty( "colr.primariesIndex", (MP4Property**)&primariesIndex ))
        item.primariesIndex = primariesIndex->GetValue();

    if( colr->FindProperty( "colr.transferFunctionIndex", (MP4Property**)&transferFunctionIndex ))
        item.transferFunctionIndex = transferFunctionIndex->GetValue();

    if( colr->FindProperty( "colr.matrixIndex", (MP4Property**)&matrixIndex ))
        item.matrixIndex = matrixIndex->GetValue();

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::get( MP4FileHandle file, MP4TrackId trackId, Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return get( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::list( MP4FileHandle file, ItemList& itemList )
{
    itemList.clear();
    MP4File& mp4 = *((MP4File*)file);

    const uint16_t trackc = mp4.GetNumberOfTracks();
    for( uint16_t i = 0; i < trackc; i++) {
        MP4TrackId id = mp4.FindTrackId( i );
        if( id == MP4_INVALID_TRACK_ID )
            continue;

        const char* type = mp4.GetTrackType( id );
        if( !type )
            continue;

        itemList.resize( itemList.size() + 1 );
        IndexedItem& xitem = itemList[itemList.size()-1];

        xitem.trackIndex = i;
        xitem.trackId    = id;

        bool success = false;
        try {
            success = !get( file, i, xitem.item );
        }
        catch( Exception* x ) {
            delete x;
        }

        if( !success ) {
            itemList.resize( itemList.size() - 1 );
            continue;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::remove( MP4FileHandle file, uint16_t trackIndex )
{
    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* colr;
    if( findColorParameterBox( file, *coding, colr ))
        throw new Exception( "colr-box not found", __FILE__, __LINE__, __FUNCTION__ );

    coding->DeleteChildAtom( colr );
    delete colr;

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::remove( MP4FileHandle file, MP4TrackId trackId )
{
    MP4File& mp4 = *((MP4File*)file);
    return remove( file, mp4.FindTrackIndex( trackId ));
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::set( MP4FileHandle file, uint16_t trackIndex, const Item& item )
{
    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* colr;
    if( findColorParameterBox( file, *coding, colr ))
        throw new Exception( "colr-box not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Integer16Property* primariesIndex;
    MP4Integer16Property* transferFunctionIndex;
    MP4Integer16Property* matrixIndex;

    if( colr->FindProperty( "colr.primariesIndex", (MP4Property**)&primariesIndex ))
        primariesIndex->SetValue( item.primariesIndex );

    if( colr->FindProperty( "colr.transferFunctionIndex", (MP4Property**)&transferFunctionIndex ))
        transferFunctionIndex->SetValue( item.transferFunctionIndex );

    if( colr->FindProperty( "colr.matrixIndex", (MP4Property**)&matrixIndex ))
        matrixIndex->SetValue( item.matrixIndex );

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
ColorParameterBox::set( MP4FileHandle file, MP4TrackId trackId, const Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return set( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

ColorParameterBox::IndexedItem::IndexedItem()
    : trackIndex ( numeric_limits<uint16_t>::max() )
    , trackId    ( MP4_INVALID_TRACK_ID )
{
}

///////////////////////////////////////////////////////////////////////////////

ColorParameterBox::Item::Item()
    : primariesIndex        ( 6 )
    , transferFunctionIndex ( 1 )
    , matrixIndex           ( 6 )
{
}

///////////////////////////////////////////////////////////////////////////////

void
ColorParameterBox::Item::convertFromCSV( const string& text )
{
    istringstream iss( text );
    char delim;

    iss >> primariesIndex;
    iss >> delim;
    iss >> transferFunctionIndex;
    iss >> delim;
    iss >> matrixIndex;

    // input was good if we end up with only eofbit set
    if( iss.rdstate() != ios::eofbit ) {
        reset();
        ostringstream xss;
        xss << "invalid ColorParameterBox format"
            << " (expecting: INDEX1,INDEX2,INDEX3)"
            << " got: " << text;
        throw new Exception( xss.str(), __FILE__, __LINE__, __FUNCTION__ );
    }
}

///////////////////////////////////////////////////////////////////////////////

string
ColorParameterBox::Item::convertToCSV() const
{
    string buffer;
    return convertToCSV( buffer );
}

///////////////////////////////////////////////////////////////////////////////

string&
ColorParameterBox::Item::convertToCSV( string& buffer ) const
{
    ostringstream oss;
    oss << primariesIndex << ',' << transferFunctionIndex << ',' << matrixIndex;
    buffer = oss.str();
    return buffer;
}

///////////////////////////////////////////////////////////////////////////////

void
ColorParameterBox::Item::reset()
{
    primariesIndex        = 6;
    transferFunctionIndex = 1;
    matrixIndex           = 6;
}

///////////////////////////////////////////////////////////////////////////////

namespace {

///////////////////////////////////////////////////////////////////////////////

bool
findColorParameterBox( MP4FileHandle file, MP4Atom& coding, MP4Atom*& colr )
{
    colr = NULL;

    MP4Atom* found = NULL;
    const uint32_t atomc = coding.GetNumberOfChildAtoms();
    for( uint32_t i = 0; i < atomc; i++ ) {
        MP4Atom* atom = coding.GetChildAtom( i );
        if( BOX_CODE != atom->GetType() )
            continue;
        found = atom;
    }
    if( !found )
        return true;

    MP4StringProperty* type;
    if( !found->FindProperty( "colr.colorParameterType", (MP4Property**)&type ))
        return true;

    const string type_nclc = "nclc";
    if( type_nclc != type->GetValue() )
        return true;

    colr = found;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

} // anonymous

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::qtff
