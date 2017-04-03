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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 */

#ifndef MP4V2_IMPL_MP4UTIL_H
#define MP4V2_IMPL_MP4UTIL_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#define LIBMPV42_STRINGIFY(x)  #x

#ifndef ASSERT
#   define ASSERT(expr) \
        if (!(expr)) { \
            throw new Exception("assert failure: " LIBMPV42_STRINGIFY((expr)), __FILE__, __LINE__, __FUNCTION__ ); \
        }
#endif

#define WARNING(expr) \
    if (expr) { \
        log.errorf("Warning (%s) in %s at line %u", \
                         LIBMPV42_STRINGIFY(expr), __FILE__, __LINE__); \
    }

///////////////////////////////////////////////////////////////////////////////

#define CHECK_AND_FREE(a) if ((a) != NULL) { free((void *)(a)); (a) = NULL;}

#define NUM_ELEMENTS_IN_ARRAY(name) ((sizeof((name))) / (sizeof(*(name))))

///////////////////////////////////////////////////////////////////////////////

inline void* MP4Malloc(size_t size) {
    if (size == 0) return NULL;
    void* p = malloc(size);
    if (p == NULL && size > 0) {
        throw new PlatformException("malloc failed",errno,__FILE__,__LINE__,__FUNCTION__);
    }
    return p;
}

inline void* MP4Calloc(size_t size) {
    if (size == 0) return NULL;
    return memset(MP4Malloc(size), 0, size);
}

inline char* MP4Stralloc(const char* s1) {
    char* s2 = (char*)MP4Malloc(strlen(s1) + 1);
    strcpy(s2, s1);
    return s2;
}

inline void* MP4Realloc(void* p, uint32_t newSize) {
    // workaround library bug
    if (p == NULL && newSize == 0) {
        return NULL;
    }

    void* temp = realloc(p, newSize);
    if (temp == NULL && newSize > 0) {
        throw new PlatformException("malloc failed",errno,__FILE__,__LINE__,__FUNCTION__);
    }
    return temp;
}

uint32_t STRTOINT32( const char* );
void     INT32TOSTR( uint32_t, char* );

MP4Timestamp MP4GetAbsTimestamp();

uint64_t MP4ConvertTime(uint64_t t,
                        uint32_t oldTimeScale, uint32_t newTimeScale);

bool MP4NameFirstMatches(const char* s1, const char* s2);

bool MP4NameFirstIndex(const char* s, uint32_t* pIndex);

char* MP4NameFirst(const char *s);

const char* MP4NameAfterFirst(const char *s);

char* MP4ToBase16(const uint8_t* pData, uint32_t dataSize);

char* MP4ToBase64(const uint8_t* pData, uint32_t dataSize);

const char* MP4NormalizeTrackType(const char* type);

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4UTIL_H
