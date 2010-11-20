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
//      Rouven Wessling, mp4v2@rouvenwessling.de
//
///////////////////////////////////////////////////////////////////////////////

#include "src/impl.h"

#define PRINT_ERROR(e) VERBOSE_ERROR(((MP4File*)hFile)->GetVerbosity(), e->Print());

using namespace mp4v2::impl;

extern "C" {

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsAddArtwork( const MP4Tags* tags, MP4TagArtwork* artwork )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);
    cpp.c_addArtwork( c, *artwork );
}

///////////////////////////////////////////////////////////////////////////////

const MP4Tags*
MP4TagsAlloc()
{
    MP4Tags* result = NULL;
    itmf::Tags& m = *new itmf::Tags();
    m.c_alloc( result );
    return result;
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsFree( const MP4Tags* tags )
{
    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);
    cpp->c_free( c );
    delete cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsFetch( const MP4Tags* tags, MP4FileHandle hFile )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return;

    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp->c_fetch( c, hFile );
    }
    catch( MP4Error* e ) {
        VERBOSE_ERROR( static_cast<MP4File*>(hFile)->GetVerbosity(), e->Print() );
        delete e;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsRemoveArtwork( const MP4Tags* tags, uint32_t index )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);
    cpp.c_removeArtwork( c, index );
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsSetArtwork( const MP4Tags* tags, uint32_t index, MP4TagArtwork* artwork )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);
    cpp.c_setArtwork( c, index, *artwork );
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsStore( const MP4Tags* tags, MP4FileHandle hFile )
{
    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp->c_store( c, hFile );
    }
    catch( MP4Error* e ) {
        VERBOSE_ERROR( static_cast<MP4File*>(hFile)->GetVerbosity(), e->Print() );
        delete e;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsSetName( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.name, c.name );
}

void
MP4TagsSetArtist( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.artist, c.artist );
}

void
MP4TagsSetAlbumArtist( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.albumArtist, c.albumArtist );
}

void
MP4TagsSetAlbum( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.album, c.album );
}

void
MP4TagsSetGrouping( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.grouping, c.grouping );
}

void
MP4TagsSetComposer( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.composer, c.composer );
}

void
MP4TagsSetComments( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.comments, c.comments );
}

void
MP4TagsSetGenre( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.genre, c.genre );
}

void
MP4TagsSetGenreType( const MP4Tags* m, const uint16_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.genreType, c.genreType );
}

void
MP4TagsSetReleaseDate( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.releaseDate, c.releaseDate );
}

void
MP4TagsSetTrack( const MP4Tags* m, const MP4TagTrack* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setTrack( value, cpp.track, c.track );
}

void
MP4TagsSetDisk( const MP4Tags* m, const MP4TagDisk* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setDisk( value, cpp.disk, c.disk );
}

void
MP4TagsSetTempo( const MP4Tags* m, const uint16_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.tempo, c.tempo );
}

void
MP4TagsSetCompilation( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.compilation, c.compilation );
}

void
MP4TagsSetTVShow( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.tvShow, c.tvShow );
}

void
MP4TagsSetTVNetwork( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.tvNetwork, c.tvNetwork );
}

void
MP4TagsSetTVEpisodeID( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.tvEpisodeID, c.tvEpisodeID );
}

void
MP4TagsSetTVSeason( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.tvSeason, c.tvSeason );
}

void
MP4TagsSetTVEpisode( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.tvEpisode, c.tvEpisode );
}

void
MP4TagsSetSortName( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortName, c.sortName );
}

void
MP4TagsSetSortArtist( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortArtist, c.sortArtist );
}

void
MP4TagsSetSortAlbumArtist( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortAlbumArtist, c.sortAlbumArtist );
}

void
MP4TagsSetSortAlbum( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortAlbum, c.sortAlbum );
}

void
MP4TagsSetSortComposer( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortComposer, c.sortComposer );
}

void
MP4TagsSetSortTVShow( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.sortTVShow, c.sortTVShow );
}

void
MP4TagsSetDescription( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.description, c.description );
}

void
MP4TagsSetLongDescription( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.longDescription, c.longDescription );
}

void
MP4TagsSetLyrics( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.lyrics, c.lyrics );
}

void
MP4TagsSetCopyright( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.copyright, c.copyright );
}

void
MP4TagsSetEncodingTool( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.encodingTool, c.encodingTool );
}

void
MP4TagsSetEncodedBy( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.encodedBy, c.encodedBy );
}

void
MP4TagsSetPurchaseDate( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.purchaseDate, c.purchaseDate );
}

void
MP4TagsSetPodcast( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.podcast, c.podcast );
}

void
MP4TagsSetKeywords( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.keywords, c.keywords );
}

void
MP4TagsSetCategory( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.category, c.category );
}

void
MP4TagsSetHDVideo( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.hdVideo, c.hdVideo );
}

void
MP4TagsSetMediaType( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.mediaType, c.mediaType );
}

void
MP4TagsSetContentRating( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.contentRating, c.contentRating );
}

void
MP4TagsSetGapless( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.gapless, c.gapless );
}

void
MP4TagsSetITunesAccount( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.iTunesAccount, c.iTunesAccount );
}

void
MP4TagsSetITunesAccountType( const MP4Tags* m, const uint8_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.iTunesAccountType, c.iTunesAccountType );
}

void
MP4TagsSetITunesCountry( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.iTunesCountry, c.iTunesCountry );
}

void
MP4TagsSetContentID( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.contentID, c.contentID );
}

void
MP4TagsSetArtistID( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.artistID, c.artistID );
}

void
MP4TagsSetPlaylistID( const MP4Tags* m, const uint64_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.playlistID, c.playlistID );
}

void
MP4TagsSetGenreID( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.genreID, c.genreID );
}

void
MP4TagsSetComposerID( const MP4Tags* m, const uint32_t* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setInteger( value, cpp.composerID, c.composerID );
}

void
MP4TagsSetXID( const MP4Tags* m, const char* value )
{
    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    cpp.c_setString( value, cpp.xid, c.xid );
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItem*
MP4ItmfItemAlloc( const char* code, uint32_t numData )
{
    return itmf::genericItemAlloc( code, numData );
}

///////////////////////////////////////////////////////////////////////////////

void
MP4ItmfItemFree( MP4ItmfItem* item )
{
    itmf::genericItemFree( item );
}

///////////////////////////////////////////////////////////////////////////////

void
MP4ItmfItemListFree( MP4ItmfItemList* list )
{
    itmf::genericItemListFree( list );
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
MP4ItmfGetItems( MP4FileHandle hFile )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return NULL;

    try {
        return itmf::genericGetItems( *(MP4File*)hFile );
    }
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
MP4ItmfGetItemsByCode( MP4FileHandle hFile, const char* code )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return NULL;

    try {
        return itmf::genericGetItemsByCode( *(MP4File*)hFile, code );
    }   
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
MP4ItmfGetItemsByMeaning( MP4FileHandle hFile, const char* meaning, const char* name )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return NULL;

    try {
        return itmf::genericGetItemsByMeaning( *(MP4File*)hFile, meaning, name ? name : "" );
    }
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4ItmfAddItem( MP4FileHandle hFile, const MP4ItmfItem* item )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return itmf::genericAddItem( *(MP4File*)hFile, item );
    }
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4ItmfSetItem( MP4FileHandle hFile, const MP4ItmfItem* item )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return itmf::genericSetItem( *(MP4File*)hFile, item );
    }
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4ItmfRemoveItem( MP4FileHandle hFile, const MP4ItmfItem* item )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return itmf::genericRemoveItem( *(MP4File*)hFile, item );
    }
    catch( MP4Error* e ) {
        PRINT_ERROR( e );
        delete e;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

} // extern "C"
