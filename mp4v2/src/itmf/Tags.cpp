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
//  Portions created by David Byron are Copyright (C) 2011.
//  All Rights Reserved.
//
//  Contributors:
//      Kona Blend, kona8lend@@gmail.com
//      Rouven Wessling, mp4v2@rouvenwessling.de
//      David Byron, dbyron@dbyron.com
//
///////////////////////////////////////////////////////////////////////////////

#include "impl.h"

namespace mp4v2 { namespace impl { namespace itmf {

///////////////////////////////////////////////////////////////////////////////

Tags::Tags()
    : hasMetadata(false)
{
}

///////////////////////////////////////////////////////////////////////////////

Tags::~Tags()
{
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_addArtwork( MP4Tags*& tags, MP4TagArtwork& c_artwork )
{
    artwork.resize( artwork.size() + 1 );
    c_setArtwork( tags, (uint32_t)artwork.size() - 1, c_artwork );
    updateArtworkShadow( tags );
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_alloc( MP4Tags*& tags )
{
    tags = new MP4Tags();
    memset( tags, 0, sizeof(MP4Tags) ); // safe: pure C-struct
    tags->__handle = this;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_fetch( MP4Tags*& tags, MP4FileHandle hFile )
{
    MP4Tags& c = *tags;
    MP4File& file = *static_cast<MP4File*>(hFile);

    MP4ItmfItemList* itemList = genericGetItems( file ); // alloc

    hasMetadata = (itemList->size > 0);

    /* create code -> item map.
     * map will only be used for items which do not repeat; we do not care if
     * cover-art is inserted multiple times.
     */
    CodeItemMap cim;
    for( uint32_t i = 0; i < itemList->size; i++ ) {
        MP4ItmfItem& item = itemList->elements[i];
        cim.insert( CodeItemMap::value_type( item.code, &item ));
    }

    fetchString(  cim, CODE_NAME,              name,              c.name );
    fetchString(  cim, CODE_ARTIST,            artist,            c.artist );
    fetchString(  cim, CODE_ALBUMARTIST,       albumArtist,       c.albumArtist );
    fetchString(  cim, CODE_ALBUM,             album,             c.album );
    fetchString(  cim, CODE_GROUPING,          grouping,          c.grouping );
    fetchString(  cim, CODE_COMPOSER,          composer,          c.composer );
    fetchString(  cim, CODE_COMMENTS,          comments,          c.comments );

    fetchString(  cim, CODE_GENRE,             genre,             c.genre );
    fetchGenre(   cim,                         genreType,         c.genreType );

    fetchString(  cim, CODE_RELEASEDATE,       releaseDate,       c.releaseDate );
    fetchTrack(   cim,                         track,             c.track );
    fetchDisk(    cim,                         disk,              c.disk );
    fetchInteger( cim, CODE_TEMPO,             tempo,             c.tempo );
    fetchInteger( cim, CODE_COMPILATION,       compilation,       c.compilation );

    fetchString(  cim, CODE_TVSHOW,            tvShow,            c.tvShow );
    fetchString(  cim, CODE_TVNETWORK,         tvNetwork,         c.tvNetwork );
    fetchString(  cim, CODE_TVEPISODEID,       tvEpisodeID,       c.tvEpisodeID );
    fetchInteger( cim, CODE_TVSEASON,          tvSeason,          c.tvSeason );
    fetchInteger( cim, CODE_TVEPISODE,         tvEpisode,         c.tvEpisode );

    fetchString(  cim, CODE_SORTNAME,          sortName,          c.sortName );
    fetchString(  cim, CODE_SORTARTIST,        sortArtist,        c.sortArtist );
    fetchString(  cim, CODE_SORTALBUMARTIST,   sortAlbumArtist,   c.sortAlbumArtist );
    fetchString(  cim, CODE_SORTALBUM,         sortAlbum,         c.sortAlbum );
    fetchString(  cim, CODE_SORTCOMPOSER,      sortComposer,      c.sortComposer );
    fetchString(  cim, CODE_SORTTVSHOW,        sortTVShow,        c.sortTVShow );

    fetchString(  cim, CODE_DESCRIPTION,       description,       c.description );
    fetchString(  cim, CODE_LONGDESCRIPTION,   longDescription,   c.longDescription );
    fetchString(  cim, CODE_LYRICS,            lyrics,            c.lyrics );

    fetchString(  cim, CODE_COPYRIGHT,         copyright,         c.copyright );
    fetchString(  cim, CODE_ENCODINGTOOL,      encodingTool,      c.encodingTool ); 
    fetchString(  cim, CODE_ENCODEDBY,         encodedBy,         c.encodedBy );
    fetchString(  cim, CODE_PURCHASEDATE,      purchaseDate,      c.purchaseDate );

    fetchInteger( cim, CODE_PODCAST,           podcast,           c.podcast );
    fetchString(  cim, CODE_KEYWORDS,          keywords,          c.keywords );
    fetchString(  cim, CODE_CATEGORY,          category,          c.category );

    fetchInteger( cim, CODE_HDVIDEO,           hdVideo,           c.hdVideo );
    fetchInteger( cim, CODE_MEDIATYPE,         mediaType,         c.mediaType );
    fetchInteger( cim, CODE_CONTENTRATING,     contentRating,     c.contentRating );
    fetchInteger( cim, CODE_GAPLESS,           gapless,           c.gapless );

    fetchString(  cim, CODE_ITUNESACCOUNT,     iTunesAccount,     c.iTunesAccount );
    fetchInteger( cim, CODE_ITUNESACCOUNTTYPE, iTunesAccountType, c.iTunesAccountType );
    fetchInteger( cim, CODE_ITUNESCOUNTRY,     iTunesCountry,     c.iTunesCountry );

    fetchInteger( cim, CODE_CONTENTID,         contentID,         c.contentID );
    fetchInteger( cim, CODE_ARTISTID,          artistID,          c.artistID );
    fetchInteger( cim, CODE_PLAYLISTID,        playlistID,        c.playlistID );
    fetchInteger( cim, CODE_GENREID,           genreID,           c.genreID );
    fetchInteger( cim, CODE_COMPOSERID,        composerID,        c.composerID );
    fetchString(  cim, CODE_XID,               xid,               c.xid );

    genericItemListFree( itemList ); // free

    // fetch full list and overwrite our copy, otherwise clear
    {
        CoverArtBox::ItemList items;
        if( CoverArtBox::list( hFile, items ))
            artwork.clear();
        else
            artwork = items;

        updateArtworkShadow( tags );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_free( MP4Tags*& tags )
{
    MP4Tags* c = const_cast<MP4Tags*>(tags);

    delete[] c->artwork;
    delete c;

    tags = NULL;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_removeArtwork( MP4Tags*& tags, uint32_t index ) 
{
    if( !(index < artwork.size()) )
        return;

    artwork.erase( artwork.begin() + index );
    updateArtworkShadow( tags );
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setArtwork( MP4Tags*& tags, uint32_t index, MP4TagArtwork& c_artwork )
{
    if( !(index < artwork.size()) )
        return;

    CoverArtBox::Item& item = artwork[index];

    switch( c_artwork.type ) {
        case MP4_ART_BMP:
            item.type = BT_BMP;
            break;

        case MP4_ART_GIF:
            item.type = BT_GIF;
            break;

        case MP4_ART_JPEG:
            item.type = BT_JPEG;
            break;

        case MP4_ART_PNG:
            item.type = BT_PNG;
            break;

        case MP4_ART_UNDEFINED:
        default:
            item.type = computeBasicType( c_artwork.data, c_artwork.size );
            break;
    }

    item.buffer   = (uint8_t*)malloc( c_artwork.size );
    item.size     = c_artwork.size;
    item.autofree = true;

    memcpy( item.buffer, c_artwork.data, c_artwork.size );
    updateArtworkShadow( tags );
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setInteger( const uint8_t* value, uint8_t& cpp, const uint8_t*& c )
{
    if( !value ) {
        cpp = 0;
        c = NULL;
    }
    else {
        cpp = *value;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setInteger( const uint16_t* value, uint16_t& cpp, const uint16_t*& c )
{
    if( !value ) {
        cpp = 0;
        c = NULL;
    }
    else {
        cpp = *value;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setInteger( const uint32_t* value, uint32_t& cpp, const uint32_t*& c )
{
    if( !value ) {
        cpp = 0;
        c = NULL;
    }
    else {
        cpp = *value;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setInteger( const uint64_t* value, uint64_t& cpp, const uint64_t*& c )
{
    if( !value ) {
        cpp = 0;
        c = NULL;
    }
    else {
        cpp = *value;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setString( const char* value, string& cpp, const char*& c )
{
    if( !value ) {
        cpp.clear();
        c = NULL;
    }
    else {
        cpp = value;
        c = cpp.c_str();
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setTrack( const MP4TagTrack* value, MP4TagTrack& cpp, const MP4TagTrack*& c )
{
    if( !value ) {
        cpp.index = 0;
        cpp.total = 0;
        c = NULL;
    }
    else {
        cpp.index = value->index;
        cpp.total = value->total;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_setDisk( const MP4TagDisk* value, MP4TagDisk& cpp, const MP4TagDisk*& c )
{
    if( !value ) {
        cpp.index = 0;
        cpp.total = 0;
        c = NULL;
    }
    else {
        cpp.index = value->index;
        cpp.total = value->total;
        c = &cpp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::c_store( MP4Tags*& tags, MP4FileHandle hFile )
{
    MP4Tags& c = *tags;
    MP4File& file = *static_cast<MP4File*>(hFile);
   
    storeString(  file, CODE_NAME,              name,              c.name );
    storeString(  file, CODE_ARTIST,            artist,            c.artist );
    storeString(  file, CODE_ALBUMARTIST,       albumArtist,       c.albumArtist );
    storeString(  file, CODE_ALBUM,             album,             c.album );
    storeString(  file, CODE_GROUPING,          grouping,          c.grouping );
    storeString(  file, CODE_COMPOSER,          composer,          c.composer );
    storeString(  file, CODE_COMMENTS,          comments,          c.comments );

    storeString(  file, CODE_GENRE,             genre,             c.genre );
    storeGenre(   file,                         genreType,         c.genreType );

    storeString(  file, CODE_RELEASEDATE,       releaseDate,       c.releaseDate );
    storeTrack(   file,                         track,             c.track );
    storeDisk(    file,                         disk,              c.disk );
    storeInteger( file, CODE_TEMPO,             tempo,             c.tempo );
    storeInteger( file, CODE_COMPILATION,       compilation,       c.compilation );
    
    storeString(  file, CODE_TVSHOW,            tvShow,            c.tvShow );
    storeString(  file, CODE_TVNETWORK,         tvNetwork,         c.tvNetwork );
    storeString(  file, CODE_TVEPISODEID,       tvEpisodeID,       c.tvEpisodeID );
    storeInteger( file, CODE_TVSEASON,          tvSeason,          c.tvSeason );
    storeInteger( file, CODE_TVEPISODE,         tvEpisode,         c.tvEpisode );
    
    storeString(  file, CODE_SORTNAME,          sortName,          c.sortName );
    storeString(  file, CODE_SORTARTIST,        sortArtist,        c.sortArtist );
    storeString(  file, CODE_SORTALBUMARTIST,   sortAlbumArtist,   c.sortAlbumArtist );
    storeString(  file, CODE_SORTALBUM,         sortAlbum,         c.sortAlbum );
    storeString(  file, CODE_SORTCOMPOSER,      sortComposer,      c.sortComposer );
    storeString(  file, CODE_SORTTVSHOW,        sortTVShow,        c.sortTVShow );

    storeString(  file, CODE_DESCRIPTION,       description,       c.description );
    storeString(  file, CODE_LONGDESCRIPTION,   longDescription,   c.longDescription );
    storeString(  file, CODE_LYRICS,            lyrics,            c.lyrics );

    storeString(  file, CODE_COPYRIGHT,         copyright,         c.copyright );
    storeString(  file, CODE_ENCODINGTOOL,      encodingTool,      c.encodingTool );
    storeString(  file, CODE_ENCODEDBY,         encodedBy,         c.encodedBy );
    storeString(  file, CODE_PURCHASEDATE,      purchaseDate,      c.purchaseDate );

    storeInteger( file, CODE_PODCAST,           podcast,           c.podcast );
    storeString(  file, CODE_KEYWORDS,          keywords,          c.keywords );
    storeString(  file, CODE_CATEGORY,          category,          c.category );

    storeInteger( file, CODE_HDVIDEO,           hdVideo,           c.hdVideo );
    storeInteger( file, CODE_MEDIATYPE,         mediaType,         c.mediaType );
    storeInteger( file, CODE_CONTENTRATING,     contentRating,     c.contentRating );
    storeInteger( file, CODE_GAPLESS,           gapless,           c.gapless );

    storeString(  file, CODE_ITUNESACCOUNT,     iTunesAccount,     c.iTunesAccount );
    storeInteger( file, CODE_ITUNESACCOUNTTYPE, iTunesAccountType, c.iTunesAccountType );
    storeInteger( file, CODE_ITUNESCOUNTRY,     iTunesCountry,     c.iTunesCountry );

    storeInteger( file, CODE_CONTENTID,         contentID,         c.contentID );
    storeInteger( file, CODE_ARTISTID,          artistID,          c.artistID );
    storeInteger( file, CODE_PLAYLISTID,        playlistID,        c.playlistID );
    storeInteger( file, CODE_GENREID,           genreID,           c.genreID );
    storeInteger( file, CODE_COMPOSERID,        composerID,        c.composerID );
    storeString(  file, CODE_XID,               xid,               c.xid );

    // destroy all cover-art then add each
    {
        CoverArtBox::remove( hFile );
        const CoverArtBox::ItemList::size_type max = artwork.size();
        for( CoverArtBox::ItemList::size_type i = 0; i < max; i++ )
            CoverArtBox::add( hFile, artwork[i] );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchGenre( const CodeItemMap& cim, uint16_t& cpp, const uint16_t*& c )
{
    cpp = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( CODE_GENRETYPE );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];
    if( NULL == data.value )
        return;

    cpp = (uint16_t(data.value[0]) <<  8)
        | (uint16_t(data.value[1])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchDisk( const CodeItemMap& cim, MP4TagDisk& cpp, const MP4TagDisk*& c )
{    
    cpp.index = 0;
    cpp.total = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( CODE_DISK );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp.index = (uint16_t(data.value[2]) <<  8)
              | (uint16_t(data.value[3])      );

    cpp.total = (uint16_t(data.value[4]) <<  8)
              | (uint16_t(data.value[5])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchTrack( const CodeItemMap& cim, MP4TagTrack& cpp, const MP4TagTrack*& c )
{    
    cpp.index = 0;
    cpp.total = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( CODE_TRACK );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp.index = (uint16_t(data.value[2]) <<  8)
              | (uint16_t(data.value[3])      );

    cpp.total = (uint16_t(data.value[4]) <<  8)
              | (uint16_t(data.value[5])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchInteger( const CodeItemMap& cim, const string& code, uint8_t& cpp, const uint8_t*& c )
{
    cpp = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( code );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];
    if( NULL == data.value )
        return;

    cpp = data.value[0];
    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchInteger( const CodeItemMap& cim, const string& code, uint16_t& cpp, const uint16_t*& c )
{
    cpp = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( code );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp = (uint16_t(data.value[0]) <<  8)
        | (uint16_t(data.value[1])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchInteger( const CodeItemMap& cim, const string& code, uint32_t& cpp, const uint32_t*& c )
{
    cpp = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( code );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp = (uint32_t(data.value[0]) << 24)
        | (uint32_t(data.value[1]) << 16)
        | (uint32_t(data.value[2]) <<  8)
        | (uint32_t(data.value[3])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchInteger( const CodeItemMap& cim, const string& code, uint64_t& cpp, const uint64_t*& c )
{
    cpp = 0;
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( code );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp = (uint64_t(data.value[0]) << 56)
        | (uint64_t(data.value[1]) << 48)
        | (uint64_t(data.value[2]) << 40)
        | (uint64_t(data.value[3]) << 32)
        | (uint64_t(data.value[4]) << 24)
        | (uint64_t(data.value[5]) << 16)
        | (uint64_t(data.value[6]) <<  8)
        | (uint64_t(data.value[7])      );

    c = &cpp;
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::fetchString( const CodeItemMap& cim, const string& code, string& cpp, const char*& c )
{
    cpp.clear();
    c = NULL;

    CodeItemMap::const_iterator f = cim.find( code );
    if( f == cim.end() || 0 == f->second->dataList.size )
        return;

    MP4ItmfData& data = f->second->dataList.elements[0];

    if( NULL == data.value )
        return;

    cpp.append( reinterpret_cast<char*>( data.value ), data.valueSize );
    c = cpp.c_str();
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::remove( MP4File& file, const string& code )
{
    MP4ItmfItemList* itemList = genericGetItemsByCode( file, code ); // alloc

    if( itemList->size )
        genericRemoveItem( file, &itemList->elements[0] );

    genericItemListFree( itemList ); // free
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::store( MP4File& file, const string& code, MP4ItmfBasicType basicType, const void* buffer, uint32_t size )
{
    // remove existing item
    remove( file, code );

    // add item
    MP4ItmfItem& item = *genericItemAlloc( code, 1 ); // alloc
    MP4ItmfData& data = item.dataList.elements[0];

    data.typeCode = basicType;
    data.valueSize = size;
    data.value = (uint8_t*)malloc( data.valueSize );
    memcpy( data.value, buffer, data.valueSize );

    genericAddItem( file, &item );
    genericItemFree( &item ); // free
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeGenre( MP4File& file, uint16_t cpp, const uint16_t* c )
{
    if( c ) {
        uint8_t buf[2];

        buf[0] = uint8_t((cpp & 0xff00) >> 8);
        buf[1] = uint8_t((cpp & 0x00ff)     );

        // it's not clear if you must use implicit in these situations and iirc iTunes and other software are not consistent in this regard.
        // many other tags must be integer type yet no issues there. Silly that iTunes insists it must be implict, which is then hardcoded 
        // to interpret as genres anyways.
        store( file, CODE_GENRETYPE, MP4_ITMF_BT_IMPLICIT, buf, sizeof(buf) );
    }
    else {
        remove( file, CODE_GENRETYPE );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeDisk( MP4File& file, const MP4TagDisk& cpp, const MP4TagDisk* c )
{
    if( c ) {
        uint8_t buf[6];
        memset( buf, 0, sizeof(buf) );

        buf[2] = uint8_t((cpp.index & 0xff00) >> 8);
        buf[3] = uint8_t((cpp.index & 0x00ff)     );
        buf[4] = uint8_t((cpp.total & 0xff00) >> 8);
        buf[5] = uint8_t((cpp.total & 0x00ff)     );

        store( file, CODE_DISK, MP4_ITMF_BT_IMPLICIT, buf, sizeof(buf) );
    }
    else {
        remove( file, CODE_DISK );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeTrack( MP4File& file, const MP4TagTrack& cpp, const MP4TagTrack* c )
{
    if( c ) {
        uint8_t buf[8]; // iTMF spec says 7 but iTunes media is 8
        memset( buf, 0, sizeof(buf) );

        buf[2] = uint8_t((cpp.index & 0xff00) >> 8);
        buf[3] = uint8_t((cpp.index & 0x00ff)     );
        buf[4] = uint8_t((cpp.total & 0xff00) >> 8);
        buf[5] = uint8_t((cpp.total & 0x00ff)     );

        store( file, CODE_TRACK, MP4_ITMF_BT_IMPLICIT, buf, sizeof(buf) );
    }
    else {
        remove( file, CODE_TRACK );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeInteger( MP4File& file, const string& code, uint8_t cpp, const uint8_t* c )
{
    if( c )
        store( file, code, MP4_ITMF_BT_INTEGER, &cpp, sizeof(cpp) );
    else
        remove( file, code );
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeInteger( MP4File& file, const string& code, uint16_t cpp, const uint16_t* c )
{
    if( c ) {
        uint8_t buf[2];

        buf[0] = uint8_t((cpp & 0xff00) >> 8);
        buf[1] = uint8_t((cpp & 0x00ff)     );

        store( file, code, MP4_ITMF_BT_INTEGER, buf, sizeof(buf) );
    }
    else {
        remove( file, code );
    }
}


///////////////////////////////////////////////////////////////////////////////

void
Tags::storeInteger( MP4File& file, const string& code, uint32_t cpp, const uint32_t* c )
{
    if( c ) {
        uint8_t buf[4];

        buf[0] = uint8_t((cpp & 0xff000000) >> 24 );
        buf[1] = uint8_t((cpp & 0x00ff0000) >> 16 );
        buf[2] = uint8_t((cpp & 0x0000ff00) >>  8 );
        buf[3] = uint8_t((cpp & 0x000000ff)       );

        store( file, code, MP4_ITMF_BT_INTEGER, buf, sizeof(buf) );
    }
    else {
        remove( file, code );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeInteger( MP4File& file, const string& code, uint64_t cpp, const uint64_t* c )
{
    if( c ) {
        uint8_t buf[8];

        buf[0] = uint8_t((cpp & 0xff00000000000000LL) >> 56 );
        buf[1] = uint8_t((cpp & 0x00ff000000000000LL) >> 48 );
        buf[2] = uint8_t((cpp & 0x0000ff0000000000LL) >> 40 );
        buf[3] = uint8_t((cpp & 0x000000ff00000000LL) >> 32 );
        buf[4] = uint8_t((cpp & 0x00000000ff000000LL) >> 24 );
        buf[5] = uint8_t((cpp & 0x0000000000ff0000LL) >> 16 );
        buf[6] = uint8_t((cpp & 0x000000000000ff00LL) >>  8 );
        buf[7] = uint8_t((cpp & 0x00000000000000ffLL)       );

        store( file, code, MP4_ITMF_BT_INTEGER, buf, sizeof(buf) );
    }
    else {
        remove( file, code );
    }
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::storeString( MP4File& file, const string& code, const string& cpp, const char* c )
{
    if( c )
        store( file, code, MP4_ITMF_BT_UTF8, cpp.c_str(), (uint32_t)cpp.size() );
    else
        remove( file, code );
}

///////////////////////////////////////////////////////////////////////////////

void
Tags::updateArtworkShadow( MP4Tags*& tags )
{
    if( tags->artwork ) {
        delete[] tags->artwork;
        tags->artwork = NULL;
        tags->artworkCount = 0;
    }

    if( artwork.empty() )
        return;

    MP4TagArtwork* const cartwork = new MP4TagArtwork[ artwork.size() ];
    uint32_t max = (uint32_t)artwork.size();

    for( uint32_t i = 0; i < max; i++ ) {
        MP4TagArtwork& a = cartwork[i];
        CoverArtBox::Item& item = artwork[i];

        a.data = item.buffer;
        a.size = item.size;

        switch( item.type ) {
            case BT_BMP:
                a.type = MP4_ART_BMP;
                break;

            case BT_GIF:
                a.type = MP4_ART_GIF;
                break;

            case BT_JPEG:
                a.type = MP4_ART_JPEG;
                break;

            case BT_PNG:
                a.type = MP4_ART_PNG;
                break;

            default:
                a.type = MP4_ART_UNDEFINED;
                break;
        }
    }

    tags->artwork      = cartwork;
    tags->artworkCount = max;
}

///////////////////////////////////////////////////////////////////////////////

const string Tags::CODE_NAME              = "\xa9" "nam";
const string Tags::CODE_ARTIST            = "\xa9" "ART";
const string Tags::CODE_ALBUMARTIST       = "aART";
const string Tags::CODE_ALBUM             = "\xa9" "alb";
const string Tags::CODE_GROUPING          = "\xa9" "grp";
const string Tags::CODE_COMPOSER          = "\xa9" "wrt";
const string Tags::CODE_COMMENTS          = "\xa9" "cmt";
const string Tags::CODE_GENRE             = "\xa9" "gen";
const string Tags::CODE_GENRETYPE         = "gnre";
const string Tags::CODE_RELEASEDATE       = "\xa9" "day";
const string Tags::CODE_TRACK             = "trkn";
const string Tags::CODE_DISK              = "disk";
const string Tags::CODE_TEMPO             = "tmpo";
const string Tags::CODE_COMPILATION       = "cpil";

const string Tags::CODE_TVSHOW            = "tvsh";
const string Tags::CODE_TVNETWORK         = "tvnn";
const string Tags::CODE_TVEPISODEID       = "tven";
const string Tags::CODE_TVSEASON          = "tvsn";
const string Tags::CODE_TVEPISODE         = "tves";

const string Tags::CODE_DESCRIPTION       = "desc";
const string Tags::CODE_LONGDESCRIPTION   = "ldes";
const string Tags::CODE_LYRICS            = "\xa9" "lyr";

const string Tags::CODE_SORTNAME          = "sonm";
const string Tags::CODE_SORTARTIST        = "soar";
const string Tags::CODE_SORTALBUMARTIST   = "soaa";
const string Tags::CODE_SORTALBUM         = "soal";
const string Tags::CODE_SORTCOMPOSER      = "soco";
const string Tags::CODE_SORTTVSHOW        = "sosn";

const string Tags::CODE_COPYRIGHT         = "cprt";
const string Tags::CODE_ENCODINGTOOL      = "\xa9" "too";
const string Tags::CODE_ENCODEDBY         = "\xa9" "enc";
const string Tags::CODE_PURCHASEDATE      = "purd";

const string Tags::CODE_PODCAST           = "pcst";
const string Tags::CODE_KEYWORDS          = "keyw";
const string Tags::CODE_CATEGORY          = "catg";

const string Tags::CODE_HDVIDEO           = "hdvd";
const string Tags::CODE_MEDIATYPE         = "stik";
const string Tags::CODE_CONTENTRATING     = "rtng";
const string Tags::CODE_GAPLESS           = "pgap";

const string Tags::CODE_ITUNESACCOUNT     = "apID";
const string Tags::CODE_ITUNESACCOUNTTYPE = "akID";
const string Tags::CODE_ITUNESCOUNTRY     = "sfID";
const string Tags::CODE_CONTENTID         = "cnID";
const string Tags::CODE_ARTISTID          = "atID";
const string Tags::CODE_PLAYLISTID        = "plID";
const string Tags::CODE_GENREID           = "geID";
const string Tags::CODE_COMPOSERID        = "cmID";
const string Tags::CODE_XID               = "xid ";

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::itmf
