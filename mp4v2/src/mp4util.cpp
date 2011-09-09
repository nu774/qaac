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
        if (tolower(*s1) != tolower(*s2)) {
            return false;
        }
        s1++;
        s2++;
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
    if (dataSize) {
        ASSERT(pData);
    }
    uint32_t size = 2 * dataSize + 1;
    char* s = (char*)MP4Calloc(size);

    uint32_t i, j;
    for (i = 0, j = 0; i < dataSize; i++) {
        size -= snprintf(&s[j], size, "%02x", pData[i]);
        j += 2;
    }

    return s;   /* N.B. caller is responsible for free'ing s */
}

char* MP4ToBase64(const uint8_t* pData, uint32_t dataSize)
{
    if (pData == NULL || dataSize == 0) return NULL;

    static const char encoding[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
    };

    char* s = (char*)MP4Calloc((((dataSize + 2) * 4) / 3) + 1);

    const uint8_t* src = pData;
    if (pData == NULL) return NULL;
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
    return s;   /* N.B. caller is responsible for free'ing s */
}

static bool convertBase64 (const char data, uint8_t *value)
{
    static const uint8_t decodingarr64[128] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
        0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
        0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    uint8_t index = (uint8_t)data;
    if ((index & 0x80) != 0) return false;

    if (decodingarr64[index] == 0xff) return false;
    *value = decodingarr64[index];
    return true;
}

uint8_t *Base64ToBinary (const char *pData, uint32_t decodeSize, uint32_t *pDataSize)
{
    uint8_t *ret;
    uint32_t size, ix, groups;
    if (pData == NULL ||  decodeSize == 0 || pDataSize == NULL)
        return NULL;

    if ((decodeSize % 4) != 0) {
        // must be multiples of 4 characters
        return NULL;
    }
    size = (decodeSize * 3) / 4;
    groups = decodeSize / 4;
    ret = (uint8_t *)MP4Calloc(size);
    if (ret == NULL) return NULL;
    for (ix = 0; ix < groups; ix++) {
        uint8_t value[4];
        for (uint8_t jx = 0; jx < 4; jx++) {
            if (pData[jx] == '=') {
                if (ix != (groups - 1)) {
                    free(ret);
                    return NULL;
                }
                size--;
                value[jx] = 0;
            } else if (convertBase64(pData[jx], &value[jx]) == false) {
                free(ret);
                return NULL;
            }
        }
        ret[(ix * 3)] = value[0] << 2 | ((value[1] >> 4) & 0x3);
        ret[(ix * 3) + 1] = (value[1] << 4) | (value[2] >> 2 & 0xf);
        ret[(ix * 3) + 2] = ((value[2] & 0x3) << 6) | value[3];
        pData += 4;
    }
    *pDataSize = size;
    return ret;
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
        throw new Exception("division by zero", __FILE__, __LINE__, __FUNCTION__ );
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
    if (!strcasecmp(type, "vide")
            || !strcasecmp(type, "video")
            || !strcasecmp(type, "mp4v")
            || !strcasecmp(type, "avc1")
            || !strcasecmp(type, "s263")  // 3GPP H.263
            || !strcasecmp(type, "encv")) {
        return MP4_VIDEO_TRACK_TYPE;
    }

    if (!strcasecmp(type, "soun")
            || !strcasecmp(type, "sound")
            || !strcasecmp(type, "audio")
            || !strcasecmp(type, "enca")
            || !strcasecmp(type, "samr")  // 3GPP AMR
            || !strcasecmp(type, "sawb")  // 3GPP AMR/WB
            || !strcasecmp(type, "mp4a")) {
        return MP4_AUDIO_TRACK_TYPE;
    }

    if (!strcasecmp(type, "sdsm")
            || !strcasecmp(type, "scene")
            || !strcasecmp(type, "bifs")) {
        return MP4_SCENE_TRACK_TYPE;
    }

    if (!strcasecmp(type, "odsm")
            || !strcasecmp(type, "od")) {
        return MP4_OD_TRACK_TYPE;
    }
    if (strcasecmp(type, "cntl") == 0) {
        return MP4_CNTL_TRACK_TYPE;
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
