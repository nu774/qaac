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
 * Copyright (C) Cisco Systems Inc. 2001-2005.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 *              Bill May                wmay@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

bool MP4NameFirstMatches(const char* s1, const char* s2)
{
    if (s1 == NULL || *s1 == '\0' || s2 == NULL || *s2 == '\0') {
        return false;
    }

    if (*s2 == '*') {
        return true;
    }

    while (*s1 != '\0') {
        if (*s2 == '\0' || strchr("[.", *s2)) {
            break;
        }
        if (*s1 != *s2) {
            return false;
        }
        s1++;
        s2++;
    }

    // Make sure we finished the loop by using up s2, not s1
    if ( *s2 != '[' && *s2 != '.' && *s2 != '\0' ) {
        return false;
    }

    return true;
}

bool MP4NameFirstIndex(const char* s, uint32_t* pIndex)
{
    if (s == NULL) {
        return false;
    }

    while (*s != '\0' && *s != '.') {
        if (*s == '[') {
            s++;
            ASSERT(pIndex);
            if (sscanf(s, "%u", pIndex) != 1) {
                return false;
            }
            return true;
        }
        s++;
    }
    return false;
}

char* MP4NameFirst(const char *s)
{
    if (s == NULL) {
        return NULL;
    }

    const char* end = s;

    while (*end != '\0' && *end != '.') {
        end++;
    }

    char* first = (char*)MP4Calloc((end - s) + 1);

    if (first) {
        strncpy(first, s, end - s);
    }

    return first;
}

const char* MP4NameAfterFirst(const char *s)
{
    if (s == NULL) {
        return NULL;
    }

    while (*s != '\0') {
        if (*s == '.') {
            s++;
            if (*s == '\0') {
                return NULL;
            }
            return s;
        }
        s++;
    }
    return NULL;
}

char* MP4ToBase16(const uint8_t* pData, uint32_t dataSize)
{
    if (pData == NULL && dataSize != 0) return NULL;

    uint32_t size = 2 * dataSize;
    char* s = (char*)MP4Calloc(size + 1);

    for (uint32_t i = 0; i < dataSize; i++) {
        if (snprintf(&s[2 * i], size - 2 * i, "%02x", pData[i]) != 2) {
            MP4Free(s);
            return NULL;
        }
    }

    return s;   /* N.B. caller is responsible for freeing s */
}

char* MP4ToBase64(const uint8_t* pData, uint32_t dataSize)
{
    if (pData == NULL && dataSize != 0) return NULL;

    static const char encoding[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
    };

    char* s = (char*)MP4Calloc((((dataSize + 2) * 4) / 3) + 1);

    const uint8_t* src = pData;
    char* dest = s;
    uint32_t numGroups = dataSize / 3;

    for (uint32_t i = 0; i < numGroups; i++) {
        *dest++ = encoding[src[0] >> 2];
        *dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
        *dest++ = encoding[((src[1] & 0x0F) << 2) | (src[2] >> 6)];
        *dest++ = encoding[src[2] & 0x3F];
        src += 3;
    }

    if (dataSize % 3 == 1) {
        *dest++ = encoding[src[0] >> 2];
        *dest++ = encoding[((src[0] & 0x03) << 4)];
        *dest++ = '=';
        *dest++ = '=';
    } else if (dataSize % 3 == 2) {
        *dest++ = encoding[src[0] >> 2];
        *dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
        *dest++ = encoding[((src[1] & 0x0F) << 2)];
        *dest++ = '=';
    }
    *dest = '\0';
    return s;   /* N.B. caller is responsible for freeing s */
}

// log2 of value, rounded up
static uint8_t ilog2(uint64_t value)
{
    uint64_t powerOf2 = 1;
    for (uint8_t i = 0; i < 64; i++) {
        if (value <= powerOf2) {
            return i;
        }
        powerOf2 <<= 1;
    }
    return 64;
}

uint64_t MP4ConvertTime(uint64_t t,
                        uint32_t oldTimeScale, uint32_t newTimeScale)
{
    // avoid float point exception
    if (oldTimeScale == 0) {
        throw new EXCEPTION("division by zero");
    }

    if (oldTimeScale == newTimeScale) return t;

    // check if we can safely use integer operations
    if (ilog2(t) + ilog2(newTimeScale) <= 64) {
        return (t * newTimeScale) / oldTimeScale;
    }

    // final resort is to use floating point
    double d = (double)newTimeScale;
    d *= double(t);
    d /= (double)oldTimeScale;
    d += 0.5; // round up.

    return (uint64_t)d;
}

const char* MP4NormalizeTrackType (const char* type)
{
    if (!strcasecmp(type, MP4_VIDEO_TRACK_TYPE)
            || !strcasecmp(type, "video")
            || !strcasecmp(type, "mp4v")
            || !strcasecmp(type, "avc1")
            || !strcasecmp(type, "s263")  // 3GPP H.263
            || !strcasecmp(type, "encv")) {
        return MP4_VIDEO_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_AUDIO_TRACK_TYPE)
            || !strcasecmp(type, "sound")
            || !strcasecmp(type, "audio")
            || !strcasecmp(type, "mp4a")
            || !strcasecmp(type, "ipcm")
            || !strcasecmp(type, "lpcm")
            || !strcasecmp(type, "alaw")
            || !strcasecmp(type, "ulaw")
            || !strcasecmp(type, "samr")  // 3GPP AMR
            || !strcasecmp(type, "sawb")  // 3GPP AMR/WB
            || !strcasecmp(type, "ac-3")
            || !strcasecmp(type, "alac")
            || !strcasecmp(type, "enca")) {
        return MP4_AUDIO_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_SCENE_TRACK_TYPE)
            || !strcasecmp(type, "scene")
            || !strcasecmp(type, "bifs")) {
        return MP4_SCENE_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_OD_TRACK_TYPE)
            || !strcasecmp(type, "od")) {
        return MP4_OD_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_HINT_TRACK_TYPE)
            || !strcasecmp(type, "rtp ")) {
        return MP4_HINT_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_CNTL_TRACK_TYPE)) {
        return MP4_CNTL_TRACK_TYPE;
    }

    if (!strcasecmp(type, MP4_SUBTITLE_TRACK_TYPE)
            || !strcasecmp(type, "tx3g")) {
        return MP4_SUBTITLE_TRACK_TYPE;
    }

    log.verbose1f("Attempt to normalize %s did not match",type);

    return type;
}

MP4Timestamp MP4GetAbsTimestamp() {
    /* MP4 epoch is midnight, January 1, 1904
     * offset from midnight, January 1, 1970 is 2082844800 seconds
     * 208284480 is (((1970 - 1904) * 365) + 17) * 24 * 60 * 60
     */
    return time::getLocalTimeSeconds() + 2082844800;
}

///////////////////////////////////////////////////////////////////////////////

uint32_t STRTOINT32( const char* s )
{
#if defined( MP4V2_INTSTRING_ALIGNMENT )
    // it seems ARM integer instructions require 4-byte alignment so we
    // manually copy string-data into the integer before performing ops
    uint32_t tmp;
    memcpy( &tmp, s, sizeof(tmp) );
    return MP4V2_NTOHL( tmp );
#else
    return MP4V2_NTOHL(*(uint32_t *)s);
#endif
}

void INT32TOSTR( uint32_t i, char* s )
{
#if defined( MP4V2_INTSTRING_ALIGNMENT )
    // it seems ARM integer instructions require 4-byte alignment so we
    // manually copy string-data into the integer before performing ops
    uint32_t tmp = MP4V2_HTONL( i );
    memcpy( s, &tmp, sizeof(tmp) );
#else
    *(uint32_t *)s = MP4V2_HTONL(i);
#endif
    s[4] = 0;
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
