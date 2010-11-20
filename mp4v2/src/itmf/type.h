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

#ifndef MP4V2_IMPL_ITMF_TYPE_H
#define MP4V2_IMPL_ITMF_TYPE_H

namespace mp4v2 { namespace impl { namespace itmf {

///////////////////////////////////////////////////////////////////////////////

/// Basic set of types as detailed in spec.
enum BasicType {
    BT_IMPLICIT  = 0,   ///< for use with tags for which no type needs to be indicated
    BT_UTF8      = 1,   ///< without any count or null terminator
    BT_UTF16     = 2,   ///< also known as UTF-16BE
    BT_SJIS      = 3,   ///< deprecated unless it is needed for special Japanese characters
    BT_HTML      = 6,   ///< the HTML file header specifies which HTML version
    BT_XML       = 7,   ///< the XML header must identify the DTD or schemas
    BT_UUID      = 8,   ///< also known as GUID; stored as 16 bytes in binary (valid as an ID)
    BT_ISRC      = 9,   ///< stored as UTF-8 text (valid as an ID)
    BT_MI3P      = 10,  ///< stored as UTF-8 text (valid as an ID)
    BT_GIF       = 12,  ///< (deprecated) a GIF image
    BT_JPEG      = 13,  ///< a JPEG image
    BT_PNG       = 14,  ///< a PNG image
    BT_URL       = 15,  ///< absolute, in UTF-8 characters
    BT_DURATION  = 16,  ///< in milliseconds, 32-bit integer
    BT_DATETIME  = 17,  ///< in UTC, counting seconds since midnight, January 1, 1904; 32 or 64-bits
    BT_GENRES    = 18,  ///< a list of enumerated values, see #Genre
    BT_INTEGER   = 21,  ///< a signed big-endian integer with length one of { 1,2,3,4,8 } bytes
    BT_RIAA_PA   = 24,  ///< RIAA parental advisory; { -1=no, 1=yes, 0=unspecified }, 8-bit ingteger
    BT_UPC       = 25,  ///< Universal Product Code, in text UTF-8 format (valid as an ID)
    BT_BMP       = 27,  ///< Windows bitmap image

    BT_UNDEFINED = 255
};

typedef Enum<BasicType,BT_UNDEFINED> EnumBasicType;
MP4V2_EXPORT extern const EnumBasicType enumBasicType;

///////////////////////////////////////////////////////////////////////////////

/// enumerated genre as defined in ID3v1 specification but +1 as per iTMF spec.
/// Note values 80 and higher are Winamp extensions.
enum GenreType {
    GENRE_UNDEFINED          = 0,

    /* ID3v1 standard */
    GENRE_BLUES              = 1,
    GENRE_CLASSIC_ROCK       = 2,
    GENRE_COUNTRY            = 3,
    GENRE_DANCE              = 4,
    GENRE_DISCO              = 5,
    GENRE_FUNK               = 6,
    GENRE_GRUNGE             = 7,
    GENRE_HIP_HOP            = 8,
    GENRE_JAZZ               = 9,
    GENRE_METAL              = 10,
    GENRE_NEW_AGE            = 11,
    GENRE_OLDIES             = 12,
    GENRE_OTHER              = 13,
    GENRE_POP                = 14,
    GENRE_R_AND_B            = 15,
    GENRE_RAP                = 16,
    GENRE_REGGAE             = 17,
    GENRE_ROCK               = 18,
    GENRE_TECHNO             = 19,
    GENRE_INDUSTRIAL         = 20,
    GENRE_ALTERNATIVE        = 21,
    GENRE_SKA                = 22,
    GENRE_DEATH_METAL        = 23,
    GENRE_PRANKS             = 24,
    GENRE_SOUNDTRACK         = 25,
    GENRE_EURO_TECHNO        = 26,
    GENRE_AMBIENT            = 27,
    GENRE_TRIP_HOP           = 28,
    GENRE_VOCAL              = 29,
    GENRE_JAZZ_FUNK          = 30,
    GENRE_FUSION             = 31,
    GENRE_TRANCE             = 32,
    GENRE_CLASSICAL          = 33,
    GENRE_INSTRUMENTAL       = 34,
    GENRE_ACID               = 35,
    GENRE_HOUSE              = 36,
    GENRE_GAME               = 37,
    GENRE_SOUND_CLIP         = 38,
    GENRE_GOSPEL             = 39,
    GENRE_NOISE              = 40,
    GENRE_ALTERNROCK         = 41,
    GENRE_BASS               = 42,
    GENRE_SOUL               = 43,
    GENRE_PUNK               = 44,
    GENRE_SPACE              = 45,
    GENRE_MEDITATIVE         = 46,
    GENRE_INSTRUMENTAL_POP   = 47,
    GENRE_INSTRUMENTAL_ROCK  = 48,
    GENRE_ETHNIC             = 49,
    GENRE_GOTHIC             = 50,
    GENRE_DARKWAVE           = 51,
    GENRE_TECHNO_INDUSTRIAL  = 52,
    GENRE_ELECTRONIC         = 53,
    GENRE_POP_FOLK           = 54,
    GENRE_EURODANCE          = 55,
    GENRE_DREAM              = 56,
    GENRE_SOUTHERN_ROCK      = 57,
    GENRE_COMEDY             = 58,
    GENRE_CULT               = 59,
    GENRE_GANGSTA            = 60,
    GENRE_TOP_40             = 61,
    GENRE_CHRISTIAN_RAP      = 62,
    GENRE_POP_FUNK           = 63,
    GENRE_JUNGLE             = 64,
    GENRE_NATIVE_AMERICAN    = 65,
    GENRE_CABARET            = 66,
    GENRE_NEW_WAVE           = 67,
    GENRE_PSYCHEDELIC        = 68,
    GENRE_RAVE               = 69,
    GENRE_SHOWTUNES          = 70,
    GENRE_TRAILER            = 71,
    GENRE_LO_FI              = 72,
    GENRE_TRIBAL             = 73,
    GENRE_ACID_PUNK          = 74,
    GENRE_ACID_JAZZ          = 75,
    GENRE_POLKA              = 76,
    GENRE_RETRO              = 77,
    GENRE_MUSICAL            = 78,
    GENRE_ROCK_AND_ROLL      = 79,

    /* Winamp extension */
    GENRE_HARD_ROCK          = 80,
    GENRE_FOLK               = 81,
    GENRE_FOLK_ROCK          = 82,
    GENRE_NATIONAL_FOLK      = 83,
    GENRE_SWING              = 84,
    GENRE_FAST_FUSION        = 85,
    GENRE_BEBOB              = 86,
    GENRE_LATIN              = 87,
    GENRE_REVIVAL            = 88,
    GENRE_CELTIC             = 89,
    GENRE_BLUEGRASS          = 90,
    GENRE_AVANTGARDE         = 91,
    GENRE_GOTHIC_ROCK        = 92,
    GENRE_PROGRESSIVE_ROCK   = 93,
    GENRE_PSYCHEDELIC_ROCK   = 94,
    GENRE_SYMPHONIC_ROCK     = 95,
    GENRE_SLOW_ROCK          = 96,
    GENRE_BIG_BAND           = 97,
    GENRE_CHORUS             = 98,
    GENRE_EASY_LISTENING     = 99,
    GENRE_ACOUSTIC           = 100,
    GENRE_HUMOUR             = 101,
    GENRE_SPEECH             = 102,
    GENRE_CHANSON            = 103,
    GENRE_OPERA              = 104,
    GENRE_CHAMBER_MUSIC      = 105,
    GENRE_SONATA             = 106,
    GENRE_SYMPHONY           = 107,
    GENRE_BOOTY_BASS         = 108,
    GENRE_PRIMUS             = 109,
    GENRE_PORN_GROOVE        = 110,
    GENRE_SATIRE             = 111,
    GENRE_SLOW_JAM           = 112,
    GENRE_CLUB               = 113,
    GENRE_TANGO              = 114,
    GENRE_SAMBA              = 115,
    GENRE_FOLKLORE           = 116,
    GENRE_BALLAD             = 117,
    GENRE_POWER_BALLAD       = 118,
    GENRE_RHYTHMIC_SOUL      = 119,
    GENRE_FREESTYLE          = 120,
    GENRE_DUET               = 121,
    GENRE_PUNK_ROCK          = 122,
    GENRE_DRUM_SOLO          = 123,
    GENRE_A_CAPELLA          = 124,
    GENRE_EURO_HOUSE         = 125,
    GENRE_DANCE_HALL         = 126,

    GENRE_NONE = 255
};

typedef Enum<GenreType,GENRE_UNDEFINED> EnumGenreType;
MP4V2_EXPORT extern const EnumGenreType enumGenreType;

///////////////////////////////////////////////////////////////////////////////

/// enumerated 8-bit Video Type used by iTunes.
/// Note values are not formally defined in any specification.
enum StikType {
    STIK_OLD_MOVIE   = 0,
    STIK_NORMAL      = 1,
    STIK_AUDIOBOOK   = 2,
    STIK_MUSIC_VIDEO = 6,
    STIK_MOVIE       = 9,
    STIK_TV_SHOW     = 10,
    STIK_BOOKLET     = 11,
    STIK_RINGTONE    = 14,

    STIK_UNDEFINED   = 255
};

typedef Enum<StikType,STIK_UNDEFINED> EnumStikType;
MP4V2_EXPORT extern const EnumStikType enumStikType;

///////////////////////////////////////////////////////////////////////////////

/// enumerated 8-bit Account Type used by the iTunes Store.
/// Note values are not formally defined in any specification.
enum AccountType {
    AT_ITUNES    = 0,
    AT_AOL       = 1,

    AT_UNDEFINED = 255
};

typedef Enum<AccountType,AT_UNDEFINED> EnumAccountType;
MP4V2_EXPORT extern const EnumAccountType enumAccountType;

///////////////////////////////////////////////////////////////////////////////

/// enumerated 32-bit Country Code used by the iTunes Store.
/// Note values are not formally defined in any specification.
enum CountryCode {   
    CC_USA   = 143441,
    CC_FRA   = 143442,
    CC_DEU   = 143443,
    CC_GBR   = 143444,
    CC_AUT   = 143445,
    CC_BEL   = 143446,
    CC_FIN   = 143447,
    CC_GRC   = 143448,
    CC_IRL   = 143449,
    CC_ITA   = 143450,
    CC_LUX   = 143451,
    CC_NLD   = 143452,
    CC_PRT   = 143453,
    CC_ESP   = 143454,
    CC_CAN   = 143455,
    CC_SWE   = 143456,
    CC_NOR   = 143457,
    CC_DNK   = 143458,
    CC_CHE   = 143459,
    CC_AUS   = 143460,
    CC_NZL   = 143461,
    CC_JPN   = 143462,

    CC_UNDEFINED = 0
};

typedef Enum<CountryCode,CC_UNDEFINED> EnumCountryCode;
MP4V2_EXPORT extern const EnumCountryCode enumCountryCode;

///////////////////////////////////////////////////////////////////////////////

/// enumerated 8-bit Content Rating used by iTunes.
/// Note values are not formally defined in any specification.
enum ContentRating {   
    CR_NONE      = 0,
    CR_CLEAN     = 2,
    CR_EXPLICIT  = 4,

    CR_UNDEFINED = 255
};

typedef Enum<ContentRating,CR_UNDEFINED> EnumContentRating;
MP4V2_EXPORT extern const EnumContentRating enumContentRating;

///////////////////////////////////////////////////////////////////////////////
/// compute BasicType by examining raw bytes header.
MP4V2_EXPORT BasicType
computeBasicType( const void* buffer, uint32_t size );

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::impl::itmf

#endif // MP4V2_IMPL_ITMF_TYPE_H
