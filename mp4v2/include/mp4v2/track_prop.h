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
 *      Kona Blend,                kona8lend@gmail.com
 *      Robert Kausch,             robert.kausch@freac.org
 */
#ifndef MP4V2_TRACK_PROP_H
#define MP4V2_TRACK_PROP_H

/**************************************************************************//**
 *
 *  @defgroup mp4_track_prop MP4v2 Track Properties
 *  @{
 *
 *****************************************************************************/

/* specific track properties */

/** Get the track type.
 *
 *  MP4GetTrackType gets the type of the track with the specified track id.
 *
 *  Note: the library does not provide a MP4SetTrackType function, the
 *  track type needs to be specified when the track is created, e.g.
 *  MP4AddSystemsTrack(MP4_OCI_TRACK_TYPE).
 *
 *  Known track types are:
 *
 *      @li #MP4_OD_TRACK_TYPE
 *      @li #MP4_SCENE_TRACK_TYPE
 *      @li #MP4_AUDIO_TRACK_TYPE
 *      @li #MP4_VIDEO_TRACK_TYPE
 *      @li #MP4_HINT_TRACK_TYPE
 *      @li #MP4_CNTL_TRACK_TYPE
 *      @li #MP4_TEXT_TRACK_TYPE
 *      @li #MP4_CLOCK_TRACK_TYPE
 *      @li #MP4_MPEG7_TRACK_TYPE
 *      @li #MP4_OCI_TRACK_TYPE
 *      @li #MP4_IPMP_TRACK_TYPE
 *      @li #MP4_MPEGJ_TRACK_TYPE
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *
 *  @return On success, a string indicating track type. On failure, NULL.
 */
MP4V2_EXPORT
const char* MP4GetTrackType(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the name of the track's media data atom.
 *
 *  MP4GetTrackMediaDataName returns the four character name of the specified
 *  track's media data atom, i.e. the child atom of the track's @b stsd atom.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the media data atom name is
 *      desired.
 *
 *  @return The name of the track's media data atom or NULL in case of an
 *      error.
 */
MP4V2_EXPORT
const char* MP4GetTrackMediaDataName(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the name of an encrypted track's original media data atom.
 *
 *  MP4GetTrackMediaDataOriginalFormat is used to get the original media data
 *  atom name if a track has been encrypted. The track identified by @p trackId
 *  must be an encrypted track with @b encv as the media data name returned by
 *  MP4GetTrackMediaDataName.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the encoded track for which the original media
 *      data atom name is desired.
 *  @param originalFormat specifies a buffer to receive the original media data
        atom name.
 *  @param buflen specifies the size of the buffer pointed to by @p
 *      originalFormat.
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4GetTrackMediaDataOriginalFormat(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    char*         originalFormat,
    uint32_t      buflen );

/** Get the duration of a track.
 *
 *  MP4GetTrackDuration returns the total duration of all the samples in the
 *  specified track in the mp4 file.
 *
 *  Caveat: The value is in units of the track time scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the duration is desired.
 *
 *  @return The duration in track time scale units of the track in the mp4
 *      file.
 */
MP4V2_EXPORT
MP4Duration MP4GetTrackDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the time scale of a track.
 *
 *  MP4GetTrackTimeScale returns the time scale of the specified track in
 *  the mp4 file. The time scale determines the number of clock ticks per
 *  second for this track.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *
 *  @return timescale (ticks per second) of the track in the mp4 file.
 */
MP4V2_EXPORT
uint32_t MP4GetTrackTimeScale(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Set the time scale of a track.
 *
 *  MP4SetTrackTimeScale sets the time scale of the specified track in the
 *  mp4 file. The time scale determines the number of clock ticks per
 *  second for this track.
 *
 *  Typically this value is set once when the track is created. However
 *  this call can be used to modify the value if that is desired. Since
 *  track sample durations are expressed in units of the track time scale,
 *  any change to the time scale value will effect the real time duration
 *  of the samples.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param value desired time scale for the track.
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4SetTrackTimeScale(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint32_t      value );

/** Get ISO-639-2/T language code of a track.
 *  The language code is a 3-char alpha code consisting of lower-case letters.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param code buffer to hold 3-char+null (4-bytes total).
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4GetTrackLanguage(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    char*         code );

/** Set ISO-639-2/T language code of a track.
 *  The language code is a 3-char alpha code consisting of lower-case letters.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param code 3-char language code.
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4SetTrackLanguage(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   code );

/** Get track name.
 *
 *  MP4GetTrackName gets the name of the track via udta.name property.
 *
 *  The memory to store the track name is allocated by the library, so the
 *  caller is responsible for freeing it with MP4Free().
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param name pointer to a variable to receive the track name.
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4GetTrackName(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    char**        name );

/** Set track name.
 *
 *  MP4SetTrackName sets the name of the track via udta.name property.
 *  The udta atom is created if needed.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param name the name to set as the track name.
 *
 *  @return <b>true</b> on success, <b>false</b> on failure.
 */
MP4V2_EXPORT
bool MP4SetTrackName(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   name );

/** Get the encoding type of an MPEG-4 audio track.
 *
 *  MP4GetTrackAudioMpeg4Type returns the MPEG-4 encoding type of the specified
 *  MPEG-4 audio track in the mp4 file. If an mp4 audio track has type
 *  MP4_MPEG4_AUDIO_TYPE, this call can be used to determine which specific
 *  MPEG-4 audio encoding is contained in the track.
 *
 *  Known MPEG-4 audio encoding types are:
 *
 *  Type                                | Description
 *  ------------------------------------|--------------------------------------
 *  MP4_MPEG4_AAC_MAIN_AUDIO_TYPE       | MPEG-4 AAC Main profile
 *  MP4_MPEG4_AAC_LC_AUDIO_TYPE         | MPEG-4 AAC Low Complexity profile
 *  MP4_MPEG4_AAC_SSR_AUDIO_TYPE        | MPEG-4 AAC SSR profile
 *  MP4_MPEG4_AAC_LTP_AUDIO_TYPE        | MPEG-4 AAC Long Term Prediction profile
 *  MP4_MPEG4_AAC_SCALABLE_AUDIO_TYPE   | MPEG-4 AAC Scalable
 *  MP4_MPEG4_CELP_AUDIO_TYPE           | MPEG-4 CELP
 *  MP4_MPEG4_HVXC_AUDIO_TYPE           | MPEG-4 HVXC
 *  MP4_MPEG4_TTSI_AUDIO_TYPE           | MPEG-4 Text To Speech
 *  MP4_MPEG4_MAIN_SYNTHETIC_AUDIO_TYPE | MPEG-4 Main Synthetic profile
 *  MP4_MPEG4_WAVETABLE_AUDIO_TYPE      | MPEG-4 Wavetable Synthesis profile
 *  MP4_MPEG4_MIDI_AUDIO_TYPE           | MPEG-4 MIDI profile
 *  MP4_MPEG4_ALGORITHMIC_FX_AUDIO_TYPE | MPEG-4 Algorithmic Synthesis and Audio FX profile
 *
 *  Note: This information is retrieved from the audio track ES configuration.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the MPEG-4 audio type is
 *      desired.
 *
 *  @return Upon success, the MPEG-4 audio type of the track. Upon error,
 *      MP4_MPEG4_INVALID_AUDIO_TYPE is returned.
 *
 *  @see MP4GetTrackAudioType()
 */
MP4V2_EXPORT
uint8_t MP4GetTrackAudioMpeg4Type(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the encoding object type id from a track's esds atom.
 *
 *  MP4GetTrackEsdsObjectTypeId returns the encoding object type of the
 *  specified audio or video track in the mp4 file.
 *
 *  Known audio object types are:
 *
 *  Type                          | Description
 *  ------------------------------|--------------------------------------------
 *  MP4_MPEG1_AUDIO_TYPE          | MPEG-1 Audio Layers I, II, & III
 *  MP4_MPEG2_AUDIO_TYPE          | MPEG-2 low bitrate extensions to MPEG-1 Audio
 *  ^                             | MP4_MP3_AUDIO_TYPE is an alias for this value
 *  MP4_MPEG2_AAC_MAIN_AUDIO_TYPE | MPEG-2 AAC Main profile
 *  MP4_MPEG2_AAC_LC_AUDIO_TYPE   | MPEG-2 AAC Low Complexity profile
 *  MP4_MPEG2_AAC_SSR_AUDIO_TYPE  | MPEG-2 AAC SSR profile
 *  MP4_MPEG4_AUDIO_TYPE          | MPEG-4 Audio, includes MPEG-4 extensions to AAC
 *  MP4_PRIVATE_AUDIO_TYPE        | User private type
 *
 *  Known video object types are:
 *
 *  Type                         | Description
 *  -----------------------------|---------------------------------------------
 *  MP4_MPEG1_VIDEO_TYPE         | MPEG-1 Video
 *  MP4_MPEG2_SIMPLE_VIDEO_TYPE  | MPEG-2 Simple Profile Video
 *  MP4_MPEG2_MAIN_VIDEO_TYPE    | MPEG-2 Main Profile Video (Broadcast/DVD)
 *  MP4_MPEG2_SNR_VIDEO_TYPE     | MPEG-2 SNR Profile Video
 *  MP4_MPEG2_SPATIAL_VIDEO_TYPE | MPEG-2 Spatial Scalability Profile Video
 *  MP4_MPEG2_HIGH_VIDEO_TYPE    | MPEG-2 High Profile Video (HDTV)
 *  MP4_MPEG2_442_VIDEO_TYPE     | MPEG-2 442 Profile Video (Studio)
 *  MP4_MPEG4_VIDEO_TYPE         | MPEG-4 Video
 *  MP4_JPEG_VIDEO_TYPE          | JPEG stills or motion JPEG
 *  MP4_PRIVATE_VIDEO_TYPE       | User private type
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the encoding object type is
 *      desired.
 *
 *  @return Upon success, the encoding object type of the track. Upon error,
 *      MP4_INVALID_AUDIO_TYPE is returned.
 */
MP4V2_EXPORT
uint8_t MP4GetTrackEsdsObjectTypeId(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the fixed duration of samples in a track.
 *
 *  MP4GetTrackFixedSampleDuration returns the duration of samples in the
 *  specified track in the mp4 file, if this value is fixed for all samples.
 *  This is typically the case for audio tracks and video tracks. If the track
 *  samples have variable duration, then MP4_INVALID_DURATION is returned.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *
 *  @return Upon success, the number of fixed duration of the samples in the
 *      track in track time scale units. Upon an error, MP4_INVALID_DURATION.
 */
MP4V2_EXPORT
MP4Duration MP4GetTrackFixedSampleDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the average bit rate in bits per second of the specified track.
 *
 *  MP4GetTrackBitRate returns the average bit rate in bits per second in the
 *  specified track in the mp4 file.
 *
 *  Note: hint tracks will not return their bit rate via this mechanism.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the bit rate is desired.
 *
 *  @return Upon success, the average bit rate in bits per second of the track.
 *      Upon an error, 0.
 */
MP4V2_EXPORT
uint32_t MP4GetTrackBitRate(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

MP4V2_EXPORT
bool MP4GetTrackVideoMetadata(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint8_t**     ppConfig,
    uint32_t*     pConfigSize );

/** Get the elementary stream (ES) configuration of a track.
 *
 *  MP4GetTrackESConfiguration returns the elementary stream (ES) configuration
 *  of the specified track in the mp4 file. This information is codec specific
 *  and contains the configuration necessary for the given codec to decode the
 *  samples in the track.
 *
 *  Caveat: the returned block of memory has been allocated by the library. The
 *  caller may safely modify the value without effecting the library, but the
 *  caller takes responsiblity for freeing the memory with MP4Free().
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the ES configuration is
 *      desired.
 *  @param ppConfig specifies a pointer to a pointer variable that will be
 *      given the address of the configuration information.
 *  @param pConfigSize specifies a pointer to a variable to hold the size of
 *      the ES configuration information.
 *
 *  @return Upon success, *ppConfig will point to a newly allocated block of
 *      memory with the ES configuration, and *pConfigSize will indicated the
 *      number of bytes of the ES configuration. Upon error, *ppConfig will be
 *      NULL, and *pConfigSize will be 0.
 *
 *  @see MP4SetTrackESConfiguration()
 */
MP4V2_EXPORT
bool MP4GetTrackESConfiguration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint8_t**     ppConfig,
    uint32_t*     pConfigSize );

/** Set the elementary stream (ES) configuration of a track.
 *
 *  MP4SetTrackESConfiguration sets the elementary stream (ES) configuration of
 *  the specified track in the mp4 file. This information is codec specific and
 *  contains the configuration necessary for the given codec to decode the
 *  samples in the track.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId the track to which the operation applies.
 *  @param pConfig specifies a pointer to the ES configuration information.
 *  @param configSize specifies the size of the ES configuration information.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetTrackESConfiguration()
 */
MP4V2_EXPORT
bool MP4SetTrackESConfiguration(
    MP4FileHandle  hFile,
    MP4TrackId     trackId,
    const uint8_t* pConfig,
    uint32_t       configSize );

/* h264 information routines */

MP4V2_EXPORT
bool MP4GetTrackH264ProfileLevel(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint8_t*      pProfile,
    uint8_t*      pLevel );

MP4V2_EXPORT
bool MP4GetTrackH264SeqPictHeaders(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint8_t***    pSeqHeaders,
    uint32_t**    pSeqHeaderSize,
    uint8_t***    pPictHeader,
    uint32_t**    pPictHeaderSize );

/** Frees the memory allocated by MP4GetTrackH264SeqPictHeaders.
 *
 *  MP4FreeH264SeqPictHeaders frees the memory that was allocated by a
 *  call to the MP4GetTrackH264SeqPictHeaders function.
 *
 *  When a client application wants to extract the H.264 video data from
 *  an MP4 file it will call MP4GetTrackH264SeqPictHeaders to obtain the
 *  sequence and picture parameter sets.  These parameter sets are
 *  required for decoding a sequence of one, or more, coded slices.  When
 *  the client application is done with the data it must free it.  On the
 *  Windows platform this cannot be done directly by the client
 *  application because the C runtime of the client application and the C
 *  runtime of the mp4v2 DLL may be different, which will result in an
 *  error at runtime.  This function allows the client application to let
 *  the mp4v2 DLL free the memory with the appropriate CRT heap manager.
 *
 *  @param pSeqHeaders pointer to an array of SPS pointers.
 *  @param pSeqHeaderSize pointer to array of SPS sizes.
 *  @param pPictHeader pointer to an array of PPS pointers.
 *  @param pPictHeaderSize pointer to array of PPS sizes.
 */
MP4V2_EXPORT
void MP4FreeH264SeqPictHeaders(
    uint8_t** pSeqHeaders,
    uint32_t* pSeqHeaderSize,
    uint8_t** pPictHeader,
    uint32_t* pPictHeaderSize );

MP4V2_EXPORT
bool MP4GetTrackH264LengthSize(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint32_t*     pLength );

/** Get the number of samples in a track.
 *
 *  MP4GetTrackNumberOfSamples returns the number of samples in the specified
 *  track in the mp4 file. Sample id's are the consecutive sequence of numbers
 *  from 1 to the total number of samples, i.e. 1-based indexing, not 0-based
 *  indexing.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the number of samples is
 *      desired.
 *
 *  @return Upon success, the number of samples in the track. Upon an error, 0.
 */
MP4V2_EXPORT
MP4SampleId MP4GetTrackNumberOfSamples(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the video width in pixels of the specified video track.
 *
 *  MP4GetTrackVideoWidth returns the width of the video in pixels in the
 *  specified track in the mp4 file.
 *
 *  Caveat: Not all mp4 implementations set this value accurately. The mp4
 *  specification states that the authoritative values are contained in the
 *  track ES configuration which is video encoding specific, and hence not
 *  interpretable by the mp4 library.
 *
 *  If the value of 320 is returned, care should be taken to verify the
 *  accuracy of this value since this is the default value in the mp4
 *  specification.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the video width is desired.
 *
 *  @return Upon success, the number of pixels of the video width in the
 *      track. Upon an error, 0.
 *
 *  @see MP4GetTrackVideoHeight()
 *  @see MP4GetTrackESConfiguration()
 */
MP4V2_EXPORT
uint16_t MP4GetTrackVideoWidth(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the video height in pixels of the specified video track.
 *
 *  MP4GetTrackVideoHeight returns the height of the video in pixels in the
 *  specified track in the mp4 file.
 *
 *  Caveat: Not all mp4 implementations set this value accurately. The mp4
 *  specification states that the authoritative values are contained in the
 *  track ES configuration which is video encoding specific, and hence not
 *  interpretable by the mp4 library.
 *
 *  If the value of 240 is returned, care should be taken to verify the
 *  accuracy of this value since this is the default value in the mp4
 *  specification.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the video height is desired.
 *
 *  @return Upon success, the number of pixels of the video height in the
 *      track. Upon an error, 0.
 *
 *  @see MP4GetTrackVideoWidth()
 *  @see MP4GetTrackESConfiguration()
 */
MP4V2_EXPORT
uint16_t MP4GetTrackVideoHeight(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the video frame rate of the specified video track.
 *
 *  MP4GetTrackVideoFrameRate returns the frame rate of the video in the
 *  specified track in the mp4 file. If the video is variable rate, the average
 *  frame rate is returned.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the video frame rate is
 *      desired.
 *
 *  @return Upon success, the number of video frames per second of the track.
 *      Upon an error, 0.
 */
MP4V2_EXPORT
double MP4GetTrackVideoFrameRate(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the number of channels of the specified audio track.
 *
 *  MP4GetTrackAudioChannels returns the number of audio channels in the
 *  specified track in the mp4 file.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the number of audio channels
 *      is desired.
 *
 *  @return Upon success, the number of audio channels of the track. Upon an
 *      error, -1.
 */
MP4V2_EXPORT
int MP4GetTrackAudioChannels(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Check whether a track is ISMACrypt encrypted.
 *
 *  MP4IsIsmaCrypMediaTrack checks whether the specified track is encrypted
 *  using ISMACrypt.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the information is desired.
 *
 *  @return true (1) if the track is ISMACrypt encrypted, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4IsIsmaCrypMediaTrack(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/* generic track properties */

/** Check for presence of a track atom.
 *
 *  MP4HaveTrackAtom checks for the presence of the track atom passed in @p
 *  atomName. @p atomName can specify an atom path to check for atoms that are
 *  not direct children of the @b trak atom, e.g. "mdia.minf.stbl".
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param atomName name of the atom to check for.
 *
 *  @return true (1) if the atom is present, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4HaveTrackAtom(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   atomName );

/** Get the value of an integer property for a track.
 *
 *  MP4GetTrackIntegerProperty determines the value of the integer property
 *  identified by @p propName, e.g. "tkhd.layer", for the track identified by
 *  @p trackId. The value is stored in the variable pointed to by @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetTrackIntegerProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    uint64_t*     retVal );

/** Get the value of a float property for a track.
 *
 *  MP4GetTrackFloatProperty determines the value of the float property
 *  identified by @p propName, e.g. "tkhd.volume", for the track identified by
 *  @p trackId. The value is stored in the variable pointed to by @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetTrackFloatProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    float*        retVal );

/** Get the value of a string property for a track.
 *
 *  MP4GetTrackStringProperty determines the value of the string property
 *  identified by @p propName, e.g. "udta.hnti.sdp .sdpText", for the track
 *  identified by @p trackId. The value is stored in the variable pointed to by
 *  @p retVal.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to get.
 *  @param retVal pointer to a variable to receive the return value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetTrackStringProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    const char**  retVal );

/** Get the value of a bytes property for a track.
 *
 *  MP4GetTrackBytesProperty determines the value of the bytes property
 *  identified by @p propName, e.g. "tkhd.matrix", for the track identified by
 *  @p trackId. The value is stored in a newly allocated buffer the location of
 *  which is assigned to the variable pointed to by ppValue. The caller is
 *  responsible for freeing the memory with MP4Free().
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to get.
 *  @param ppValue pointer to a variable to receive the memory location
 *      containing the property bytes.
 *  @param pValueSize pointer to a variable to receive the length of the
 *      property bytes value.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4GetTrackBytesProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    uint8_t**     ppValue,
    uint32_t*     pValueSize );

/** Set the value of an integer property for a track.
 *
 *  MP4SetTrackIntegerProperty sets the value of the integer property
 *  identified by @p propName, e.g. "tkhd.layer", for the track identified by
 *  @p trackId.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetTrackIntegerProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    int64_t       value );

/** Set the value of a float property for a track.
 *
 *  MP4SetTrackFloatProperty sets the value of the float property identified by
 *  @p propName, e.g. "tkhd.volume", for the track identified by @p trackId.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetTrackFloatProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    float         value );

/** Set the value of a string property for a track.
 *
 *  MP4SetTrackStringProperty sets the value of the string property identified
 *  by @p propName, e.g. "udta.hnti.sdp .sdpText", for the track identified by
 *  @p trackId.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to set.
 *  @param value the new value of the property.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetTrackStringProperty(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    const char*   propName,
    const char*   value );

/** Set the value of a bytes property for a track.
 *
 *  MP4SetTrackBytesProperty sets the value of the bytes property identified by
 *  @p propName, e.g. "tkhd.matrix", for the track identified by @p trackId.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param propName path to the property to set.
 *  @param pValue pointer the bytes representing the new value of the property.
 *  @param valueSize the size of the bytes value pointed to by <b>pValue</b>.
 *
 *  @return true (1) on success, false (0) otherwise.
 */
MP4V2_EXPORT
bool MP4SetTrackBytesProperty(
    MP4FileHandle  hFile,
    MP4TrackId     trackId,
    const char*    propName,
    const uint8_t* pValue,
    uint32_t       valueSize);

/** @} ***********************************************************************/

#endif /* MP4V2_TRACK_PROP_H */
