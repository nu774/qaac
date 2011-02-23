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
//  Portions created by David Byron are Copyright (C) 2009, 2010, 2011.
//  All Rights Reserved.
//
//  Contributors:
//      Kona Blend, kona8lend@@gmail.com
//      Rouven Wessling, mp4v2@rouvenwessling.de
//      David Byron, dbyron0@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#include "src/impl.h"

using namespace mp4v2::impl;

extern "C" {

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsAddArtwork( const MP4Tags* tags, MP4TagArtwork* artwork )
{
    if( !tags || !tags->__handle || !artwork )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp.c_addArtwork( c, *artwork );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__);
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

const MP4Tags*
MP4TagsAlloc()
{
    MP4Tags* result = NULL;
    itmf::Tags* m = NULL;

    try {
        m = new itmf::Tags();
        m->c_alloc( result );
        return result;
    }
    catch( std::bad_alloc ) {
        // This could be a failure to allocate itmf::Tags or
        // a failure to allocate inside c_alloc.
        mp4v2::impl::log.errorf("%s: memory allocation error", __FUNCTION__);
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
    }

    if( result )
        delete result;

    if( m )
        delete m;

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

void
MP4TagsFree( const MP4Tags* tags )
{
    if( !tags || !tags->__handle )
        return;

    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp->c_free( c );
        delete cpp;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
    }
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsFetch( const MP4Tags* tags, MP4FileHandle hFile )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    if( !tags || !tags->__handle )
        return false;

    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp->c_fetch( c, hFile );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsHasMetadata ( const MP4Tags* tags, bool *hasMetadata )
{
    if( !tags || !tags->__handle || !hasMetadata )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);

    (*hasMetadata) = cpp.hasMetadata;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsRemoveArtwork( const MP4Tags* tags, uint32_t index )
{
    if( !tags || !tags->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp.c_removeArtwork( c, index );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsSetArtwork( const MP4Tags* tags, uint32_t index, MP4TagArtwork* artwork )
{
    if( !tags || !tags->__handle || !artwork)
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp.c_setArtwork( c, index, *artwork );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsStore( const MP4Tags* tags, MP4FileHandle hFile )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    if( !tags || !tags->__handle )
        return false;

    itmf::Tags* cpp = static_cast<itmf::Tags*>(tags->__handle);
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    try {
        cpp->c_store( c, hFile );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool
MP4TagsSetName( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.name, c.name );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetArtist( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.artist, c.artist );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetAlbumArtist( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.albumArtist, c.albumArtist );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetAlbum( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.album, c.album );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetGrouping( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.grouping, c.grouping );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetComposer( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.composer, c.composer );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetComments( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.comments, c.comments );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetGenre( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.genre, c.genre );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetGenreType( const MP4Tags* m, const uint16_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setInteger( value, cpp.genreType, c.genreType );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetReleaseDate( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setString( value, cpp.releaseDate, c.releaseDate );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTrack( const MP4Tags* m, const MP4TagTrack* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setTrack( value, cpp.track, c.track );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetDisk( const MP4Tags* m, const MP4TagDisk* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setDisk( value, cpp.disk, c.disk );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTempo( const MP4Tags* m, const uint16_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.tempo, c.tempo );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetCompilation( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.compilation, c.compilation );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTVShow( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.tvShow, c.tvShow );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTVNetwork( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.tvNetwork, c.tvNetwork );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTVEpisodeID( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.tvEpisodeID, c.tvEpisodeID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTVSeason( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.tvSeason, c.tvSeason );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetTVEpisode( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.tvEpisode, c.tvEpisode );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortName( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortName, c.sortName );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortArtist( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortArtist, c.sortArtist );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortAlbumArtist( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortAlbumArtist, c.sortAlbumArtist );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortAlbum( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortAlbum, c.sortAlbum );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortComposer( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortComposer, c.sortComposer );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetSortTVShow( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.sortTVShow, c.sortTVShow );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetDescription( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.description, c.description );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetLongDescription( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.longDescription, c.longDescription );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetLyrics( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.lyrics, c.lyrics );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetCopyright( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.copyright, c.copyright );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetEncodingTool( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.encodingTool, c.encodingTool );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetEncodedBy( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.encodedBy, c.encodedBy );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetPurchaseDate( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.purchaseDate, c.purchaseDate );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetPodcast( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.podcast, c.podcast );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetKeywords( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.keywords, c.keywords );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetCategory( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.category, c.category );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetHDVideo( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.hdVideo, c.hdVideo );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetMediaType( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.mediaType, c.mediaType );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetContentRating( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.contentRating, c.contentRating );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetGapless( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.gapless, c.gapless );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetITunesAccount( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.iTunesAccount, c.iTunesAccount );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetITunesAccountType( const MP4Tags* m, const uint8_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.iTunesAccountType, c.iTunesAccountType );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetITunesCountry( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.iTunesCountry, c.iTunesCountry );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetContentID( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.contentID, c.contentID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetArtistID( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.artistID, c.artistID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetPlaylistID( const MP4Tags* m, const uint64_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setInteger( value, cpp.playlistID, c.playlistID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetGenreID( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setInteger( value, cpp.genreID, c.genreID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetComposerID( const MP4Tags* m, const uint32_t* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);

    try {
        cpp.c_setInteger( value, cpp.composerID, c.composerID );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
}

bool
MP4TagsSetXID( const MP4Tags* m, const char* value )
{
    if( !m || !m->__handle )
        return false;

    itmf::Tags& cpp = *static_cast<itmf::Tags*>(m->__handle);
    MP4Tags& c = *const_cast<MP4Tags*>(m);
    
    try {
        cpp.c_setString( value, cpp.xid, c.xid );
        return true;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return false;
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
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
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
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed",__FUNCTION__);
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

MP4ItmfItemList*
MP4ItmfGetItemsByMeaning( MP4FileHandle hFile, const char* meaning, const char* name )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return NULL;

    if( !meaning )
        return NULL;

    try {
        return itmf::genericGetItemsByMeaning( *(MP4File*)hFile, meaning, name ? name : "" );
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
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
    catch( Exception* x) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
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
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
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
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

} // extern "C"
