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
 * Contributor(s):
 *      Dave Mackie,               dmackie@cisco.com
 *      Alix Marchandise-Franquet, alix@cisco.com
 *      Bill May,                  wmay@cisco.com
 */
#ifndef MP4V2_ISMA_H
#define MP4V2_ISMA_H

/**************************************************************************//**
 *
 *  @defgroup mp4_isma MP4v2 ISMA (Internet Streaming Media Alliance)
 *  @{
 *
 *****************************************************************************/

/** something */
typedef struct mp4v2_ismacryp_session_params {
    uint32_t    scheme_type;
    uint16_t    scheme_version;
    uint8_t     key_ind_len;
    uint8_t     iv_len;
    uint8_t     selective_enc;
    const char* kms_uri;
} mp4v2_ismacrypParams;

/*
 * API to initialize ismacryp properties to sensible defaults
 * if input param is null, a params struct is allocated
 */

MP4V2_EXPORT
mp4v2_ismacrypParams* MP4DefaultISMACrypParams(
    mp4v2_ismacrypParams* ptr );

MP4V2_EXPORT
MP4TrackId MP4AddEncAudioTrack(
    MP4FileHandle         hFile,
    uint32_t              timeScale,
    MP4Duration           sampleDuration,
    mp4v2_ismacrypParams* icPp,
    uint8_t               audioType DEFAULT(MP4_MPEG4_AUDIO_TYPE) );

MP4V2_EXPORT
MP4TrackId MP4AddEncVideoTrack(
    MP4FileHandle         hFile,
    uint32_t              timeScale,
    MP4Duration           sampleDuration,
    uint16_t              width,
    uint16_t              height,
    mp4v2_ismacrypParams* icPp,
    uint8_t               videoType DEFAULT(MP4_MPEG4_VIDEO_TYPE),
    const char*           oFormat DEFAULT(NULL) );

MP4V2_EXPORT
MP4TrackId MP4AddEncH264VideoTrack(
    MP4FileHandle         dstFile,
    uint32_t              timeScale,
    MP4Duration           sampleDuration,
    uint16_t              width,
    uint16_t              height,
    MP4FileHandle         srcFile,
    MP4TrackId            srcTrackId,
    mp4v2_ismacrypParams* icPp );

MP4V2_EXPORT
MP4TrackId MP4EncAndCloneTrack(
    MP4FileHandle         srcFile,
    MP4TrackId            srcTrackId,
    mp4v2_ismacrypParams* icPp,
    MP4FileHandle         dstFile DEFAULT(MP4_INVALID_FILE_HANDLE),
    MP4TrackId            dstHintTrackReferenceTrack DEFAULT(MP4_INVALID_TRACK_ID) );

MP4V2_EXPORT
MP4TrackId MP4EncAndCopyTrack(
    MP4FileHandle         srcFile,
    MP4TrackId            srcTrackId,
    mp4v2_ismacrypParams* icPp,
    encryptFunc_t         encfcnp,
    uint32_t              encfcnparam1,
    MP4FileHandle         dstFile DEFAULT(MP4_INVALID_FILE_HANDLE),
    bool                  applyEdits DEFAULT(false),
    MP4TrackId            dstHintTrackReferenceTrack DEFAULT(MP4_INVALID_TRACK_ID) );

/** Add ISMA compliant OD and Scene tracks.
 *
 *  MP4MakeIsmaCompliant modifies an mp4 file so that it complies with the
 *  minimal MPEG-4 Systems requirements defined by the Internet Streaming Media
 *  Alliance (ISMA).
 *
 *  This involves creating an OD and Scene track, and using them to describe a
 *  simple scene of one audio, or one video, or one of each. Whether an SDP
 *  version of this information is added to the mp4 file can be controlled with
 *  the @p addIsmaComplianceSdp parameter.
 *
 *  Caveat: whether the file is truly ISMA compliant also depends on the
 *  contents of the media and hint tracks. This function does not guarantee
 *  that these tracks are compliant.
 *
 *  @param fileName specifies the path name of the file to be modified.
 *  @param addIsmaComplianceSdp specifies whether an SDP declaring ISMA
 *      compliance should be added to the file.
 * 
 *  @return Upon success, true (1). Upon an error, false (0).
 */
MP4V2_EXPORT
bool MP4MakeIsmaCompliant(
    const char* fileName,
    bool        addIsmaComplianceSdp DEFAULT(true) );

MP4V2_EXPORT
char* MP4MakeIsmaSdpIod(
    uint8_t  videoProfile,
    uint32_t videoBitrate,
    uint8_t* videoConfig,
    uint32_t videoConfigLength,
    uint8_t  audioProfile,
    uint32_t audioBitrate,
    uint8_t* audioConfig,
    uint32_t audioConfigLength );

/** @} ***********************************************************************/

#endif /* MP4V2_ISMA_H */
