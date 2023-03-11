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
 *      Dave Mackie,               dmackie@cisco.com
 *      Alix Marchandise-Franquet, alix@cisco.com
 *      Ximpo Group Ltd.,          mp4v2@ximpo.com
 *      Bill May,                  wmay@cisco.com
 *      Kona Blend,                kona8lend@gmail.com
 */
#ifndef MP4V2_GENERAL_H
#define MP4V2_GENERAL_H

/**************************************************************************//**
 *
 *  @defgroup mp4_general MP4v2 General
 *  @{
 *
 *****************************************************************************/

/* MP4 API types */
typedef void*       MP4FileHandle;
typedef uint32_t    MP4TrackId;
typedef uint32_t    MP4SampleId;
typedef uint64_t    MP4Timestamp;
typedef uint64_t    MP4Duration;
typedef uint32_t    MP4EditId;

typedef enum {
    MP4_LOG_NONE = 0,
    MP4_LOG_ERROR = 1,
    MP4_LOG_WARNING = 2,
    MP4_LOG_INFO = 3,
    MP4_LOG_VERBOSE1 = 4,
    MP4_LOG_VERBOSE2 = 5,
    MP4_LOG_VERBOSE3 = 6,
    MP4_LOG_VERBOSE4 = 7
}  MP4LogLevel;

/*****************************************************************************/

typedef void (*MP4LogCallback)(
    MP4LogLevel loglevel,
    const char* fmt,
    va_list     ap );

/*****************************************************************************/

/** Encryption function pointer.
 *
 * @see MP4EncAndCopySample()
 * @see MP4EncAndCopyTrack()
 */
typedef uint32_t (*encryptFunc_t)( uint32_t, uint32_t, uint8_t*, uint32_t*, uint8_t** );

/*****************************************************************************/

#define MP4_INVALID_FILE_HANDLE ((MP4FileHandle)NULL) /**< Constant: invalid MP4FileHandle. */
#define MP4_INVALID_TRACK_ID    ((MP4TrackId)0)       /**< Constant: invalid MP4TrackId. */
#define MP4_INVALID_SAMPLE_ID   ((MP4SampleId)0)      /**< Constant: invalid MP4SampleId. */
#define MP4_INVALID_TIMESTAMP   ((MP4Timestamp)-1)    /**< Constant: invalid MP4Timestamp. */
#define MP4_INVALID_DURATION    ((MP4Duration)-1)     /**< Constant: invalid MP4Duration. */
#define MP4_INVALID_EDIT_ID     ((MP4EditId)0)        /**< Constant: invalid MP4EditId. */

/* Macros to test for API type validity */
#define MP4_IS_VALID_FILE_HANDLE(x) ((x) != MP4_INVALID_FILE_HANDLE)
#define MP4_IS_VALID_TRACK_ID(x)    ((x) != MP4_INVALID_TRACK_ID)
#define MP4_IS_VALID_SAMPLE_ID(x)   ((x) != MP4_INVALID_SAMPLE_ID)
#define MP4_IS_VALID_TIMESTAMP(x)   ((x) != MP4_INVALID_TIMESTAMP)
#define MP4_IS_VALID_DURATION(x)    ((x) != MP4_INVALID_DURATION)
#define MP4_IS_VALID_EDIT_ID(x)     ((x) != MP4_INVALID_EDIT_ID)

/*
 * MP4 Known track type names - e.g. MP4GetNumberOfTracks(type)
 *
 * Note this first group of track types should be created
 * via the MP4Add<Type>Track() functions, and not MP4AddTrack(type)
 */
#define MP4_OD_TRACK_TYPE       "odsm"  /**< Constant: OD track. */
#define MP4_SCENE_TRACK_TYPE    "sdsm"  /**< Constant: scene track. */
#define MP4_AUDIO_TRACK_TYPE    "soun"  /**< Constant: audio track. */
#define MP4_VIDEO_TRACK_TYPE    "vide"  /**< Constant: video track. */
#define MP4_HINT_TRACK_TYPE     "hint"  /**< Constant: hint track. */
#define MP4_CNTL_TRACK_TYPE     "cntl"  /**< Constant: control track. */
#define MP4_TEXT_TRACK_TYPE     "text"  /**< Constant: text track. */
#define MP4_SUBTITLE_TRACK_TYPE "sbtl"  /**< Constant: subtitle track. */
#define MP4_SUBPIC_TRACK_TYPE   "subp"  /**< Constant: subpic track. */
/*
 * This second set of track types should be created
 * via MP4AddSystemsTrack(type)
 */
#define MP4_CLOCK_TRACK_TYPE    "crsm"  /**< Constant: clock track. */
#define MP4_MPEG7_TRACK_TYPE    "m7sm"  /**< Constant: mpeg7 track. */
#define MP4_OCI_TRACK_TYPE      "ocsm"  /**< Constant: OCI track. */
#define MP4_IPMP_TRACK_TYPE     "ipsm"  /**< Constant: IPMP track. */
#define MP4_MPEGJ_TRACK_TYPE    "mjsm"  /**< Constant: MPEGJ track. */

#define MP4_IS_VIDEO_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_VIDEO_TRACK_TYPE))

#define MP4_IS_AUDIO_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_AUDIO_TRACK_TYPE))

#define MP4_IS_CNTL_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_CNTL_TRACK_TYPE))

#define MP4_IS_OD_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_OD_TRACK_TYPE))

#define MP4_IS_SCENE_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_SCENE_TRACK_TYPE))

#define MP4_IS_HINT_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_HINT_TRACK_TYPE))

#define MP4_IS_SYSTEMS_TRACK_TYPE(type) \
    (!strcasecmp(type, MP4_CLOCK_TRACK_TYPE) || \
     !strcasecmp(type, MP4_MPEG7_TRACK_TYPE) || \
     !strcasecmp(type, MP4_OCI_TRACK_TYPE)   || \
     !strcasecmp(type, MP4_IPMP_TRACK_TYPE)  || \
     !strcasecmp(type, MP4_MPEGJ_TRACK_TYPE))

/* MP4 Audio track types - see MP4AddAudioTrack()*/
#define MP4_INVALID_AUDIO_TYPE             0x00
#define MP4_MPEG1_AUDIO_TYPE               0x6B
#define MP4_MPEG2_AUDIO_TYPE               0x69
#define MP4_MP3_AUDIO_TYPE                 MP4_MPEG2_AUDIO_TYPE
#define MP4_MPEG2_AAC_MAIN_AUDIO_TYPE      0x66
#define MP4_MPEG2_AAC_LC_AUDIO_TYPE        0x67
#define MP4_MPEG2_AAC_SSR_AUDIO_TYPE       0x68
#define MP4_MPEG2_AAC_AUDIO_TYPE           MP4_MPEG2_AAC_MAIN_AUDIO_TYPE
#define MP4_MPEG4_AUDIO_TYPE               0x40
#define MP4_PRIVATE_AUDIO_TYPE             0xC0
#define MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE 0xE0 /* a private definition */
#define MP4_VORBIS_AUDIO_TYPE              0xE1 /* a private definition */
#define MP4_AC3_AUDIO_TYPE                 0xE2 /* a private definition */
#define MP4_ALAW_AUDIO_TYPE                0xE3 /* a private definition */
#define MP4_ULAW_AUDIO_TYPE                0xE4 /* a private definition */
#define MP4_G723_AUDIO_TYPE                0xE5 /* a private definition */
#define MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE    0xE6 /* a private definition */

/* MP4 MPEG-4 Audio types from 14496-3 Table 1.5.1 */
#define MP4_MPEG4_INVALID_AUDIO_TYPE         0
#define MP4_MPEG4_AAC_MAIN_AUDIO_TYPE        1
#define MP4_MPEG4_AAC_LC_AUDIO_TYPE          2
#define MP4_MPEG4_AAC_SSR_AUDIO_TYPE         3
#define MP4_MPEG4_AAC_LTP_AUDIO_TYPE         4
#define MP4_MPEG4_SBR_AUDIO_TYPE             5
#define MP4_MPEG4_AAC_SCALABLE_AUDIO_TYPE    6
#define MP4_MPEG4_TWINVQ_AUDIO_TYPE          7
#define MP4_MPEG4_CELP_AUDIO_TYPE            8
#define MP4_MPEG4_HVXC_AUDIO_TYPE            9
#define MP4_MPEG4_TTSI_AUDIO_TYPE            12
#define MP4_MPEG4_MAIN_SYNTHETIC_AUDIO_TYPE  13
#define MP4_MPEG4_WAVETABLE_AUDIO_TYPE       14
#define MP4_MPEG4_MIDI_AUDIO_TYPE            15
#define MP4_MPEG4_ALGORITHMIC_FX_AUDIO_TYPE  16
#define MP4_MPEG4_ER_AAC_LC_AUDIO_TYPE       17
#define MP4_MPEG4_ER_AAC_LTP_AUDIO_TYPE      19
#define MP4_MPEG4_ER_AAC_SCALABLE_AUDIO_TYPE 20
#define MP4_MPEG4_ER_TWINVQ_AUDIO_TYPE       21
#define MP4_MPEG4_ER_BSAC_AUDIO_TYPE         22
#define MP4_MPEG4_ER_AAC_LD_AUDIO_TYPE       23
#define MP4_MPEG4_ER_CELP_AUDIO_TYPE         24
#define MP4_MPEG4_ER_HXVC_AUDIO_TYPE         25
#define MP4_MPEG4_ER_HILN_AUDIO_TYPE         26
#define MP4_MPEG4_ER_PARAMETRIC_AUDIO_TYPE   27
#define MP4_MPEG4_SSC_AUDIO_TYPE             28
#define MP4_MPEG4_PS_AUDIO_TYPE              29
#define MP4_MPEG4_MPEG_S_AUDIO_TYPE          30
#define MP4_MPEG4_LAYER1_AUDIO_TYPE          32
#define MP4_MPEG4_LAYER2_AUDIO_TYPE          33
#define MP4_MPEG4_LAYER3_AUDIO_TYPE          34
#define MP4_MPEG4_DST_AUDIO_TYPE             35
#define MP4_MPEG4_ALS_AUDIO_TYPE             36
#define MP4_MPEG4_SLS_AUDIO_TYPE             37
#define MP4_MPEG4_SLS_NON_CORE_AUDIO_TYPE    38
#define MP4_MPEG4_ER_AAC_ELD_AUDIO_TYPE      39
#define MP4_MPEG4_SMR_SIMPLE_AUDIO_TYPE      40
#define MP4_MPEG4_SMR_MAIN_AUDIO_TYPE        41
#define MP4_MPEG4_USAC_AUDIO_TYPE            42
#define MP4_MPEG4_SAOC_AUDIO_TYPE            43
#define MP4_MPEG4_MPEG_S_LD_AUDIO_TYPE       44

#define MP4_MPEG4_AAC_HE_AUDIO_TYPE          MP4_MPEG4_SBR_AUDIO_TYPE
#define MP4_MPEG4_AAC_HEV2_AUDIO_TYPE        MP4_MPEG4_PS_AUDIO_TYPE
#define MP4_MPEG4_AAC_XHE_AUDIO_TYPE         MP4_MPEG4_USAC_AUDIO_TYPE

/* MP4 Audio type utilities following common usage */
#define MP4_IS_MP3_AUDIO_TYPE(type) \
    ((type) == MP4_MPEG1_AUDIO_TYPE || \
     (type) == MP4_MPEG2_AUDIO_TYPE)

#define MP4_IS_MPEG2_AAC_AUDIO_TYPE(type) \
    ((type) >= MP4_MPEG2_AAC_MAIN_AUDIO_TYPE && \
     (type) <= MP4_MPEG2_AAC_SSR_AUDIO_TYPE)

#define MP4_IS_MPEG4_AAC_AUDIO_TYPE(mpeg4Type) \
    (((mpeg4Type) >= MP4_MPEG4_AAC_MAIN_AUDIO_TYPE  && \
      (mpeg4Type) <= MP4_MPEG4_AAC_SCALABLE_AUDIO_TYPE)    || \
      (mpeg4Type) == MP4_MPEG4_AAC_HEV2_AUDIO_TYPE         || \
      (mpeg4Type) == MP4_MPEG4_AAC_XHE_AUDIO_TYPE          || \
     ((mpeg4Type) >= MP4_MPEG4_ER_AAC_LC_AUDIO_TYPE && \
      (mpeg4Type) <= MP4_MPEG4_ER_AAC_SCALABLE_AUDIO_TYPE) || \
      (mpeg4Type) == MP4_MPEG4_ER_AAC_LD_AUDIO_TYPE        || \
      (mpeg4Type) == MP4_MPEG4_ER_AAC_ELD_AUDIO_TYPE)

#define MP4_IS_AAC_AUDIO_TYPE(type) \
    (MP4_IS_MPEG2_AAC_AUDIO_TYPE(type) || \
     (type) == MP4_MPEG4_AUDIO_TYPE)

/* MP4 Video track types - see MP4AddVideoTrack() */
#define MP4_INVALID_VIDEO_TYPE          0x00
#define MP4_MPEG1_VIDEO_TYPE            0x6A
#define MP4_MPEG2_SIMPLE_VIDEO_TYPE     0x60
#define MP4_MPEG2_MAIN_VIDEO_TYPE       0x61
#define MP4_MPEG2_SNR_VIDEO_TYPE        0x62
#define MP4_MPEG2_SPATIAL_VIDEO_TYPE    0x63
#define MP4_MPEG2_HIGH_VIDEO_TYPE       0x64
#define MP4_MPEG2_442_VIDEO_TYPE        0x65
#define MP4_MPEG2_VIDEO_TYPE            MP4_MPEG2_MAIN_VIDEO_TYPE
#define MP4_MPEG4_VIDEO_TYPE            0x20
#define MP4_JPEG_VIDEO_TYPE             0x6C
#define MP4_PRIVATE_VIDEO_TYPE          0xD0
#define MP4_YUV12_VIDEO_TYPE            0xF0    /* a private definition */
#define MP4_H263_VIDEO_TYPE             0xF2    /* a private definition */
#define MP4_H261_VIDEO_TYPE             0xF3    /* a private definition */

/* MP4 Video type utilities */
#define MP4_IS_MPEG1_VIDEO_TYPE(type) \
    ((type) == MP4_MPEG1_VIDEO_TYPE)

#define MP4_IS_MPEG2_VIDEO_TYPE(type) \
    (MP4_IS_MPEG1_VIDEO_TYPE(type) || \
     ((type) >= MP4_MPEG2_SIMPLE_VIDEO_TYPE && \
      (type) <= MP4_MPEG2_442_VIDEO_TYPE))

#define MP4_IS_MPEG4_VIDEO_TYPE(type) \
    ((type) == MP4_MPEG4_VIDEO_TYPE)

/* Mpeg4 Visual Profile Defines - ISO/IEC 14496-2:2001/Amd.2:2002(E) */
#define MPEG4_SP_L1 (0x1)
#define MPEG4_SP_L2 (0x2)
#define MPEG4_SP_L3 (0x3)
#define MPEG4_SP_L0 (0x8)
#define MPEG4_SSP_L1 (0x11)
#define MPEG4_SSP_L2 (0x12)
#define MPEG4_CP_L1 (0x21)
#define MPEG4_CP_L2 (0x22)
#define MPEG4_MP_L2 (0x32)
#define MPEG4_MP_L3 (0x33)
#define MPEG4_MP_L4 (0x34)
#define MPEG4_NBP_L2 (0x42)
#define MPEG4_STP_L1 (0x51)
#define MPEG4_SFAP_L1 (0x61)
#define MPEG4_SFAP_L2 (0x62)
#define MPEG4_SFBAP_L1 (0x63)
#define MPEG4_SFBAP_L2 (0x64)
#define MPEG4_BATP_L1 (0x71)
#define MPEG4_BATP_L2 (0x72)
#define MPEG4_HP_L1 (0x81)
#define MPEG4_HP_L2 (0x82)
#define MPEG4_ARTSP_L1 (0x91)
#define MPEG4_ARTSP_L2 (0x92)
#define MPEG4_ARTSP_L3 (0x93)
#define MPEG4_ARTSP_L4 (0x94)
#define MPEG4_CSP_L1 (0xa1)
#define MPEG4_CSP_L2 (0xa2)
#define MPEG4_CSP_L3 (0xa3)
#define MPEG4_ACEP_L1 (0xb1)
#define MPEG4_ACEP_L2 (0xb2)
#define MPEG4_ACEP_L3 (0xb3)
#define MPEG4_ACEP_L4 (0xb4)
#define MPEG4_ACP_L1 (0xc1)
#define MPEG4_ACP_L2 (0xc2)
#define MPEG4_AST_L1 (0xd1)
#define MPEG4_AST_L2 (0xd2)
#define MPEG4_AST_L3 (0xd3)
#define MPEG4_S_STUDIO_P_L1 (0xe1)
#define MPEG4_S_STUDIO_P_L2 (0xe2)
#define MPEG4_S_STUDIO_P_L3 (0xe3)
#define MPEG4_S_STUDIO_P_L4 (0xe4)
#define MPEG4_C_STUDIO_P_L1 (0xe5)
#define MPEG4_C_STUDIO_P_L2 (0xe6)
#define MPEG4_C_STUDIO_P_L3 (0xe7)
#define MPEG4_C_STUDIO_P_L4 (0xe8)
#define MPEG4_ASP_L0 (0xF0)
#define MPEG4_ASP_L1 (0xF1)
#define MPEG4_ASP_L2 (0xF2)
#define MPEG4_ASP_L3 (0xF3)
#define MPEG4_ASP_L4 (0xF4)
#define MPEG4_ASP_L5 (0xF5)
#define MPEG4_ASP_L3B (0xF7)
#define MPEG4_FGSP_L0 (0xf8)
#define MPEG4_FGSP_L1 (0xf9)
#define MPEG4_FGSP_L2 (0xfa)
#define MPEG4_FGSP_L3 (0xfb)
#define MPEG4_FGSP_L4 (0xfc)
#define MPEG4_FGSP_L5 (0xfd)

/*****************************************************************************/

/* 3GP specific utilities */

MP4V2_EXPORT
bool MP4Make3GPCompliant(
    const char* fileName,
    char*       majorBrand DEFAULT(0),
    uint32_t    minorVersion DEFAULT(0),
    char**      supportedBrands DEFAULT(NULL),
    uint32_t    supportedBrandsCount DEFAULT(0),
    bool        deleteIodsAtom DEFAULT(true) );

/* NOTE this section of functionality has not yet been fully tested */

/** Add an edit segment to a track.
 *
 *  MP4AddTrackEdit adds an edit segment to the track edit list.
 *
 *  The track edit list is a feature that allows creation of an alternate
 *  timeline for the track, typically cutting out segments of the full track to
 *  form a shortened, cleaned up version. The edit segments that form the edit
 *  list are a sequence of track start times and durations, they do not alter
 *  the track media in any way, i.e. no data can be lost via edit list
 *  operations.
 *
 *  To read out the editted version of the track, use
 *  MP4ReadSampleFromEditTime() instead of MP4ReadSample().
 *
 *  To export the editted version of the track to a new track, potentially in a
 *  new mp4 file, see MP4CopyTrack().
 *
 *  Note with many media encodings such as MPEG-4, AAC and MP3, care must be
 *  taken when choosing the edit segment start times. E.g. for video tracks a
 *  reference or key frame should be selected as the starting sample of any
 *  edit segment. For audio tracks, an audio sample start time should be used.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the desired position in the edit list sequence for
 *      the new edit segment. If the value is MP4_INVALID_EDIT_ID, then the
 *      edit segment is added at the end of the existing edit list. Note
 *      editId's start with the value of 1, not 0.
 *  @param startTime specifies the starting time of the edit segment in the
 *      track time scale.
 *  @param duration specifies the duration of the edit segment in the track
 *      time scale.
 *  @param dwell If false, the track media should be played at its normal rate.
 *      If true, the media should be paused for the duration of this edit
 *      segment. This is a mechanism by which one can delay the start of a
 *      media track.
 *
 *  @return Upon success, the edit id of the new edit segment. Upon an error,
 *      MP4_INVALID_EDIT_ID.
 *
 *  @see MP4DeleteTrackEdit()
 *  @see MP4ReadSampleFromEditTime()
 *  @see MP4CopyTrack()
 */
MP4V2_EXPORT
MP4EditId MP4AddTrackEdit(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId DEFAULT(MP4_INVALID_EDIT_ID),
    MP4Timestamp  startTime DEFAULT(0),
    MP4Duration   duration DEFAULT(0),
    bool          dwell DEFAULT(false) );

/** Delete a track edit segment.
 *
 *  MP4DeleteTrackEdit deletes the specified track edit segment. Note that
 *  since editId's form a sequence, deleting an editId will cause all edit
 *  segments with editId's greater than the deleted one to be reassigned to
 *  their editId minus one.
 *
 *  Deleting an edit segment does not delete any media samples.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment to be deleted.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4AddTrackEdit()
 */
MP4V2_EXPORT
bool MP4DeleteTrackEdit(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId );

/** Get the number of edit segments for a track.
 *
 *  MP4GetTrackNumberOfEdits returns the number of edit segments in the
 *  specified track in the mp4 file. Edit id's are the consecutive sequence of
 *  numbers from 1 to the total number of edit segments, i.e. 1-based indexing,
 *  not 0-based indexing.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track for which the number of edit segments is
 *      desired.
 *
 *  @return Upon success, the number of edit segments for the track. Upon an
 *      error, 0.
 *
 *  @see MP4AddTrackEdit()
 *  @see MP4DeleteTrackEdit()
 */
MP4V2_EXPORT
uint32_t MP4GetTrackNumberOfEdits(
    MP4FileHandle hFile,
    MP4TrackId    trackId );

/** Get the start time of a track edit segment.
 *
 *  MP4GetTrackEditStart returns the start time of the specified track edit
 *  segment in the timeline of the track edit list.
 *
 *  Caveat: The value is in units of the track time scale.
 *
 *  Note that this differs from the edit segment media start time,
 *  MP4GetTrackEditMediaStart(). For example:
 *
 *  EditId | Start | MediaStart | Duration
 *  -------|-------|------------|---------
 *  1      | 0     | 15         | 30
 *  2      | 30    | 120        | 20
 *  3      | 50    | 3000       | 10
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment for which the start time is
 *      desired.
 *
 *  @return The start time of the edit segment in track time scale units of the
 *      track in the mp4 file.
 *
 *  @see MP4SetTrackEditStart()
 */
MP4V2_EXPORT
MP4Timestamp MP4GetTrackEditStart(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId );

/** Get the total duration of a sequence of track edit segments.
 *
 *  MP4GetTrackEditTotalDuration returns the total duration of the specified
 *  sequence of track edit segments from the first edit segment up to and
 *  including the specified edit segment. If the edit id value is
 *  MP4_INVALID_EDIT_ID, then the total duration of all of the edit segments is
 *  returned.
 *
 *  Caveat: The value is in units of the track time scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment for which the total duration is
 *      desired. A value of MP4_INVALID_EDIT_ID specifies that all edit
 *      segments should be included.
 *
 *  @return The total duration of the edit segment sequence in track time scale
 *      units of the track in the mp4 file.
 *
 *  @see MP4GetTrackEditDuration()
 */
MP4V2_EXPORT
MP4Duration MP4GetTrackEditTotalDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId DEFAULT(MP4_INVALID_EDIT_ID) );

/** Get the media start time of a track edit segment.
 *
 *  MP4GetTrackEditMediaStart returns the media start time of the specified
 *  track edit segment. 
 *
 *  Caveat: The value is in units of the track time scale.
 *
 *  Note that this differs from the edit segment start time,
 *  MP4GetTrackEditStart(). For example:
 *
 *  EditId | Start | MediaStart | Duration
 *  -------|-------|------------|---------
 *  1      | 0     | 15         | 30
 *  2      | 30    | 120        | 20
 *  3      | 50    | 3000       | 10
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment for which the media start time is
 *      desired.
 *
 *  @return The media start time of the edit segment in track time scale units
 *      of the track in the mp4 file.
 *
 *  @see MP4SetTrackEditMediaStart()
 */
MP4V2_EXPORT
MP4Timestamp MP4GetTrackEditMediaStart(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId );

/** Set the media start time of a track edit segment.
 *
 *  MP4SetTrackEditMediaStart sets the media start time of the specified edit
 *  segment from the specified track in the track time scale units. See
 *  MP4ConvertToTrackTimestamp() for how to map this value from another time
 *  scale.
 *
 *  Note that this differs from the edit segment start time. For example:
 *
 *  EditId | Start | MediaStart | Duration
 *  -------|-------|------------|---------
 *  1      | 0     | 15         | 30
 *  2      | 30    | 120        | 20
 *  3      | 50    | 3000       | 10
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment to which the operation applies.
 *      Caveat: the first edit has id 1 not 0.
 *  @param startTime specifies the new value for the media start in track time
 *      scale units.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetTrackEditMediaStart()
 */
MP4V2_EXPORT
bool MP4SetTrackEditMediaStart(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId,
    MP4Timestamp  startTime );

/** Get the duration of a track edit segment.
 *
 *  MP4GetTrackEditDuration returns the duration of the specified track edit
 *  segment.
 *
 *  Caveat: The value is in units of the track time scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment for which the duration is desired.
 *
 *  @return The duration of the edit segment in track time scale units of the
 *      track in the mp4 file.
 *
 *  @see MP4SetTrackEditDuration()
 */
MP4V2_EXPORT
MP4Duration MP4GetTrackEditDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId );

/** Set the duration of a track edit segment.
 *
 *  MP4SetTrackEditDuration sets the duration of the specified edit segment
 *  from the specified track in the track time scale units. See
 *  MP4ConvertToTrackDuration() for how to map this value from another time
 *  scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment to which the operation applies.
 *      Caveat: the first edit has id 1 not 0.
 *  @param duration specifies the new value for the duration in track time
 *      scale units.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetTrackEditDuration()
 */
MP4V2_EXPORT
bool MP4SetTrackEditDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId,
    MP4Duration   duration );

/** Get the dwell value of a track edit segment.
 *
 *  MP4GetTrackEditDwell returns the dwell value of the specified track edit
 *  segment. A value of true (1) indicates that during this edit segment the
 *  media will be paused; a value of false (0) indicates that during this edit
 *  segment the media will be played at its normal rate.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment for which the dwell value is
 *      desired.
 *
 *  @return The dwell value of the edit segment of the track in the mp4 file.
 *
 *  @see MP4SetTrackEditDwell()
 */
MP4V2_EXPORT
int8_t MP4GetTrackEditDwell(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId );

/** Set the dwell value of a track edit segment.
 *
 *  MP4SetTrackEditDwell sets the dwell value of the specified edit segment
 *  from the specified track.
 *
 *  A value of true (1) indicates that during this edit segment the media will
 *  be paused; a value of false (0) indicates that during this edit segment the
 *  media will be played at its normal rate.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param editId specifies the edit segment to which the operation applies.
 *      Caveat: the first edit has id 1 not 0.
 *  @param dwell specifies the new dwell value.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4GetTrackEditDwell()
 */
MP4V2_EXPORT
bool MP4SetTrackEditDwell(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4EditId     editId,
    bool          dwell );

/** Read a track sample based on a specified edit list time.
 *
 *  MP4ReadSampleFromEditTime reads the sample corresponding to the time on the
 *  track edit list timeline from the specified track. Typically this sample is
 *  then decoded in a codec dependent fashion and rendered in an appropriate
 *  fashion.
 *
 *  The argument, <b>ppBytes</b>, allows for two possible approaches for
 *  buffering:
 *
 *  @li If the calling application wishes to handle its own buffering it can
 *      set <b>*ppBytes</b> to the buffer it wishes to use. The calling
 *      application is responsible for ensuring that the buffer is large enough
 *      to hold the sample. This can be done by using either MP4GetSampleSize()
 *      or MP4GetTrackMaxSampleSize() to determine beforehand how large the
 *      receiving buffer must be.
 *
 *  @li If the value of <b>*ppBytes</b> is NULL, then an appropriately sized
 *      buffer is automatically allocated for the sample data and
 *      <b>*ppBytes</b> set to this pointer. The calling application is
 *      responsible for freeing this memory with MP4Free().
 *
 *  The last four arguments are pointers to variables that can receive optional
 *  sample information. 
 *
 *  Typically for audio none of these are needed. MPEG audio such as MP3 or AAC
 *  has a fixed sample duration and every sample can be accessed at random.
 *
 *  For video, all of these optional values could be needed. MPEG video can be
 *  encoded at a variable frame rate, with only occasional random access
 *  points, and with "B frames" which cause the rendering (display) order of
 *  the video frames to differ from the storage/decoding order.
 *
 *  Other media types fall between these two extremes.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param when specifies which sample is to be read based on a time in the
 *      edit list timeline. See MP4GetSampleIdFromEditTime() for details.
 *  @param ppBytes Pointer to the pointer to the sample data. See function
 *      description above for details on this argument.
 *  @param pNumBytes Pointer to variable that will be hold the size in bytes of
 *      the sample.
 *  @param pStartTime If non-NULL, pointer to variable that will receive the
 *      starting timestamp for this sample. Caveat: The timestamp is in the
 *      track timescale.
 *  @param pDuration If non-NULL, pointer to variable that will receive the
 *      duration for this sample. Caveat: The duration is in the track
 *      timescale units.
 *  @param pRenderingOffset If non-NULL, pointer to variable that will receive
 *      the rendering offset for this sample. Currently the only media type
 *      that needs this feature is MPEG video. Caveat: The offset is in the
 *      track timescale units.
 *  @param pIsSyncSample If non-NULL, pointer to variable that will receive the
 *      state of the sync/random access flag for this sample.
 *
 *  @return Upon success, true (1). Upon an error, false (0).
 *
 *  @see MP4ReadSample()
 *  @see MP4GetSampleIdFromEditTime()
 *  @see MP4ReadSampleFromTime()
 */
MP4V2_EXPORT
bool MP4ReadSampleFromEditTime(
    /* input parameters */
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4Timestamp  when,
    /* input/output parameters */
    uint8_t**     ppBytes,
    uint32_t*     pNumBytes,
    /* output parameters */
    MP4Timestamp* pStartTime DEFAULT(NULL),
    MP4Duration*  pDuration DEFAULT(NULL),
    MP4Duration*  pRenderingOffset DEFAULT(NULL),
    bool*         pIsSyncSample DEFAULT(NULL) );

/** Get the sample id of a specified time in the edit list timeline.
 *
 *  MP4GetSampleIdFromEditTime returns the sample id of the track sample in
 *  which the specified time occurs in the edit list timeline. 
 *
 *  The specified time should be in the track time scale. See
 *  MP4ConvertToTrackTimestamp() for how to map a time value to this time
 *  scale.
 *
 *  Since the edit list can cause the sample start time and duration to be
 *  different that it in the standard track timeline, it is strongly advised
 *  that the caller retrieve the new sample start time and duration via this
 *  function.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param when specifies the time in the track time scale that is desired.
 *  @param pStartTime If non-NULL, pointer to variable that will receive the
 *      starting timestamp for this sample. Caveat: The timestamp is in the
 *      track edit list timescale.
 *  @param pDuration If non-NULL, pointer to variable that will receive the
 *      duration for this sample in the edit list timeline. Caveat: The
 *      duration is in the track timescale units.
 *
 *  @return Upon success, the sample id that occurs at the specified time. Upon
 *      an error, MP4_INVALID_SAMPLE_ID.
 *
 *  @see MP4GetSampleIdFromTime()
 */
MP4V2_EXPORT
MP4SampleId MP4GetSampleIdFromEditTime(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4Timestamp  when,
    MP4Timestamp* pStartTime DEFAULT(NULL),
    MP4Duration*  pDuration DEFAULT(NULL) );

/* time conversion utilties */

/* predefined values for timeScale parameter below */
#define MP4_SECONDS_TIME_SCALE      1
#define MP4_MILLISECONDS_TIME_SCALE 1000
#define MP4_MICROSECONDS_TIME_SCALE 1000000
#define MP4_NANOSECONDS_TIME_SCALE  1000000000

#define MP4_SECS_TIME_SCALE     MP4_SECONDS_TIME_SCALE
#define MP4_MSECS_TIME_SCALE    MP4_MILLISECONDS_TIME_SCALE
#define MP4_USECS_TIME_SCALE    MP4_MICROSECONDS_TIME_SCALE
#define MP4_NSECS_TIME_SCALE    MP4_NANOSECONDS_TIME_SCALE

/** Convert a duration from the movie (file) time scale to a specified time
 *      scale.
 *
 *  MP4ConvertFromMovieDuration converts a duration such as the total movie
 *  (file) duration from the movie time scale to another specified time scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param duration specifies the duration that is to be converted.
 *  @param timeScale specifies the new time scale in ticks per second to which
 *      the duration should be converted.
 *
 *  @return Upon success, the duration in the new time scale units. Upon error,
 *      (uint64_t) MP4_INVALID_DURATION.
 *
 *  @see MP4GetDuration()
 */
MP4V2_EXPORT
uint64_t MP4ConvertFromMovieDuration(
    MP4FileHandle hFile,
    MP4Duration   duration,
    uint32_t      timeScale );

/** Convert a timestamp from the track time scale to a specified time scale.
 *
 *  MP4ConvertFromTrackTimestamp converts a timestamp such as a sample start
 *  time from the track time scale to another specified time scale. This can be
 *  used by a player application to map all track samples to a common time
 *  scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track from which the timestamp originates.
 *  @param timeStamp specifies the timestamp that is to be converted.
 *  @param timeScale specifies the new time scale in ticks per second to which
 *      the timestamp should be converted.
 *
 *  @return Upon success, the timestamp in the new time scale units. Upon
 *      error, (uint64_t) MP4_INVALID_TIMESTAMP.
 *
 *  @see MP4ConvertToTrackTimestamp()
 */
MP4V2_EXPORT
uint64_t MP4ConvertFromTrackTimestamp(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4Timestamp  timeStamp,
    uint32_t      timeScale );

/** Convert a timestamp from a specified time scale to the track time scale.
 *
 *  MP4ConvertToTrackTimestamp converts a timestamp such as a sample start time
 *  from the specified time scale to the track time scale.
 *
 *  @param hFile specifies the mp4 file to which the operation applies.
 *  @param trackId specifies the track to which the operation applies.
 *  @param timeStamp specifies the timestamp that is to be converted.
 *  @param timeScale specifies the time scale in ticks per second in which the
 *      timestamp is currently expressed.
 *
 *  @return Upon success, the timestamp in the track time scale units. Upon
 *      error, MP4_INVALID_TIMESTAMP.
 *
 *  @see MP4ConvertFromTrackTimestamp()
 */
MP4V2_EXPORT
MP4Timestamp MP4ConvertToTrackTimestamp(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint64_t      timeStamp,
    uint32_t      timeScale );

/** Convert duration from track time scale to an arbitrary time scale.
 *
 *  MP4ConvertFromTrackDuration converts a duration such as a sample duration
 *  from the track time scale to another time scale. This can be used by a
 *  player application to map all track samples to a common time scale.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param duration value to be converted.
 *  @param timeScale time scale in ticks per second.
 *
 *  @return On success, the duration in arbitrary time scale units.
 *      On error, (uint64_t) MP4_INVALID_DURATION.
 *
 *  @see MP4GetSampleDuration()
 *  @see MP4ConvertToTrackDuration()
 */
MP4V2_EXPORT
uint64_t MP4ConvertFromTrackDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    MP4Duration   duration,
    uint32_t      timeScale );

/** Convert duration from arbitrary time scale to track time scale.
 *
 *  MP4ConvertToTrackDuration converts a duration such as a sample duration
 *  from the specified time scale to the track time scale.
 *
 *  @param hFile handle of file for operation.
 *  @param trackId id of track for operation.
 *  @param duration value to be converted.
 *  @param timeScale time scale in ticks per second.
 *
 *  @return On success, the duration in track time scale units.
 *      On error, #MP4_INVALID_DURATION.
 *
 *  @see MP4ConvertFromTrackDuration()
 */
MP4V2_EXPORT
MP4Duration MP4ConvertToTrackDuration(
    MP4FileHandle hFile,
    MP4TrackId    trackId,
    uint64_t      duration,
    uint32_t      timeScale );

/** Convert binary data to a base 16 string.
 *
 *  MP4BinaryToBase16 converts binary data to a base 16 string. This encoding
 *  maps groups of 4 bits into the character set [0-9a-f]. The string is in
 *  newly allocated memory, so the caller is responsible for freeing the memory
 *  with MP4Free().
 *
 *  This utility is useful for generating the SDP descriptions for some RTP
 *  payloads.
 *
 *  Example:
 *  @code
 *    0x12, 0xAB -> "12ab"
 *  @endcode
 *
 *  @param pData specifies the pointer to the binary data.
 *  @param dataSize specifies the size in bytes of the binary data.
 *
 *  @return Upon success, a null terminated string representing the data in
 *      base 16. Upon error, NULL.
 */
MP4V2_EXPORT
char* MP4BinaryToBase16(
    const uint8_t* pData,
    uint32_t       dataSize );

/** Convert binary data to a base 64 string.
 *
 *  MP4BinaryToBase64 converts binary data to a base 64 string. This encoding
 *  maps groups of 6 bits into the character set [A-Za-z0-9+/=]. The string is
 *  in newly allocated memory, so the caller is responsible for freeing the
 *  memory with MP4Free().
 *
 *  This utility is useful for generating the SDP descriptions for some RTP
 *  payloads.
 *
 *  Example:
 *  @code
 *    0x12, 0xAB -> "Eqs="
 *  @endcode
 *
 *  @param pData specifies the pointer to the binary data.
 *  @param dataSize specifies the size in bytes of the binary data.
 *
 *  @return Upon success, a null terminated string representing the data in
 *      base 64. Upon error, NULL.
 */
MP4V2_EXPORT
char* MP4BinaryToBase64(
    const uint8_t* pData,
    uint32_t       dataSize );

/** Free memory allocated by other library functions.
 *
 *  MP4Free frees a block of memory previously allocated by another library
 *  function.
 *
 *  Generally, library functions that allocate memory specify in their
 *  documentation whether MP4Free or another function must be used to free that
 *  memory.
 *
 *  @param p specifies a pointer to the memory block to free.
 */
MP4V2_EXPORT
void MP4Free(
    void* p );

/** Set the current log handler function.
 * 
 *  MP4SetLogCallback sets the function to call to output diagnostic
 *  information in place of the default log handler. The signature of the
 *  specified function must be compatible with the MP4LogCallback typedef.
 *
 *  @param cb_func specifies the new log handler function.
 *
 *  @see MP4LogCallback
 */
MP4V2_EXPORT
void MP4SetLogCallback(
    MP4LogCallback cb_func );

/** Get the current maximum log level.
 *
 *  MP4LogGetLevel returns the currently set maximum level of diagnostic
 *  information passed to the log handler.
 *
 *  @return the current maximum level of diagnostic information.
 *
 *  @see MP4LogSetLevel()
 */
MP4V2_EXPORT
MP4LogLevel MP4LogGetLevel( void );

/** Set the maximum log level.
 *
 *  MP4LogSetLevel sets the maximum level of diagnostic information passed to
 *  the current log handler.
 *
 *  @param verbosity specifies the log level to set.
 *
 *  @see MP4LogGetLevel()
 */
MP4V2_EXPORT
void MP4LogSetLevel(
    MP4LogLevel verbosity );

/** @} ***********************************************************************/

#endif /* MP4V2_GENERAL_H */
