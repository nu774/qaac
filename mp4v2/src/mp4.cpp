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
 * Copyright (C) Cisco Systems Inc. 2001 - 2005.  All Rights Reserved.
 *
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie         dmackie@cisco.com
 *      Alix Marchandise-Franquet   alix@cisco.com
 *              Ximpo Group Ltd.                mp4v2@ximpo.com
 *              Bill May                        wmay@cisco.com
 */

/*
 * MP4 library API functions
 *
 * These are wrapper functions that provide C linkage conventions
 * to the library, and catch any internal errors, ensuring that
 * a proper return value is given.
 */

#include "src/impl.h"

using namespace mp4v2::impl;

static MP4File  *ConstructMP4File ( void )
{
    MP4File* pFile = NULL;
    try {
        pFile = new MP4File();
    }
    catch( std::bad_alloc ) {
        mp4v2::impl::log.errorf("%s: unable to allocate MP4File", __FUNCTION__);
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: unknown exception constructing MP4File", __FUNCTION__ );
    }

    return pFile;
}

extern "C" {

const char* MP4GetFilename( MP4FileHandle hFile )
{
    if (!MP4_IS_VALID_FILE_HANDLE(hFile))
        return NULL;
    try
    {
        ASSERT(hFile);
        MP4File& file = *static_cast<MP4File*>(hFile);
        ASSERT(file.GetFilename().c_str());
        return file.GetFilename().c_str();
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: unknown exception accessing MP4File "
                                "filename", __FUNCTION__ );
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////

MP4FileHandle MP4Read( const char* fileName )
{
    if (!fileName)
        return MP4_INVALID_FILE_HANDLE;

    MP4File *pFile = ConstructMP4File();
    if (!pFile)
        return MP4_INVALID_FILE_HANDLE;

    try
    {
        ASSERT(pFile);
        pFile->Read( fileName, NULL );
        return (MP4FileHandle)pFile;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: \"%s\": failed", __FUNCTION__,
                                fileName );
    }

    if (pFile)
        delete pFile;
    return MP4_INVALID_FILE_HANDLE;
}

MP4FileHandle MP4ReadProvider( const char* fileName, const MP4FileProvider* fileProvider )
{
    if (!fileName)
        return MP4_INVALID_FILE_HANDLE;

    MP4File *pFile = ConstructMP4File();
    if (!pFile)
        return MP4_INVALID_FILE_HANDLE;

    try {
        pFile->Read( fileName, fileProvider );
        return (MP4FileHandle)pFile;
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf("%s: \"%s\": failed", __FUNCTION__,
                                fileName );
    }

    if (pFile)
        delete pFile;
    return MP4_INVALID_FILE_HANDLE;
}

///////////////////////////////////////////////////////////////////////////////

    MP4FileHandle MP4Create (const char* fileName,
                             uint32_t  flags)
    {
        return MP4CreateEx(fileName, flags);
    }

    MP4FileHandle MP4CreateEx (const char* fileName,
                               uint32_t  flags,
                               int add_ftyp,
                               int add_iods,
                               char* majorBrand,
                               uint32_t minorVersion,
                               char** supportedBrands,
                               uint32_t supportedBrandsCount)
    {
        if (!fileName)
            return MP4_INVALID_FILE_HANDLE;

        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return MP4_INVALID_FILE_HANDLE;

        try {
            ASSERT(pFile);
            // LATER useExtensibleFormat, moov first, then mvex's
            pFile->Create(fileName, flags, add_ftyp, add_iods,
                          majorBrand, minorVersion,
                          supportedBrands, supportedBrandsCount);
            return (MP4FileHandle)pFile;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: \"%s\": failed", __FUNCTION__,
                                    fileName );
        }

        if (pFile)
            delete pFile;
        return MP4_INVALID_FILE_HANDLE;
    }

    MP4FileHandle MP4Modify(const char* fileName,
                            uint32_t flags)
    {
        if (!fileName)
            return MP4_INVALID_FILE_HANDLE;

        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return MP4_INVALID_FILE_HANDLE;

        try {
            ASSERT(pFile);
            // LATER useExtensibleFormat, moov first, then mvex's
            if (pFile->Modify(fileName))
                return (MP4FileHandle)pFile;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: \"%s\": failed", __FUNCTION__,
                                    fileName );
        }

        if (pFile)
            delete pFile;
        return MP4_INVALID_FILE_HANDLE;
    }

    bool MP4Optimize(const char* fileName,
                     const char* newFileName)
    {
        // Must at least have fileName for in-place optimize; newFileName
        // can be null, however.
        if (fileName == NULL)
            return false;

        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return MP4_INVALID_FILE_HANDLE;

        try {
            ASSERT(pFile);
            pFile->Optimize(fileName, newFileName);
            delete pFile;
            return true;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s(%s,%s) failed", __FUNCTION__,
                                    fileName, newFileName );
        }

        if (pFile)
            delete pFile;
        return false;
    }

    void MP4Close(MP4FileHandle hFile, uint32_t  flags)
    {
        if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
            return;

        MP4File& f = *(MP4File*)hFile;
        try {
            f.Close(flags);
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
        }

        delete &f;
    }

    bool MP4Dump(
        MP4FileHandle hFile,
        bool dumpImplicits)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->Dump(dumpImplicits);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4Duration MP4GetDuration(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetDuration();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    uint32_t MP4GetTimeScale(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTimeScale();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4SetTimeScale(MP4FileHandle hFile, uint32_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTimeScale(value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint8_t MP4GetODProfileLevel(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetODProfileLevel();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4SetODProfileLevel(MP4FileHandle hFile, uint8_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetODProfileLevel(value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint8_t MP4GetSceneProfileLevel(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSceneProfileLevel();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4SetSceneProfileLevel(MP4FileHandle hFile, uint8_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetSceneProfileLevel(value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint8_t MP4GetVideoProfileLevel(MP4FileHandle hFile,
                                    MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetVideoProfileLevel();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
            if (MP4_IS_VALID_TRACK_ID(trackId)) {
                uint8_t *foo;
                uint32_t bufsize;
                uint8_t type;
                // for mpeg4 video tracks, try to look for the VOSH header,
                // which has this info.
                type = MP4GetTrackEsdsObjectTypeId(hFile, trackId);
                if (type == MP4_MPEG4_VIDEO_TYPE) {
                    if (MP4GetTrackESConfiguration(hFile,
                                                   trackId,
                                                   &foo,
                                                   &bufsize)) {
                        uint8_t *ptr = foo;
                        while (bufsize > 0) {
                            if (MP4V2_HTONL(*(uint32_t *)ptr) == 0x1b0) {
                                uint8_t ret = ptr[4];
                                free(foo);
                                return ret;
                            }
                            ptr++;
                            bufsize--;
                        }
                        free(foo);
                    }
                }
            }

        }
        return 0;
    }

    void MP4SetVideoProfileLevel(MP4FileHandle hFile, uint8_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetVideoProfileLevel(value);
                return ;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return ;
    }

    uint8_t MP4GetAudioProfileLevel(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetAudioProfileLevel();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    void MP4SetAudioProfileLevel(MP4FileHandle hFile, uint8_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetAudioProfileLevel(value);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    uint8_t MP4GetGraphicsProfileLevel(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetGraphicsProfileLevel();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4SetGraphicsProfileLevel(MP4FileHandle hFile, uint8_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetGraphicsProfileLevel(value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    /* generic file properties */

    bool MP4HaveAtom (MP4FileHandle hFile, const char *atomName)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File *)hFile)->FindAtom(atomName) != NULL;
            } catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetIntegerProperty(
        MP4FileHandle hFile, const char* propName, uint64_t *retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue = ((MP4File*)hFile)->GetIntegerProperty(propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetFloatProperty(
        MP4FileHandle hFile, const char* propName, float *retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue = ((MP4File*)hFile)->GetFloatProperty(propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetStringProperty(
        MP4FileHandle hFile, const char* propName,
        const char **retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue =  ((MP4File*)hFile)->GetStringProperty(propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetBytesProperty(
        MP4FileHandle hFile, const char* propName,
        uint8_t** ppValue, uint32_t* pValueSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetBytesProperty(propName, ppValue, pValueSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *ppValue = NULL;
        *pValueSize = 0;
        return false;
    }

    bool MP4SetIntegerProperty(
        MP4FileHandle hFile, const char* propName, int64_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetIntegerProperty(propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetFloatProperty(
        MP4FileHandle hFile, const char* propName, float value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetFloatProperty(propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetStringProperty(
        MP4FileHandle hFile, const char* propName, const char* value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetStringProperty(propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetBytesProperty(
        MP4FileHandle hFile, const char* propName,
        const uint8_t* pValue, uint32_t valueSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetBytesProperty(propName, pValue, valueSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    /* track operations */

    MP4TrackId MP4AddTrack(
        MP4FileHandle hFile, const char* type,uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddSystemsTrack(type, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddSystemsTrack(
        MP4FileHandle hFile, const char* type)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddSystemsTrack(type);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddODTrack(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddODTrack();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddSceneTrack(MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddSceneTrack();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddULawAudioTrack(
        MP4FileHandle hFile,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       AddULawAudioTrack(timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddALawAudioTrack(
        MP4FileHandle hFile,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       AddALawAudioTrack(timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddAudioTrack(
        MP4FileHandle hFile,
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint8_t audioType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       AddAudioTrack(timeScale, sampleDuration, audioType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

//
// API to initialize ismacryp properties to sensible defaults.
// if the input pointer is null then an ismacryp params is malloc'd.
// caller must see to it that it is properly disposed of.
//
    mp4v2_ismacrypParams *MP4DefaultISMACrypParams(mp4v2_ismacrypParams *ptr)
    {
        try
        {
            if (ptr == NULL) {
                ptr = (mp4v2_ismacrypParams *)MP4Malloc(sizeof(mp4v2_ismacrypParams));
            }
            memset(ptr, 0, sizeof(*ptr));
            return ptr;
        }

        catch (...) {
            return MP4_INVALID_TRACK_ID;
        }
    }


    MP4TrackId MP4AddAC3AudioTrack(
        MP4FileHandle hFile,
        uint32_t samplingRate,
        uint8_t fscod,
        uint8_t bsid,
        uint8_t bsmod,
        uint8_t acmod,
        uint8_t lfeon,
        uint8_t bit_rate_code)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                    AddAC3AudioTrack(samplingRate, fscod, bsid, bsmod, acmod, lfeon, bit_rate_code);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddEncAudioTrack(MP4FileHandle hFile,
                                   uint32_t timeScale,
                                   MP4Duration sampleDuration,
                                   mp4v2_ismacrypParams *icPp,
                                   uint8_t audioType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                if (icPp == NULL) {
                    return ((MP4File*)hFile)->
                           AddEncAudioTrack(timeScale, sampleDuration, audioType,
                                            0, 0,
                                            0, 0,
                                            false, NULL, false);
                } else {
                    return ((MP4File*)hFile)->
                           AddEncAudioTrack(timeScale, sampleDuration, audioType,
                                            icPp->scheme_type, icPp->scheme_version,
                                            icPp->key_ind_len, icPp->iv_len,
                                            icPp->selective_enc, icPp->kms_uri, true);
                }
            } catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }
    MP4TrackId MP4AddAmrAudioTrack(
        MP4FileHandle hFile,
        uint32_t timeScale,
        uint16_t modeSet,
        uint8_t modeChangePeriod,
        uint8_t framesPerSample,
        bool isAmrWB)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       AddAmrAudioTrack(timeScale, modeSet, modeChangePeriod, framesPerSample, isAmrWB);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    void MP4SetAmrVendor(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint32_t vendor)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->
                SetAmrVendor(trackId, vendor);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    void MP4SetAmrDecoderVersion(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint8_t decoderVersion)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->
                SetAmrDecoderVersion(trackId, decoderVersion);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    void MP4SetAmrModeSet(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint16_t modeSet)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->
                SetAmrModeSet(trackId, modeSet);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    uint16_t MP4GetAmrModeSet(
        MP4FileHandle hFile,
        MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       GetAmrModeSet(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    MP4TrackId MP4AddHrefTrack (MP4FileHandle hFile,
                                uint32_t timeScale,
                                MP4Duration sampleDuration,
                                const char *base_url)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4File *pFile = (MP4File *)hFile;

                return pFile->AddHrefTrack(timeScale,
                                           sampleDuration,
                                           base_url);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    const char *MP4GetHrefTrackBaseUrl (MP4FileHandle hFile,
                                        MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackStringProperty(trackId,
                        "mdia.minf.stbl.stsd.href.burl.base_url");
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    MP4TrackId MP4AddVideoTrack(
        MP4FileHandle hFile,
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t videoType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4File *pFile = (MP4File *)hFile;

                return pFile->AddMP4VideoTrack(timeScale,
                                               sampleDuration,
                                               width,
                                               height,
                                               videoType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddEncVideoTrack(MP4FileHandle hFile,
                                   uint32_t timeScale,
                                   MP4Duration sampleDuration,
                                   uint16_t width,
                                   uint16_t height,
                                   mp4v2_ismacrypParams *icPp,
                                   uint8_t videoType,
                                   const char *oFormat)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {

                // test for valid ismacrypt session descriptor
                if (icPp == NULL) {
                    return MP4_INVALID_TRACK_ID;
                }
                MP4File *pFile = (MP4File *)hFile;

                return pFile->AddEncVideoTrack(timeScale,
                                               sampleDuration,
                                               width,
                                               height,
                                               videoType,
                                               icPp,
                                               oFormat);

            } catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddColr(
        MP4FileHandle hFile, MP4TrackId refTrackId, uint16_t pri, uint16_t tran, uint16_t mat)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddColr(refTrackId, pri, tran, mat);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }


    MP4TrackId MP4AddH264VideoTrack(MP4FileHandle hFile,
                                    uint32_t timeScale,
                                    MP4Duration sampleDuration,
                                    uint16_t width,
                                    uint16_t height,
                                    uint8_t AVCProfileIndication,
                                    uint8_t profile_compat,
                                    uint8_t AVCLevelIndication,
                                    uint8_t sampleLenFieldSizeMinusOne)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4File *pFile = (MP4File *)hFile;

                return pFile->AddH264VideoTrack(timeScale,
                                                sampleDuration,
                                                width,
                                                height,
                                                AVCProfileIndication,
                                                profile_compat,
                                                AVCLevelIndication,
                                                sampleLenFieldSizeMinusOne);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddEncH264VideoTrack(
        MP4FileHandle hFile,
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        MP4FileHandle srcFile,
        MP4TrackId srcTrackId,
        mp4v2_ismacrypParams *icPp
    )

    {
        MP4Atom *srcAtom;
        MP4File *pFile;

        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {

                pFile = (MP4File *)srcFile;
                srcAtom = pFile->FindTrackAtom(srcTrackId, "mdia.minf.stbl.stsd.avc1.avcC");
                if (srcAtom == NULL)
                    return MP4_INVALID_TRACK_ID;

                pFile = (MP4File *)hFile;

                return pFile->AddEncH264VideoTrack(timeScale,
                                                   sampleDuration,
                                                   width,
                                                   height,
                                                   srcAtom,
                                                   icPp);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    void MP4AddH264SequenceParameterSet (MP4FileHandle hFile,
                                         MP4TrackId trackId,
                                         const uint8_t *pSequence,
                                         uint16_t sequenceLen)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4File *pFile = (MP4File *)hFile;

                pFile->AddH264SequenceParameterSet(trackId,
                                                   pSequence,
                                                   sequenceLen);
                return;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return;
    }
    void MP4AddH264PictureParameterSet (MP4FileHandle hFile,
                                        MP4TrackId trackId,
                                        const uint8_t *pPict,
                                        uint16_t pictLen)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4File *pFile = (MP4File *)hFile;

                pFile->AddH264PictureParameterSet(trackId,
                                                  pPict,
                                                  pictLen);
                return;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return;
    }

    MP4TrackId MP4AddH263VideoTrack(
        MP4FileHandle hFile,
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t h263Level,
        uint8_t h263Profile,
        uint32_t avgBitrate,
        uint32_t maxBitrate)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       AddH263VideoTrack(timeScale, sampleDuration, width, height, h263Level, h263Profile, avgBitrate, maxBitrate);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }

        return MP4_INVALID_TRACK_ID;
    }

    void MP4SetH263Vendor(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint32_t vendor)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->
                SetH263Vendor(trackId, vendor);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    void MP4SetH263DecoderVersion(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint8_t decoderVersion)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {

            try {
                ((MP4File*)hFile)->
                SetH263DecoderVersion(trackId, decoderVersion);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    void MP4SetH263Bitrates(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint32_t avgBitrate,
        uint32_t maxBitrate)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {

            try {
                ((MP4File*)hFile)->
                SetH263Bitrates(trackId, avgBitrate, maxBitrate);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    MP4TrackId MP4AddHintTrack(
        MP4FileHandle hFile, MP4TrackId refTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddHintTrack(refTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddTextTrack(
        MP4FileHandle hFile, MP4TrackId refTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddTextTrack(refTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddSubtitleTrack(MP4FileHandle hFile,
                                   uint32_t timescale,
                                   uint16_t width,
                                   uint16_t height)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddSubtitleTrack(timescale, width, height);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddSubpicTrack(MP4FileHandle hFile,
                                   uint32_t timescale,
                                   uint16_t width,
                                   uint16_t height)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddSubpicTrack(timescale, width, height);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddChapterTextTrack(
        MP4FileHandle hFile, MP4TrackId refTrackId, uint32_t timescale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddChapterTextTrack(refTrackId, timescale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    MP4TrackId MP4AddPixelAspectRatio(
        MP4FileHandle hFile, MP4TrackId refTrackId, uint32_t hSpacing, uint32_t vSpacing)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->AddPixelAspectRatio(refTrackId, hSpacing, vSpacing);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    void MP4AddChapter(
        MP4FileHandle hFile, MP4TrackId chapterTrackId, MP4Duration chapterDuration, const char *chapterTitle)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddChapter(chapterTrackId, chapterDuration, chapterTitle);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    void MP4AddNeroChapter(
        MP4FileHandle hFile, MP4Timestamp chapterStart, const char *chapterTitle)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddNeroChapter(chapterStart, chapterTitle);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    MP4ChapterType MP4ConvertChapters(
        MP4FileHandle hFile, MP4ChapterType toChapterType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile))
        {
            try {
                return ((MP4File*)hFile)->ConvertChapters(toChapterType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4ChapterTypeNone;
    }

    MP4ChapterType MP4DeleteChapters(
        MP4FileHandle hFile, MP4ChapterType fromChapterType, MP4TrackId chapterTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->DeleteChapters(fromChapterType, chapterTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4ChapterTypeNone;
    }

    MP4ChapterType MP4GetChapters(
        MP4FileHandle hFile, MP4Chapter_t ** chapterList, uint32_t * chapterCount, MP4ChapterType fromChapterType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetChapters(chapterList, chapterCount, fromChapterType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4ChapterTypeNone;
    }

    MP4ChapterType MP4SetChapters(
        MP4FileHandle hFile, MP4Chapter_t * chapterList, uint32_t chapterCount, MP4ChapterType toChapterType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->SetChapters(chapterList, chapterCount, toChapterType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4ChapterTypeNone;
    }

    void MP4ChangeMovieTimeScale(
        MP4FileHandle hFile, uint32_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->ChangeMovieTimeScale(value);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
    }

    MP4TrackId MP4CloneTrack (MP4FileHandle srcFile,
                              MP4TrackId srcTrackId,
                              MP4FileHandle dstFile,
                              MP4TrackId dstHintTrackReferenceTrack)
    {
        MP4TrackId dstTrackId = MP4_INVALID_TRACK_ID;

        if (dstFile == NULL) {
            dstFile = srcFile;
        }

        const char* trackType =
            MP4GetTrackType(srcFile, srcTrackId);

        if (!trackType) {
            return dstTrackId;
        }

        const char *media_data_name =
            MP4GetTrackMediaDataName(srcFile, srcTrackId);
        if (media_data_name == NULL) return dstTrackId;

        if (MP4_IS_VIDEO_TRACK_TYPE(trackType)) {
            if (ATOMID(media_data_name) == ATOMID("mp4v")) {
                MP4SetVideoProfileLevel(dstFile,
                                        MP4GetVideoProfileLevel(srcFile));
                dstTrackId = MP4AddVideoTrack(
                                 dstFile,
                                 MP4GetTrackTimeScale(srcFile,
                                                      srcTrackId),
                                 MP4GetTrackFixedSampleDuration(srcFile,
                                                                srcTrackId),
                                 MP4GetTrackVideoWidth(srcFile,
                                                       srcTrackId),
                                 MP4GetTrackVideoHeight(srcFile,
                                                        srcTrackId),
                                 MP4GetTrackEsdsObjectTypeId(srcFile,
                                                             srcTrackId));
            } else if (ATOMID(media_data_name) == ATOMID("avc1")) {
                uint8_t AVCProfileIndication;
                uint8_t profile_compat;
                uint8_t AVCLevelIndication;
                uint32_t sampleLenFieldSizeMinusOne;
                uint64_t temp;

                if (MP4GetTrackH264ProfileLevel(srcFile, srcTrackId,
                                                &AVCProfileIndication,
                                                &AVCLevelIndication) == false) {
                    return dstTrackId;
                }
                if (MP4GetTrackH264LengthSize(srcFile, srcTrackId,
                                              &sampleLenFieldSizeMinusOne) == false) {
                    return dstTrackId;
                }
                sampleLenFieldSizeMinusOne--;
                if (MP4GetTrackIntegerProperty(srcFile, srcTrackId,
                                               "mdia.minf.stbl.stsd.*[0].avcC.profile_compatibility",
                                               &temp) == false) return dstTrackId;
                profile_compat = temp & 0xff;

                dstTrackId = MP4AddH264VideoTrack(dstFile,
                                                  MP4GetTrackTimeScale(srcFile,
                                                                       srcTrackId),
                                                  MP4GetTrackFixedSampleDuration(srcFile,
                                                                                 srcTrackId),
                                                  MP4GetTrackVideoWidth(srcFile,
                                                                        srcTrackId),
                                                  MP4GetTrackVideoHeight(srcFile,
                                                                         srcTrackId),
                                                  AVCProfileIndication,
                                                  profile_compat,
                                                  AVCLevelIndication,
                                                  sampleLenFieldSizeMinusOne);
                uint8_t **seqheader, **pictheader;
                uint32_t *pictheadersize, *seqheadersize;
                uint32_t ix;
                MP4GetTrackH264SeqPictHeaders(srcFile, srcTrackId,
                                              &seqheader, &seqheadersize,
                                              &pictheader, &pictheadersize);
                for (ix = 0; seqheadersize[ix] != 0; ix++) {
                    MP4AddH264SequenceParameterSet(dstFile, dstTrackId,
                                                   seqheader[ix], seqheadersize[ix]);
                    free(seqheader[ix]);
                }
                free(seqheader);
                free(seqheadersize);
                for (ix = 0; pictheadersize[ix] != 0; ix++) {
                    MP4AddH264PictureParameterSet(dstFile, dstTrackId,
                                                  pictheader[ix], pictheadersize[ix]);
                    free(pictheader[ix]);
                }
                free(pictheader);
                free(pictheadersize);
            } else
                return dstTrackId;
        } else if (MP4_IS_AUDIO_TRACK_TYPE(trackType)) {
            if (ATOMID(media_data_name) != ATOMID("mp4a")) return dstTrackId;
            MP4SetAudioProfileLevel(dstFile,
                                    MP4GetAudioProfileLevel(srcFile));
            dstTrackId = MP4AddAudioTrack(
                             dstFile,
                             MP4GetTrackTimeScale(srcFile, srcTrackId),
                             MP4GetTrackFixedSampleDuration(srcFile, srcTrackId),
                             MP4GetTrackEsdsObjectTypeId(srcFile, srcTrackId));

        } else if (MP4_IS_OD_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddODTrack(dstFile);

        } else if (MP4_IS_SCENE_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddSceneTrack(dstFile);

        } else if (MP4_IS_HINT_TRACK_TYPE(trackType)) {
            if (dstHintTrackReferenceTrack == MP4_INVALID_TRACK_ID) {
                dstTrackId = MP4_INVALID_TRACK_ID;
            } else {
                dstTrackId = MP4AddHintTrack(
                                 dstFile,
                                 dstHintTrackReferenceTrack);
            }

        } else if (MP4_IS_SYSTEMS_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddSystemsTrack(dstFile, trackType);

        } else {
            dstTrackId = MP4AddTrack(dstFile, trackType);
        }

        if (dstTrackId == MP4_INVALID_TRACK_ID) {
            return dstTrackId;
        }

        MP4SetTrackTimeScale(
            dstFile,
            dstTrackId,
            MP4GetTrackTimeScale(srcFile, srcTrackId));

        if (MP4_IS_AUDIO_TRACK_TYPE(trackType)
                || MP4_IS_VIDEO_TRACK_TYPE(trackType)) {
            // copy track ES configuration
            uint8_t* pConfig = NULL;
            uint32_t configSize = 0;
            MP4LogLevel verb = mp4v2::impl::log.verbosity;
            mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
            bool haveEs = MP4GetTrackESConfiguration(srcFile,
                          srcTrackId,
                          &pConfig,
                          &configSize);
            mp4v2::impl::log.setVerbosity(verb);
            if (haveEs &&
                    pConfig != NULL && configSize != 0) {
                if (!MP4SetTrackESConfiguration(
                            dstFile,
                            dstTrackId,
                            pConfig,
                            configSize)) {
                    free(pConfig);
                    MP4DeleteTrack(dstFile, dstTrackId);
                    return MP4_INVALID_TRACK_ID;
                }

                free(pConfig);
            }
        }

        if (MP4_IS_HINT_TRACK_TYPE(trackType)) {
            // probably not exactly what is wanted
            // but caller can adjust later to fit their desires

            char* payloadName = NULL;
            char *encodingParms = NULL;
            uint8_t payloadNumber;
            uint16_t maxPayloadSize;

            if (MP4GetHintTrackRtpPayload(
                        srcFile,
                        srcTrackId,
                        &payloadName,
                        &payloadNumber,
                        &maxPayloadSize,
                        &encodingParms)) {

                if (MP4SetHintTrackRtpPayload(
                            dstFile,
                            dstTrackId,
                            payloadName,
                            &payloadNumber,
                            maxPayloadSize,
                            encodingParms) == false) {
                    MP4DeleteTrack(dstFile, dstTrackId);
                    return MP4_INVALID_TRACK_ID;
                }
            }
#if 0
            MP4SetHintTrackSdp(
                dstFile,
                dstTrackId,
                MP4GetHintTrackSdp(srcFile, srcTrackId));
#endif
        }

        return dstTrackId;
    }

// Given a track, make an encrypted clone of it in the dest. file
    MP4TrackId MP4EncAndCloneTrack(MP4FileHandle srcFile,
                                   MP4TrackId srcTrackId,
                                   mp4v2_ismacrypParams *icPp,
                                   MP4FileHandle dstFile,
                                   MP4TrackId dstHintTrackReferenceTrack
                                  )
    {
        const char *oFormat;

        MP4TrackId dstTrackId = MP4_INVALID_TRACK_ID;

        if (dstFile == NULL) {
            dstFile = srcFile;
        }

        const char* trackType = MP4GetTrackType(srcFile, srcTrackId);

        if (!trackType) {
            return dstTrackId;
        }

        if (MP4_IS_VIDEO_TRACK_TYPE(trackType)) {

            // test source file format for avc1
            oFormat = MP4GetTrackMediaDataName(srcFile, srcTrackId);
            if (!strcasecmp(oFormat, "avc1"))
            {
                dstTrackId = MP4AddEncH264VideoTrack(dstFile,
                                                     MP4GetTrackTimeScale(srcFile, srcTrackId),
                                                     MP4GetTrackFixedSampleDuration(srcFile, srcTrackId),
                                                     MP4GetTrackVideoWidth(srcFile, srcTrackId),
                                                     MP4GetTrackVideoHeight(srcFile, srcTrackId),
                                                     srcFile,
                                                     srcTrackId,
                                                     icPp
                                                    );
            }
            else
            {
                MP4SetVideoProfileLevel(dstFile, MP4GetVideoProfileLevel(srcFile));
                dstTrackId = MP4AddEncVideoTrack(dstFile,
                                                 MP4GetTrackTimeScale(srcFile, srcTrackId),
                                                 MP4GetTrackFixedSampleDuration(srcFile, srcTrackId),
                                                 MP4GetTrackVideoWidth(srcFile, srcTrackId),
                                                 MP4GetTrackVideoHeight(srcFile, srcTrackId),
                                                 icPp,
                                                 MP4GetTrackEsdsObjectTypeId(srcFile, srcTrackId),
                                                 oFormat
                                                );
            }

        } else if (MP4_IS_AUDIO_TRACK_TYPE(trackType)) {
            MP4SetAudioProfileLevel(dstFile, MP4GetAudioProfileLevel(srcFile));
            dstTrackId = MP4AddEncAudioTrack(dstFile,
                                             MP4GetTrackTimeScale(srcFile, srcTrackId),
                                             MP4GetTrackFixedSampleDuration(srcFile,
                                                                            srcTrackId),
                                             icPp,
                                             MP4GetTrackEsdsObjectTypeId(srcFile,
                                                                         srcTrackId)
                                            );

        } else if (MP4_IS_OD_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddODTrack(dstFile);

        } else if (MP4_IS_SCENE_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddSceneTrack(dstFile);

        } else if (MP4_IS_HINT_TRACK_TYPE(trackType)) {
            if (dstHintTrackReferenceTrack == MP4_INVALID_TRACK_ID) {
                dstTrackId = MP4_INVALID_TRACK_ID;
            } else {
                dstTrackId = MP4AddHintTrack(dstFile,
                                             MP4GetHintTrackReferenceTrackId(srcFile,
                                                                             srcTrackId));
            }
        } else if (MP4_IS_SYSTEMS_TRACK_TYPE(trackType)) {
            dstTrackId = MP4AddSystemsTrack(dstFile, trackType);

        } else {
            dstTrackId = MP4AddTrack(dstFile, trackType);
        }

        if (dstTrackId == MP4_INVALID_TRACK_ID) {
            return dstTrackId;
        }

        MP4SetTrackTimeScale(dstFile,
                             dstTrackId,
                             MP4GetTrackTimeScale(srcFile, srcTrackId));

        if (MP4_IS_AUDIO_TRACK_TYPE(trackType)
                || MP4_IS_VIDEO_TRACK_TYPE(trackType)) {
            // copy track ES configuration
            uint8_t* pConfig = NULL;
            uint32_t configSize = 0;
            if (MP4GetTrackESConfiguration(srcFile, srcTrackId,
                                           &pConfig, &configSize)) {

                if (pConfig != NULL) {
                    MP4SetTrackESConfiguration(dstFile, dstTrackId,
                                               pConfig, configSize);
                }
            }
            if (pConfig != NULL)
                free(pConfig);
        }

        // Bill's change to MP4CloneTrack
        if (MP4_IS_HINT_TRACK_TYPE(trackType)) {
            // probably not exactly what is wanted
            // but caller can adjust later to fit their desires

            char* payloadName = NULL;
            char *encodingParms = NULL;
            uint8_t payloadNumber;
            uint16_t maxPayloadSize;

            if (MP4GetHintTrackRtpPayload(
                        srcFile,
                        srcTrackId,
                        &payloadName,
                        &payloadNumber,
                        &maxPayloadSize,
                        &encodingParms)) {

                (void)MP4SetHintTrackRtpPayload(
                    dstFile,
                    dstTrackId,
                    payloadName,
                    &payloadNumber,
                    maxPayloadSize,
                    encodingParms);
            }
#if 0
            MP4SetHintTrackSdp(
                dstFile,
                dstTrackId,
                MP4GetHintTrackSdp(srcFile, srcTrackId));
#endif
        }

        return dstTrackId;
    }

    MP4TrackId MP4CopyTrack(MP4FileHandle srcFile,
                            MP4TrackId srcTrackId,
                            MP4FileHandle dstFile,
                            bool applyEdits,
                            MP4TrackId dstHintTrackReferenceTrack)
    {
        bool copySamples = true;  // LATER allow false => reference samples

        MP4TrackId dstTrackId =
            MP4CloneTrack(srcFile, srcTrackId, dstFile, dstHintTrackReferenceTrack);

        if (dstTrackId == MP4_INVALID_TRACK_ID) {
            return dstTrackId;
        }

        bool viaEdits =
            applyEdits && MP4GetTrackNumberOfEdits(srcFile, srcTrackId);

        MP4SampleId sampleId = 0;
        MP4SampleId numSamples =
            MP4GetTrackNumberOfSamples(srcFile, srcTrackId);

        MP4Timestamp when = 0;
        MP4Duration editsDuration =
            MP4GetTrackEditTotalDuration(srcFile, srcTrackId);

        while (true) {
            MP4Duration sampleDuration = MP4_INVALID_DURATION;

            if (viaEdits) {
                sampleId = MP4GetSampleIdFromEditTime(
                               srcFile,
                               srcTrackId,
                               when,
                               NULL,
                               &sampleDuration);

                // in theory, this shouldn't happen
                if (sampleId == MP4_INVALID_SAMPLE_ID) {
                    MP4DeleteTrack(dstFile, dstTrackId);
                    return MP4_INVALID_TRACK_ID;
                }

                when += sampleDuration;

                if (when >= editsDuration) {
                    break;
                }
            } else {
                sampleId++;
                if (sampleId > numSamples) {
                    break;
                }
            }

            bool rc = false;

            if (copySamples) {
                rc = MP4CopySample(
                         srcFile,
                         srcTrackId,
                         sampleId,
                         dstFile,
                         dstTrackId,
                         sampleDuration);

            } else {
                rc = MP4ReferenceSample(
                         srcFile,
                         srcTrackId,
                         sampleId,
                         dstFile,
                         dstTrackId,
                         sampleDuration);
            }

            if (!rc) {
                MP4DeleteTrack(dstFile, dstTrackId);
                return MP4_INVALID_TRACK_ID;
            }
        }

        return dstTrackId;
    }

// Given a source track in a source file, make an encrypted copy of
// the track in the destination file, including sample encryption
    MP4TrackId MP4EncAndCopyTrack(MP4FileHandle srcFile,
                                  MP4TrackId srcTrackId,
                                  mp4v2_ismacrypParams *icPp,
                                  encryptFunc_t encfcnp,
                                  uint32_t encfcnparam1,
                                  MP4FileHandle dstFile,
                                  bool applyEdits,
                                  MP4TrackId dstHintTrackReferenceTrack
                                 )
    {
        bool copySamples = true;  // LATER allow false => reference samples

        MP4TrackId dstTrackId =
            MP4EncAndCloneTrack(srcFile, srcTrackId,
                                icPp,
                                dstFile, dstHintTrackReferenceTrack);

        if (dstTrackId == MP4_INVALID_TRACK_ID) {
            return dstTrackId;
        }

        bool viaEdits =
            applyEdits && MP4GetTrackNumberOfEdits(srcFile, srcTrackId);

        MP4SampleId sampleId = 0;
        MP4SampleId numSamples =
            MP4GetTrackNumberOfSamples(srcFile, srcTrackId);

        MP4Timestamp when = 0;
        MP4Duration editsDuration =
            MP4GetTrackEditTotalDuration(srcFile, srcTrackId);

        while (true) {
            MP4Duration sampleDuration = MP4_INVALID_DURATION;

            if (viaEdits) {
                sampleId = MP4GetSampleIdFromEditTime(srcFile,
                                                      srcTrackId,
                                                      when,
                                                      NULL,
                                                      &sampleDuration);

                // in theory, this shouldn't happen
                if (sampleId == MP4_INVALID_SAMPLE_ID) {
                    MP4DeleteTrack(dstFile, dstTrackId);
                    return MP4_INVALID_TRACK_ID;
                }

                when += sampleDuration;

                if (when >= editsDuration) {
                    break;
                }
            } else {
                sampleId++;
                if (sampleId > numSamples) {
                    break;
                }
            }

            bool rc = false;

            if (copySamples) {
                // encrypt and copy
                rc = MP4EncAndCopySample(srcFile,
                                         srcTrackId,
                                         sampleId,
                                         encfcnp,
                                         encfcnparam1,
                                         dstFile,
                                         dstTrackId,
                                         sampleDuration);

            } else {
                // not sure what these are - encrypt?
                rc = MP4ReferenceSample(srcFile,
                                        srcTrackId,
                                        sampleId,
                                        dstFile,
                                        dstTrackId,
                                        sampleDuration);
            }

            if (!rc) {
                MP4DeleteTrack(dstFile, dstTrackId);
                return MP4_INVALID_TRACK_ID;
            }
        }

        return dstTrackId;
    }

    bool MP4DeleteTrack(
        MP4FileHandle hFile,
        MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->DeleteTrack(trackId);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint32_t MP4GetNumberOfTracks(
        MP4FileHandle hFile,
        const char* type,
        uint8_t subType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetNumberOfTracks(type, subType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    MP4TrackId MP4FindTrackId(
        MP4FileHandle hFile,
        uint16_t index,
        const char* type,
        uint8_t subType)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->FindTrackId(index, type, subType);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    uint16_t MP4FindTrackIndex(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->FindTrackIndex(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return (uint16_t)-1;
    }

    /* specific track properties */

    const char* MP4GetTrackType(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackType(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }
    const char* MP4GetTrackMediaDataName(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackMediaDataName(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    bool MP4GetTrackMediaDataOriginalFormat(
        MP4FileHandle hFile, MP4TrackId trackId, char *originalFormat,
        uint32_t buflen)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {

                return ((MP4File*)hFile)->GetTrackMediaDataOriginalFormat(trackId,
                        originalFormat, buflen);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4Duration MP4GetTrackDuration(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackDuration(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    uint32_t MP4GetTrackTimeScale(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackTimeScale(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4SetTrackTimeScale(
        MP4FileHandle hFile, MP4TrackId trackId, uint32_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackTimeScale(trackId, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint8_t MP4GetTrackAudioMpeg4Type(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackAudioMpeg4Type(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_MPEG4_INVALID_AUDIO_TYPE;
    }



// Replacement to MP4GetTrackVideoType and MP4GetTrackAudioType
// Basically does the same thing but with a more self-explanatory name
    uint8_t MP4GetTrackEsdsObjectTypeId(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {

                return ((MP4File*)hFile)->GetTrackEsdsObjectTypeId(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_AUDIO_TYPE;
    }

    MP4Duration MP4GetTrackFixedSampleDuration(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackFixedSampleDuration(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    uint32_t MP4GetTrackBitRate(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            MP4File *pFile = (MP4File *)hFile;
            try {
                return pFile->GetTrackIntegerProperty(trackId,
                                                      "mdia.minf.stbl.stsd.*.esds.decConfigDescr.avgBitrate");
            }
            catch( Exception* x ) {
                //mp4v2::impl::log.errorf(*x);  we don't really need to print this.
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
            // if we're here, we can't get the bitrate from above -
            // lets calculate it
            try {
                MP4Duration trackDur;
                trackDur = MP4GetTrackDuration(hFile, trackId);
                uint64_t msDuration =
                    pFile->ConvertFromTrackDuration(trackId, trackDur,
                                                    MP4_MSECS_TIME_SCALE);
                if (msDuration == 0) return 0;

                MP4Track *pTrack = pFile->GetTrack(trackId);
                uint64_t bytes = pTrack->GetTotalOfSampleSizes();
                bytes *= UINT64_C(8000);	// 8 * 1000
                bytes /= msDuration;
                return (uint32_t)bytes;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4GetTrackESConfiguration(
        MP4FileHandle hFile, MP4TrackId trackId,
        uint8_t** ppConfig, uint32_t* pConfigSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetTrackESConfiguration(
                    trackId, ppConfig, pConfigSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *ppConfig = NULL;
        *pConfigSize = 0;
        return false;
    }
    bool MP4GetTrackVideoMetadata(
        MP4FileHandle hFile, MP4TrackId trackId,
        uint8_t** ppConfig, uint32_t* pConfigSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetTrackVideoMetadata(
                    trackId, ppConfig, pConfigSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *ppConfig = NULL;
        *pConfigSize = 0;
        return false;
    }

    bool MP4SetTrackESConfiguration(
        MP4FileHandle hFile, MP4TrackId trackId,
        const uint8_t* pConfig, uint32_t configSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackESConfiguration(
                    trackId, pConfig, configSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackH264ProfileLevel (MP4FileHandle hFile,
                                      MP4TrackId trackId,
                                      uint8_t *pProfile,
                                      uint8_t *pLevel)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *pProfile =
                    ((MP4File *)hFile)->GetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*[0].avcC.AVCProfileIndication");
                *pLevel =
                    ((MP4File *)hFile)->GetTrackIntegerProperty(trackId,
                            "mdia.minf.stbl.stsd.*[0].avcC.AVCLevelIndication");

                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackH264SeqPictHeaders (MP4FileHandle hFile,
                                        MP4TrackId trackId,
                                        uint8_t ***pSeqHeader,
                                        uint32_t **pSeqHeaderSize,
                                        uint8_t ***pPictHeader,
                                        uint32_t **pPictHeaderSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetTrackH264SeqPictHeaders(trackId,
                        pSeqHeader,
                        pSeqHeaderSize,
                        pPictHeader,
                        pPictHeaderSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }
    bool MP4GetTrackH264LengthSize (MP4FileHandle hFile,
                                    MP4TrackId trackId,
                                    uint32_t *pLength)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *pLength = 1 +
                           ((MP4File*) hFile)->GetTrackIntegerProperty(trackId,
                                   "mdia.minf.stbl.stsd.*[0].avcC.lengthSizeMinusOne");
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4SampleId MP4GetTrackNumberOfSamples(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackNumberOfSamples(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    uint16_t MP4GetTrackVideoWidth(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackIntegerProperty(trackId,
                        "mdia.minf.stbl.stsd.*.width");
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    uint16_t MP4GetTrackVideoHeight(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackIntegerProperty(trackId,
                        "mdia.minf.stbl.stsd.*.height");
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    double MP4GetTrackVideoFrameRate(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackVideoFrameRate(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0.0;
    }

    int MP4GetTrackAudioChannels (MP4FileHandle hFile,
                                  MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackAudioChannels(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return -1;
    }

// returns true if the track is a media track encrypted according to ismacryp
    bool MP4IsIsmaCrypMediaTrack(
        MP4FileHandle hFile, MP4TrackId trackId)
    {
        bool retval = false;
        MP4LogLevel verb = mp4v2::impl::log.verbosity;
        mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);

        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                retval = ((MP4File*)hFile)->IsIsmaCrypMediaTrack(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        mp4v2::impl::log.setVerbosity(verb);
        return retval;
    }


    /* generic track properties */

    bool MP4HaveTrackAtom (MP4FileHandle hFile,
                           MP4TrackId trackId,
                           const char *atomName)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->FindTrackAtom(trackId, atomName) != NULL;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackIntegerProperty (
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName,
        uint64_t *retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue = ((MP4File*)hFile)->GetTrackIntegerProperty(trackId,
                            propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackFloatProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName,
        float *retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue = ((MP4File*)hFile)->GetTrackFloatProperty(trackId, propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackStringProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName,
        const char **retvalue)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                *retvalue = ((MP4File*)hFile)->GetTrackStringProperty(trackId, propName);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4GetTrackBytesProperty(
        MP4FileHandle hFile, MP4TrackId trackId, const char* propName,
        uint8_t** ppValue, uint32_t* pValueSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetTrackBytesProperty(
                    trackId, propName, ppValue, pValueSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *ppValue = NULL;
        *pValueSize = 0;
        return false;
    }

    bool MP4SetTrackIntegerProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName, int64_t value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackIntegerProperty(trackId,
                        propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetTrackFloatProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName, float value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackFloatProperty(trackId, propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetTrackStringProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName, const char* value)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackStringProperty(trackId, propName, value);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetTrackBytesProperty(
        MP4FileHandle hFile, MP4TrackId trackId,
        const char* propName, const uint8_t* pValue, uint32_t valueSize)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackBytesProperty(
                    trackId, propName, pValue, valueSize);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    /* sample operations */

    bool MP4ReadSample(
        /* input parameters */
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId,
        /* output parameters */
        uint8_t** ppBytes,
        uint32_t* pNumBytes,
        MP4Timestamp* pStartTime,
        MP4Duration* pDuration,
        MP4Duration* pRenderingOffset,
        bool* pIsSyncSample)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->ReadSample(
                    trackId,
                    sampleId,
                    ppBytes,
                    pNumBytes,
                    pStartTime,
                    pDuration,
                    pRenderingOffset,
                    pIsSyncSample);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *pNumBytes = 0;
        return false;
    }

    bool MP4ReadSampleFromTime(
        /* input parameters */
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Timestamp when,
        /* output parameters */
        uint8_t** ppBytes,
        uint32_t* pNumBytes,
        MP4Timestamp* pStartTime,
        MP4Duration* pDuration,
        MP4Duration* pRenderingOffset,
        bool* pIsSyncSample)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4SampleId sampleId =
                    ((MP4File*)hFile)->GetSampleIdFromTime(
                        trackId, when, false);

                ((MP4File*)hFile)->ReadSample(
                    trackId,
                    sampleId,
                    ppBytes,
                    pNumBytes,
                    pStartTime,
                    pDuration,
                    pRenderingOffset,
                    pIsSyncSample);

                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        *pNumBytes = 0;
        return false;
    }

    bool MP4WriteSample(
        MP4FileHandle  hFile,
        MP4TrackId     trackId,
        const uint8_t* pBytes,
        uint32_t       numBytes,
        MP4Duration    duration,
        MP4Duration    renderingOffset,
        bool           isSyncSample )
    {
        if( MP4_IS_VALID_FILE_HANDLE( hFile )) {
            try {
                ((MP4File*)hFile)->WriteSample(
                    trackId,
                    pBytes,
                    numBytes,
                    duration,
                    renderingOffset,
                    isSyncSample );
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4WriteSampleDependency(
        MP4FileHandle  hFile,
        MP4TrackId     trackId,
        const uint8_t* pBytes,
        uint32_t       numBytes,
        MP4Duration    duration,
        MP4Duration    renderingOffset,
        bool           isSyncSample,
        uint32_t       dependencyFlags )
    {
        if( MP4_IS_VALID_FILE_HANDLE( hFile )) {
            try {
                ((MP4File*)hFile)->WriteSampleDependency(
                    trackId,
                    pBytes,
                    numBytes,
                    duration,
                    renderingOffset,
                    isSyncSample,
                    dependencyFlags );
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4CopySample(
        MP4FileHandle srcFile,
        MP4TrackId    srcTrackId,
        MP4SampleId   srcSampleId,
        MP4FileHandle dstFile,
        MP4TrackId    dstTrackId,
        MP4Duration   dstSampleDuration )
    {
        if( !MP4_IS_VALID_FILE_HANDLE( srcFile ))
            return false;

        try {
            MP4File::CopySample(
                (MP4File*)srcFile,
                srcTrackId,
                srcSampleId,
                (MP4File*)dstFile,
                dstTrackId,
                dstSampleDuration );
            return true;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
        }

        return false;
    }

    bool MP4EncAndCopySample(
        MP4FileHandle srcFile,
        MP4TrackId    srcTrackId,
        MP4SampleId   srcSampleId,
        encryptFunc_t encfcnp,
        uint32_t      encfcnparam1,
        MP4FileHandle dstFile,
        MP4TrackId    dstTrackId,
        MP4Duration   dstSampleDuration)
    {
        if( !MP4_IS_VALID_FILE_HANDLE( srcFile ))
            return false;

        try {
            MP4File::EncAndCopySample(
                (MP4File*)srcFile,
                srcTrackId,
                srcSampleId,
                encfcnp,
                encfcnparam1,
                (MP4File*)dstFile,
                dstTrackId,
                dstSampleDuration );
            return true;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
        }

        return false;
    }

    bool MP4ReferenceSample(
        MP4FileHandle srcFile,
        MP4TrackId srcTrackId,
        MP4SampleId srcSampleId,
        MP4FileHandle dstFile,
        MP4TrackId dstTrackId,
        MP4Duration dstSampleDuration)
    {
        // LATER Not yet implemented
        return false;
    }

    uint32_t MP4GetSampleSize(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleSize(
                           trackId, sampleId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    uint32_t MP4GetTrackMaxSampleSize(
        MP4FileHandle hFile,
        MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackMaxSampleSize(trackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    MP4SampleId MP4GetSampleIdFromTime(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Timestamp when,
        bool wantSyncSample)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleIdFromTime(
                           trackId, when, wantSyncSample);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_SAMPLE_ID;
    }

    MP4Timestamp MP4GetSampleTime(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleTime(
                           trackId, sampleId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TIMESTAMP;
    }

    MP4Duration MP4GetSampleDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleDuration(
                           trackId, sampleId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    MP4Duration MP4GetSampleRenderingOffset(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleRenderingOffset(
                           trackId, sampleId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    bool MP4SetSampleRenderingOffset(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId,
        MP4Duration renderingOffset)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetSampleRenderingOffset(
                    trackId, sampleId, renderingOffset);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    int8_t MP4GetSampleSync(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4SampleId sampleId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleSync(
                           trackId, sampleId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return -1;
    }


    uint64_t MP4ConvertFromMovieDuration(
        MP4FileHandle hFile,
        MP4Duration duration,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->ConvertFromMovieDuration(
                           duration, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return (uint64_t)MP4_INVALID_DURATION;
    }

    uint64_t MP4ConvertFromTrackTimestamp(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Timestamp timeStamp,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->ConvertFromTrackTimestamp(
                           trackId, timeStamp, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return (uint64_t)MP4_INVALID_TIMESTAMP;
    }

    MP4Timestamp MP4ConvertToTrackTimestamp(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint64_t timeStamp,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->ConvertToTrackTimestamp(
                           trackId, timeStamp, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TIMESTAMP;
    }

    uint64_t MP4ConvertFromTrackDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Duration duration,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->ConvertFromTrackDuration(
                           trackId, duration, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return (uint64_t)MP4_INVALID_DURATION;
    }

    MP4Duration MP4ConvertToTrackDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        uint64_t duration,
        uint32_t timeScale)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->ConvertToTrackDuration(
                           trackId, duration, timeScale);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    bool MP4GetHintTrackRtpPayload(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        char** ppPayloadName,
        uint8_t* pPayloadNumber,
        uint16_t* pMaxPayloadSize,
        char **ppEncodingParams)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->GetHintTrackRtpPayload(
                    hintTrackId, ppPayloadName, pPayloadNumber, pMaxPayloadSize,
                    ppEncodingParams);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4SetHintTrackRtpPayload(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        const char* pPayloadName,
        uint8_t* pPayloadNumber,
        uint16_t maxPayloadSize,
        const char *encode_params,
        bool include_rtp_map,
        bool include_mpeg4_esid)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetHintTrackRtpPayload(
                    hintTrackId, pPayloadName, pPayloadNumber, maxPayloadSize, encode_params,
                    include_rtp_map, include_mpeg4_esid);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    const char* MP4GetSessionSdp(
        MP4FileHandle hFile)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSessionSdp();
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    bool MP4SetSessionSdp(
        MP4FileHandle hFile,
        const char* sdpString)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetSessionSdp(sdpString);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AppendSessionSdp(
        MP4FileHandle hFile,
        const char* sdpString)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AppendSessionSdp(sdpString);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    const char* MP4GetHintTrackSdp(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetHintTrackSdp(hintTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    bool MP4SetHintTrackSdp(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        const char* sdpString)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetHintTrackSdp(hintTrackId, sdpString);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AppendHintTrackSdp(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        const char* sdpString)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AppendHintTrackSdp(hintTrackId, sdpString);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4TrackId MP4GetHintTrackReferenceTrackId(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       GetHintTrackReferenceTrackId(hintTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TRACK_ID;
    }

    bool MP4ReadRtpHint(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        MP4SampleId hintSampleId,
        uint16_t* pNumPackets)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->ReadRtpHint(
                    hintTrackId, hintSampleId, pNumPackets);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint16_t MP4GetRtpHintNumberOfPackets(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetRtpHintNumberOfPackets(hintTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    int8_t MP4GetRtpPacketBFrame(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        uint16_t packetIndex)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       GetRtpPacketBFrame(hintTrackId, packetIndex);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return -1;
    }

    int32_t MP4GetRtpPacketTransmitOffset(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        uint16_t packetIndex)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->
                       GetRtpPacketTransmitOffset(hintTrackId, packetIndex);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    bool MP4ReadRtpPacket(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        uint16_t packetIndex,
        uint8_t** ppBytes,
        uint32_t* pNumBytes,
        uint32_t ssrc,
        bool includeHeader,
        bool includePayload)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->ReadRtpPacket(
                    hintTrackId, packetIndex,
                    ppBytes, pNumBytes,
                    ssrc, includeHeader, includePayload);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4Timestamp MP4GetRtpTimestampStart(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetRtpTimestampStart(hintTrackId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TIMESTAMP;
    }

    bool MP4SetRtpTimestampStart(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        MP4Timestamp rtpStart)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetRtpTimestampStart(
                    hintTrackId, rtpStart);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AddRtpHint(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        return MP4AddRtpVideoHint(hFile, hintTrackId, false, 0);
    }

    bool MP4AddRtpVideoHint(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        bool isBframe,
        uint32_t timestampOffset)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddRtpHint(hintTrackId,
                                              isBframe, timestampOffset);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AddRtpPacket(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        bool setMbit,
        int32_t transmitOffset)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddRtpPacket(
                    hintTrackId, setMbit, transmitOffset);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AddRtpImmediateData(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        const uint8_t* pBytes,
        uint32_t numBytes)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddRtpImmediateData(hintTrackId,
                                                       pBytes, numBytes);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AddRtpSampleData(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        MP4SampleId sampleId,
        uint32_t dataOffset,
        uint32_t dataLength)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddRtpSampleData(
                    hintTrackId, sampleId, dataOffset, dataLength);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4AddRtpESConfigurationPacket(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->AddRtpESConfigurationPacket(hintTrackId);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4WriteRtpHint(
        MP4FileHandle hFile,
        MP4TrackId hintTrackId,
        MP4Duration duration,
        bool isSyncSample)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->WriteRtpHint(
                    hintTrackId, duration, isSyncSample);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }
    /* 3GPP specific operations */

    bool MP4Make3GPCompliant(
        const char* fileName,
        char* majorBrand,
        uint32_t minorVersion,
        char** supportedBrands,
        uint32_t supportedBrandsCount,
        bool deleteIodsAtom)
    {
        if (!fileName)
            return false;

        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return MP4_INVALID_FILE_HANDLE;

        try {
            ASSERT(pFile);
            pFile->Modify(fileName);
            pFile->Make3GPCompliant(fileName, majorBrand, minorVersion, supportedBrands, supportedBrandsCount, deleteIodsAtom);
            pFile->Close();
            delete pFile;
            return true;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
        }

        if (pFile)
            delete pFile;
        return false;
    }

    /* ISMA specific operations */

    bool MP4MakeIsmaCompliant(
        const char* fileName,
        bool addIsmaComplianceSdp)
    {
        if (!fileName)
            return false;

        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return MP4_INVALID_FILE_HANDLE;

        try {
            ASSERT(pFile);
            pFile->Modify(fileName);
            pFile->MakeIsmaCompliant(addIsmaComplianceSdp);
            pFile->Close();
            delete pFile;
            return true;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: \"%s\": failed", __FUNCTION__,
                                    fileName );
        }

        if (pFile)
            delete pFile;
        return false;
    }

    char* MP4MakeIsmaSdpIod(
        uint8_t videoProfile,
        uint32_t videoBitrate,
        uint8_t* videoConfig,
        uint32_t videoConfigLength,
        uint8_t audioProfile,
        uint32_t audioBitrate,
        uint8_t* audioConfig,
        uint32_t audioConfigLength)

    {
        MP4File* pFile = ConstructMP4File();
        if (!pFile)
            return NULL;

        try {
            uint8_t* pBytes = NULL;
            uint64_t numBytes = 0;

            ASSERT(pFile);
            pFile->CreateIsmaIodFromParams(
                videoProfile,
                videoBitrate,
                videoConfig,
                videoConfigLength,
                audioProfile,
                audioBitrate,
                audioConfig,
                audioConfigLength,
                &pBytes,
                &numBytes);

            char* iodBase64 =
                MP4ToBase64(pBytes, numBytes);
            MP4Free(pBytes);

            char* sdpIod =
                (char*)MP4Malloc(strlen(iodBase64) + 64);
            snprintf(sdpIod, strlen(iodBase64) + 64,
                     "a=mpeg4-iod: \042data:application/mpeg4-iod;base64,%s\042",
                     iodBase64);
            MP4Free(iodBase64);

            delete pFile;

            return sdpIod;
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
        }

        if (pFile)
            delete pFile;
        return NULL;
    }

    /* Edit list */

    MP4EditId MP4AddTrackEdit(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId,
        MP4Timestamp startTime,
        MP4Duration duration,
        bool dwell)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                MP4EditId newEditId =
                    ((MP4File*)hFile)->AddTrackEdit(trackId, editId);

                if (newEditId != MP4_INVALID_EDIT_ID) {
                    ((MP4File*)hFile)->SetTrackEditMediaStart(
                        trackId, newEditId, startTime);
                    ((MP4File*)hFile)->SetTrackEditDuration(
                        trackId, newEditId, duration);
                    ((MP4File*)hFile)->SetTrackEditDwell(
                        trackId, newEditId, dwell);
                }

                return newEditId;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_EDIT_ID;
    }

    bool MP4DeleteTrackEdit(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->DeleteTrackEdit(trackId, editId);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    uint32_t MP4GetTrackNumberOfEdits(
        MP4FileHandle hFile,
        MP4TrackId trackId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackNumberOfEdits(trackId);
            }
            catch( Exception* x ) {
                //mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return 0;
    }

    MP4Timestamp MP4GetTrackEditMediaStart(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackEditMediaStart(
                           trackId, editId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_TIMESTAMP;
    }

    MP4Duration MP4GetTrackEditTotalDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackEditTotalDuration(
                           trackId, editId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    bool MP4SetTrackEditMediaStart(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId,
        MP4Timestamp startTime)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackEditMediaStart(
                    trackId, editId, startTime);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    MP4Duration MP4GetTrackEditDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackEditDuration(trackId, editId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_DURATION;
    }

    bool MP4SetTrackEditDuration(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId,
        MP4Duration duration)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackEditDuration(trackId, editId, duration);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    int8_t MP4GetTrackEditDwell(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetTrackEditDwell(trackId, editId);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return -1;
    }

    bool MP4SetTrackEditDwell(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4EditId editId,
        bool dwell)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                ((MP4File*)hFile)->SetTrackEditDwell(trackId, editId, dwell);
                return true;
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return false;
    }

    bool MP4ReadSampleFromEditTime(
        /* input parameters */
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Timestamp when,
        /* output parameters */
        uint8_t** ppBytes,
        uint32_t* pNumBytes,
        MP4Timestamp* pStartTime,
        MP4Duration* pDuration,
        MP4Duration* pRenderingOffset,
        bool* pIsSyncSample)
    {
        MP4SampleId sampleId =
            MP4GetSampleIdFromEditTime(
                hFile,
                trackId,
                when,
                pStartTime,
                pDuration);

        return MP4ReadSample(
                   hFile,
                   trackId,
                   sampleId,
                   ppBytes,
                   pNumBytes,
                   NULL,
                   NULL,
                   pRenderingOffset,
                   pIsSyncSample);
    }

    MP4SampleId MP4GetSampleIdFromEditTime(
        MP4FileHandle hFile,
        MP4TrackId trackId,
        MP4Timestamp when,
        MP4Timestamp* pStartTime,
        MP4Duration* pDuration)
    {
        if (MP4_IS_VALID_FILE_HANDLE(hFile)) {
            try {
                return ((MP4File*)hFile)->GetSampleIdFromEditTime(
                           trackId, when, pStartTime, pDuration);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return MP4_INVALID_SAMPLE_ID;
    }

    /* Utlities */

    char* MP4BinaryToBase16(
        const uint8_t* pData,
        uint32_t dataSize)
    {
        if (pData || dataSize == 0) {
            try {
                return MP4ToBase16(pData, dataSize);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    char* MP4BinaryToBase64(
        const uint8_t* pData,
        uint32_t dataSize)
    {
        if (pData || dataSize == 0) {
            try {
                return MP4ToBase64(pData, dataSize);
            }
            catch( Exception* x ) {
                mp4v2::impl::log.errorf(*x);
                delete x;
            }
            catch( ... ) {
                mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            }
        }
        return NULL;
    }

    void MP4Free (void *p)
    {
        if (p != NULL)
            free(p);
    }

    bool MP4AddIPodUUID (MP4FileHandle hFile, MP4TrackId trackId)
    {
        if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
            return false;

        MP4Track* track = NULL;
        MP4Atom* avc1 = NULL;

        try
        {
            track = ((MP4File*)hFile)->GetTrack(trackId);
            ASSERT(track);
            avc1 = track->GetTrakAtom().FindChildAtom("mdia.minf.stbl.stsd.avc1");
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
            return false;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: failed", __FUNCTION__ );
            return false;
        }

        IPodUUIDAtom    *ipod_uuid = NULL;
        try
        {
            ipod_uuid = new IPodUUIDAtom(*(MP4File*)hFile);
        }
        catch( std::bad_alloc ) {
            mp4v2::impl::log.errorf("%s: unable to allocate IPodUUIDAtom", __FUNCTION__);
        }
        catch( Exception* x ) {
            mp4v2::impl::log.errorf(*x);
            delete x;
            return false;
        }
        catch( ... ) {
            mp4v2::impl::log.errorf("%s: unknown exception constructing IPodUUIDAtom", __FUNCTION__ );
            return false;
        }

        try
        {
            ASSERT(avc1);
            ASSERT(ipod_uuid);
            avc1->AddChildAtom(ipod_uuid);
            return true;
        }
        catch( Exception* x ) {
            delete ipod_uuid;
            ipod_uuid = NULL;
            mp4v2::impl::log.errorf(*x);
            delete x;
            return false;
        }
        catch( ... ) {
            delete ipod_uuid;
            ipod_uuid = NULL;
            mp4v2::impl::log.errorf("%s: unknown exception adding IPodUUIDAtom", __FUNCTION__ );
            return false;
        }

        return false;
    }

///////////////////////////////////////////////////////////////////////////////

bool MP4GetTrackLanguage(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    char*         code )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return ((MP4File*)hFile)->GetTrackLanguage( trackId, code );
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

bool MP4SetTrackLanguage(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   code )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return ((MP4File*)hFile)->SetTrackLanguage( trackId, code );
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

bool MP4GetTrackName(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    char**        name )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return ((MP4File*)hFile)->GetTrackName( trackId, name );
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

bool MP4SetTrackName(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   code )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        return ((MP4File*)hFile)->SetTrackName( trackId, code );
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

bool MP4GetTrackDurationPerChunk(
    MP4FileHandle hFile,
    MP4TrackId    trackId, 
    MP4Duration*  duration )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    if (!duration)
        return false;

    try {
        *duration = ((MP4File*)hFile)->GetTrackDurationPerChunk( trackId );
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

bool MP4SetTrackDurationPerChunk(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4Duration   duration )
{
    if( !MP4_IS_VALID_FILE_HANDLE( hFile ))
        return false;

    try {
        ((MP4File*)hFile)->SetTrackDurationPerChunk( trackId, duration );
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

} // extern "C"
