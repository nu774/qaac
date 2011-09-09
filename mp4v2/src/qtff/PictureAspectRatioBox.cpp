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
    const string BOX_CODE = "pasp";

    bool findPictureAspectRatioBox( MP4FileHandle file, MP4Atom& coding, MP4Atom*& pasp );
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::add( MP4FileHandle file, uint16_t trackIndex, const Item& item )
{
    MP4Atom* coding;

    if( !MP4_IS_VALID_FILE_HANDLE( file ))
        throw new Exception( "invalid file handle", __FILE__, __LINE__, __FUNCTION__ );

    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* pasp;
    if( !findPictureAspectRatioBox( file, *coding, pasp ))
        throw new Exception( "pasp-box already exists", __FILE__, __LINE__, __FUNCTION__ );

    pasp = MP4Atom::CreateAtom( *((MP4File *)file), coding, BOX_CODE.c_str() );
    coding->AddChildAtom( pasp );
    pasp->Generate();

    MP4Integer16Property* hSpacing;
    MP4Integer16Property* vSpacing;

    if( pasp->FindProperty( "pasp.hSpacing", (MP4Property**)&hSpacing ))
        hSpacing->SetValue( item.hSpacing );

    if( pasp->FindProperty( "pasp.vSpacing", (MP4Property**)&vSpacing ))
        vSpacing->SetValue( item.vSpacing );

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::add( MP4FileHandle file, MP4TrackId trackId, const Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return add( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::get( MP4FileHandle file, uint16_t trackIndex, Item& item )
{
    item.reset();

    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* pasp;
    if( findPictureAspectRatioBox( file, *coding, pasp ))
        throw new Exception( "pasp-box not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Integer16Property* hSpacing;
    MP4Integer16Property* vSpacing;

    if( pasp->FindProperty( "pasp.hSpacing", (MP4Property**)&hSpacing ))
        item.hSpacing = hSpacing->GetValue();

    if( pasp->FindProperty( "pasp.vSpacing", (MP4Property**)&vSpacing ))
        item.vSpacing = vSpacing->GetValue();

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::get( MP4FileHandle file, MP4TrackId trackId, Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return get( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::list( MP4FileHandle file, ItemList& itemList )
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
PictureAspectRatioBox::remove( MP4FileHandle file, uint16_t trackIndex )
{
    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* pasp;
    if( findPictureAspectRatioBox( file, *coding, pasp ))
        throw new Exception( "pasp-box not found", __FILE__, __LINE__, __FUNCTION__ );

    coding->DeleteChildAtom( pasp );
    delete pasp;

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::remove( MP4FileHandle file, MP4TrackId trackId )
{
    MP4File& mp4 = *((MP4File*)file);
    return remove( file, mp4.FindTrackIndex( trackId ));
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::set( MP4FileHandle file, uint16_t trackIndex, const Item& item )
{
    MP4Atom* coding;
    if( findCoding( file, trackIndex, coding ))
        throw new Exception( "supported coding not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Atom* pasp;
    if( findPictureAspectRatioBox( file, *coding, pasp ))
        throw new Exception( "pasp-box not found", __FILE__, __LINE__, __FUNCTION__ );

    MP4Integer16Property* hSpacing;
    MP4Integer16Property* vSpacing;

    if( pasp->FindProperty( "pasp.hSpacing", (MP4Property**)&hSpacing ))
        hSpacing->SetValue( item.hSpacing );

    if( pasp->FindProperty( "pasp.vSpacing", (MP4Property**)&vSpacing ))
        vSpacing->SetValue( item.vSpacing );

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
PictureAspectRatioBox::set( MP4FileHandle file, MP4TrackId trackId, const Item& item )
{
    MP4File& mp4 = *((MP4File*)file);
    return set( file, mp4.FindTrackIndex( trackId ), item );
}

///////////////////////////////////////////////////////////////////////////////

PictureAspectRatioBox::IndexedItem::IndexedItem()
    : trackIndex ( numeric_limits<uint16_t>::max() )
    , trackId    ( MP4_INVALID_TRACK_ID )
{
}

///////////////////////////////////////////////////////////////////////////////

PictureAspectRatioBox::Item::Item()
    : hSpacing ( 1 )
    , vSpacing ( 1 )
{
}

///////////////////////////////////////////////////////////////////////////////

void
PictureAspectRatioBox::Item::reset()
{
    hSpacing = 1;
    vSpacing = 1;
}

///////////////////////////////////////////////////////////////////////////////

void
PictureAspectRatioBox::Item::convertFromCSV( const string& text )
{
    istringstream iss( text );
    char delim;

    iss >> hSpacing;
    iss >> delim;
    iss >> vSpacing;

    // input was good if we end up with only eofbit set
    if( iss.rdstate() != ios::eofbit ) {
        reset();
        ostringstream xss;
        xss << "invalid PcitureAspectRatioBox format"
            << " (expecting: hSpacing,vSpacing)"
            << " got: " << text;
        throw new Exception( xss.str(), __FILE__, __LINE__, __FUNCTION__ );
    }
}

///////////////////////////////////////////////////////////////////////////////

string
PictureAspectRatioBox::Item::convertToCSV() const
{
    string buffer;
    return convertToCSV( buffer );
}

///////////////////////////////////////////////////////////////////////////////

string&
PictureAspectRatioBox::Item::convertToCSV( string& buffer ) const
{
    ostringstream oss;
    oss << hSpacing << ',' << vSpacing;
    buffer = oss.str();
    return buffer;
}

///////////////////////////////////////////////////////////////////////////////

namespace {

///////////////////////////////////////////////////////////////////////////////

bool
findPictureAspectRatioBox( MP4FileHandle file, MP4Atom& coding, MP4Atom*& pasp )
{
    pasp = NULL;

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

    pasp = found;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

}}}} // namespace mp4v2::impl::qtff::anonymous
