/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2016 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

/*
 * Project: MPEG-4 ISO Base Media File Format (ISO BMFF) library
 * Content: C interface for ISO BMFF library
 */

/**
 * @file mmtisobmff_c.h
 *
 * @brief Main C-Interface of mmtisobmff library
 * \defgroup cinterface C-Interface for reading and writing
 * @{
 *
 * This is the main C-Interface. It contains all functions and definitions
 * to read and write MP4 files.
 *
 * @note The C-Interface is a wrapper around the C++ core. It only covers a fraction of the C++
 * interfaces. The biggest difference is that the C-interface does not support video track handling,
 * whereas the C++ interface has full support for this.
 */

#ifndef MMTISOBMFF_C_H
#define MMTISOBMFF_C_H

#ifdef __cplusplus
extern "C" {
#endif

#if (defined _WIN32 && _MSC_VER < 1700)
#include <stdio.h>
#define uint8_t unsigned __int8
#define uint16_t unsigned __int16
#define uint32_t unsigned __int32
#define uint64_t unsigned __int64
#define int16_t __int16
#define int64_t __int64
#else
#include <stdint.h>
#endif

#if (defined _WIN32 || defined __CYGWIN__) && defined MMTISOBMFF_BUILD_SHARED
#ifdef __GNUC__
#define MMTISOBMFF_DLL __attribute__((dllexport))
#else
#define MMTISOBMFF_DLL __declspec(dllexport)
#endif
#else
#if __GNUC__ >= 4
#define MMTISOBMFF_DLL __attribute__((visibility("default")))
#else
#define MMTISOBMFF_DLL
#endif
#endif

/**
 * @brief interface error values
 *
 * The error values that can be returned
 */
typedef enum {
  ISOBMFF_OK = 0,               /**< No error */
  ISOBMFF_UNKNOWN_ERR = 1,      /**< The error type is unknown */
  ISOBMFF_PARAM_ERR = 2,        /**< The used parameter is invalid */
  ISOBMFF_ALREADY_INIT_ERR = 3, /**< The called handle or resource is already created */
  ISOBMFF_MEMORY_ERR = 4,       /**< The library was not able to allocate the needed memory */
  ISOBMFF_NOT_INIT_ERR =
      5, /**< A function was called before creating the needed handle or resource */
  ISOBMFF_LIB_ERR =
      6, /**< The underlying C++ library returned an error. See error log for more details. */
  ISOBMFF_NOT_IMPL_ERR = 7 /**< Feature is not implemented */
} ISOBMFF_ERR;

/**
 * @brief C interface track types
 *
 * The track types supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the track type is not known by the library.
 */
typedef enum {
  TrackType_Undefined = 0, /**< Unknown track  */
  TrackType_Audio,         /**< Audio track */
  TrackType_Video,         /**< Video track */
  TrackType_Hint           /**< Hint track */
} TrackType_C;

/**
 * @brief C interface codecs
 *
 * The codec types supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the codec type is not known by the library.
 */
typedef enum {
  Codec_Undefined = 0, /**< Unknown codec */
  Codec_Mp4a = 4,      /**< MP4a based audio codec (AAC, HE-AAC, HE-AACv2, xHE-AAC, etc...) */
  Codec_Mpegh_Mha,     /**< MPEG-H MHA audio codec. RAW-AU in MP4 */
  Codec_Mpegh_Mhm,     /**< MPEG-H MHM audio codec. MHAS in MP4 */

  Codec_Mp4v = 65535, /**< MPEG4 video codecs */
  Codec_Avc,          /**< AVC/H.264 video codec */
  Codec_Hevc,         /**< HEVC/H.265 video codec */
  Codec_Jxs,          /**< JPEG XS video codec */
  Codec_Vvc           /**< VVC/H.266 video codec */
} Codec_C;

/**
 * @brief C interface sample groups
 *
 * The sample groups supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the sample group is not known by the library.
 * Sample groups are a concept to group samples together.
 */
typedef enum {
  SampleGroup_Undefined = 0, /**< Unknown sample group */
  SampleGroup_None = 1,      /**< No sample group */
  SampleGroup_Roll = 2,      /**< Sample group of type Roll-Recovery */
  SampleGroup_Prol = 3,      /**< Sample group of type Pre-Roll */
  SampleGroup_Sap = 4        /**< Sample group of type Stream-Access-Point */
} SampleGroup_C;

/**
 * @brief C interface stream access point types
 *
 * Stream Access Points (SAPs) define an entry point into an MP4 file and describe
 * what is to be expected when the stream is joined at an SAP sample.
 */
typedef enum {
  SapTypeUnknown = 0, /**< Unknown stream access type */
  SapType1 = 1,       /**< Indicates a sync sample (see codec standard)*/
  SapType2 = 2,       /**< Indicates a sync sample (see codec standard)*/
  SapType3 = 3,       /**< SAP is marked as a member of a sample group of type 'rap' */
  SapType4 =
      4, /**< SAP is marked as a member of a sample group of type 'roll', with roll distance > 0 */
  SapType5 = 5, /**< No specific signalling in isobmff standard supported */
  SapType6 = 6  /**< No specific signalling in isobmff standard supported */
} SapType_C;

/** @name forward declarations (reading interface) */
/**@{*/
struct ISOBMFF_Reader;
typedef struct ISOBMFF_Reader ISOBMFF_Reader;
/**< Object holding an isobmff movie reader instance */

struct TrackReader;
typedef struct TrackReader TrackReader;
/**< Object holding an isobmff track reader instance */
/**@}*/

/** @name forward declarations (writing interface) */
/**@{*/

struct ISOBMFF_Writer;
typedef struct ISOBMFF_Writer ISOBMFF_Writer;
/**< Object holding an isobmff movie writer instance */

struct TrackWriter;
typedef struct TrackWriter TrackWriter;
/**< Object holding an isobmff track writer instance */

struct TrackConfig;
typedef struct TrackConfig TrackConfig;
/**< Object holding the config for track creation */

struct IodsConfig;
typedef struct IodsConfig IodsConfig;
/**< Object holding the config for 'iods' creation */

struct SidxConfig;
typedef struct SidxConfig SidxConfig;
/**< Object holding the config for'sidx' creation */

struct MovieConfig;
typedef struct MovieConfig MovieConfig;
/**< Object holding the config for iso movie creation */

struct MpeghDecoderConfigRecord;
typedef struct MpeghDecoderConfigRecord MpeghDecoderConfigRecord;
/**< Object holding the config for an MPEG-H track */

struct Mp4aDecoderConfigRecord;
typedef struct Mp4aDecoderConfigRecord Mp4aDecoderConfigRecord;
/**<Object holding the config for an MP4a track */

struct EditListEntry;
typedef struct EditListEntry EditListEntry;
/**< Object holding the config for an edit list */

struct MpeghMultiStreamConfig;
typedef struct MpeghMultiStreamConfig MpeghMultiStreamConfig;
/**< Object holding extra config data for multi stream MPEG-H track creation */
/**@}*/

/** @name forward declarations (common interface) */
/**@{*/

struct Sample;
typedef struct Sample Sample;
/**< Object holding ab instance of a sample */
/**@}*/

/************* MMTISOBMFF LOGGING INTERFACE ********************/

/** @name MMTISOBMFF LOGGING INTERFACE */
/**@{*/

/**
 * @brief Redirects logging to the specified file.
 * @param [in] fileUri - must be non-NULL.
 * @returns ISOBMFF_OK if the redirection of the log to a file was successful.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_redirectLog(const char* fileUri);
/**
 * @brief Redirects logging and appends to the specified file.
 * @param [in] fileUri - must be non-NULL.
 * @returns ISOBMFF_OK if the redirection of the log to a file was successful.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_redirectLogAppend(const char* fileUri);
/**
 * @brief Redirects logging to console.
 * @returns ISOBMFF_OK if the redirection of the log to console was successful.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_redirectLogToConsole(void);
/**
 * @brief Redirects logging to the system logger.
 * @returns ISOBMFF_OK if the redirection of the log to the system logger was successful.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_redirectLogToSystemLogger(void);
/**
 * @brief Disables logging. Can be re-enabled by using any of the above specified redirectors.
 * @returns ISOBMFF_OK if disabling the logging was successful.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_disableLogging(void);

/**@}*/

/************* MMTISOBMFF READ INTERFACE ********************/

/** @name MMTISOBMFF READ INTERFACE */
/**@{*/

/**
 * @brief Creates a file based isobmff reader instance.
 *
 * This is the starting point for reading an MP4 file using the C interface.
 * @param [out] isobmffReader - an isobmff reader instance.
 * @param [in]  fileUri       - URI of the file to be read.
 * @returns ISOBMFF_OK if opening of the MP4 file and the initialization of the
 * reader were successful.
 *
 * @note Created ISOBMFF_Reader instance must be be destroyed with @ref isobmff_destroy.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createFileReader(ISOBMFF_Reader** isobmffReader,
                                                    const char* fileUri);
/**
 * @brief Creates a memory based isobmff reader instance.
 *
 * This is the starting point for reading MP4 memory segments using the C interface.
 *
 * @note The byte buffer passed in via the data variable must contain at least 1 complete
 * segment with the MP4 init segment (at least 'moov') prepended at the very beginning of the
 * buffer.\n
 * \b Example: <code><init><segment></code> or <code><init><segment><segment> ... <segment></code>
 *
 * @param [out] isobmffReader - an isobmff reader instance.
 * @param [in] dataBuffer     - MP4 media segment(s) with init segment prepended at the beginning of
 * the buffer
 * @param [in] size           - size of the data buffer in bytes.
 * @returns ISOBMFF_OK if MP4 memory segments parsing and the initialization of the
 * reader were successful.
 *
 * @note Created ISOBMFF_Reader instance must be be destroyed with @ref isobmff_destroy.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMemoryReader(ISOBMFF_Reader** isobmffReader,
                                                      const uint8_t* dataBuffer,
                                                      const uint64_t size);
/**
 * @brief Destroys the isobmff reader instance
 *
 * Calling this function will not invalidate other derived instances (e.g. samples or tracks).
 * They have to also be de-allocated by their respective destroy functions.
 *
 * @note No library interaction is allowed for this instance afterwards!
 *
 * @param [in] isobmffReader - an isobmff reader instance.
 * @returns ISOBMFF_OK if the reader instance was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroy(ISOBMFF_Reader* isobmffReader);
/**
 * @brief Gets the number of tracks reported in the MP4 file
 *
 * @param [in]  isobmffReader - an isobmff reader instance.
 * @param [out] trackCount    - number of tracks in the movie.
 * @returns ISOBMFF_OK if the number of tracks was queried successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackCount(ISOBMFF_Reader* isobmffReader,
                                                 uint32_t* trackCount);
/**
 * @brief Gets movie time scale of the movie.
 *
 * @param [in]  isobmffReader - an isobmff reader instance.
 * @param [out] timeScale     - movie time scale.
 * @returns ISOBMFF_OK if the movie time scale was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTimeScale(ISOBMFF_Reader* isobmffReader, uint32_t* timeScale);
/**
 * @brief Gets the number of the user data entries contained in the given movie.
 *
 * @param [in] isobmffReader  - an isobmff reader instance.
 * @param [out] count         - count of user data entries in the track.
 * @returns ISOBMFF_OK if the count of the user data entries contained in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getMovieUserDataEntryCount(ISOBMFF_Reader* isobmffReader,
                                                              uint32_t* count);
/**
 * @brief Retrieves the user data (of the specific index) contained in the movie
 *
 * @param [in] isobmffReader  - an isobmff reader instance.
 * @param [in] index          - the index of the user data entry (0-based).
 * @param [out] data          - buffer containing the user data: this buffer is formatted as an
 * isobmff box (or 'atom') specified in ISO/IEC 14496-12, Clause 4.2: 'size' [4 bytes] - 'type' (aka
 * Fourcc) [4 bytes] - payload ['size' bytes - 8]
 *                              \b Note: 'size' is a big-endian unsigned integer.
 * @param [out] size          - the size of the data buffer (same as the 'size' stored at the
 * beginning of 'data').
 * @returns ISOBMFF_OK if the user data was obtained successfully.
 *
 * @note The memory for the data pointer returned is managed by the library and shall not be freed
 * manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getMovieUserDataEntryByIndex(ISOBMFF_Reader* isobmffReader,
                                                                const uint32_t index,
                                                                uint8_t** data, uint32_t* size);
/**
 * @brief Gets AudioProfileLevelIndication of the'iods' box, if available.
 *
 * @param [in] isobmffReader                 - an isobmff reader instance.
 * @param [out] audioProfileLevelIndication  - the audioProfileLevelIndication stored in the 'iods'
 * box.
 * @param [out] isValid                      - a value of 1 indicates that 'iods' box exists and the
 * returned audioProfileLevelIndication is valid.
 *                                           - a value of 0 indicated that 'iods' box does not exist
 * and the returned audioProfileLevelIndication is invalid (not existing).
 * @returns ISOBMFF_OK if the AudioProfileLevelIndication was obtained successfully or there was no
 * 'iods' box.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getIodsAudioProfileLevelIndication(
    ISOBMFF_Reader* isobmffReader, uint8_t* audioProfileLevelIndication, uint8_t* isValid);
/**
 * @brief Gets the major brand of the MP4 file.
 *
 * @param [in] isobmffReader - an isobmff reader instance.
 * @param [out] majorBrand   - the major brand string.
 * @param [out] brandSize    - size of the major brand string.
 * @returns ISOBMFF_OK if the major brand string was obtained successfully.
 *
 * @note The memory for the majorBrand pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getMajorBrand(ISOBMFF_Reader* isobmffReader, char** majorBrand,
                                                 uint32_t* brandSize);
/**
 * @brief Gets the number of compatible brands of the MP4 file.
 *
 * @param [in] isobmffReader         - an isobmff reader instance.
 * @param [out] nrOfCompatibleBrands - number of available compatible brands.
 * @returns ISOBMFF_OK if the number of compatible brands was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getNrOfCompatibleBrands(ISOBMFF_Reader* isobmffReader,
                                                           uint32_t* nrOfCompatibleBrands);
/**
 * @brief Gets a major brand of the mp4 file.
 *
 * @param [in] isobmffReader        - an isobmff reader instance.
 * @param [in] brandIndex           - index of compatible brand to access (0-based)
 * @param [out] compatibleBrand     - the compatible brand string.
 * @param [out] compatibleBrandSize - size of the compatible brand string.
 * @returns ISOBMFF_OK if the specified compatible brand string was obtained successfully.
 *
 * @note The memory for the compatibleBrand pointer returned is managed by the library and shall not
 * be freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getCompatibleBrand(ISOBMFF_Reader* isobmffReader,
                                                      const uint32_t brandIndex,
                                                      char** compatibleBrand,
                                                      uint32_t* compatibleBrandSize);
/**
 * @brief Creates a track object that provides all the subsequent track related functionality.
 *
 * @param [in]  isobmffReader - an isobmff reader instance.
 * @param [out] trackReader   - a track reader object that provides all the track related
 * functionality.
 * @param [in]  trackIndex    - track index (0-based).
 * @returns ISOBMFF_OK if the track was obtained successfully.
 *
 * @note Created TrackReader instance must be be destroyed with @ref isobmff_destroyTrack.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrack(ISOBMFF_Reader* isobmffReader,
                                            TrackReader** trackReader, const uint32_t trackIndex);
/**
 * @brief Destroys a given track object.
 *
 * @param [in] trackReader - a track reader object.
 * @returns ISOBMFF_OK if the track object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyTrack(TrackReader* trackReader);
/**
 * @brief Gets track id of the given track.
 *
 * @param [in] trackReader - a track reader object.
 * @param [out] trackId    - track id.
 * @returns ISOBMFF_OK if the track id was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackId(TrackReader* trackReader, uint32_t* trackId);
/**
 * @brief Gets track type of the given track.
 *
 * @param [in]  trackReader - a track reader object.
 * @param [out] trackType   - track type.
 * @returns ISOBMFF_OK if the track type was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackType(TrackReader* trackReader, TrackType_C* trackType);
/**
 * @brief Gets track codec of the given track.
 *
 * @param [in]  trackReader - a track reader object.
 * @param [out] codec       - track codec.
 * @returns ISOBMFF_OK if the track codec was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackCodec(TrackReader* trackReader, Codec_C* codec);
/**
 * @brief Gets track handler of the given track.
 *
 * @param [in]  trackReader - a track reader object.
 * @param [out] handler     - track handler.
 * @param [out] handlerSize - handler size (size is less than or equal to 4).
 * @returns ISOBMFF_OK if the track handler was obtained successfully.
 *
 * @note The memory for the handler pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackHandler(TrackReader* trackReader, char** handler,
                                                   uint32_t* handlerSize);
/**
 * @brief Gets track coding name of the given track.
 *
 * @param [in]  trackReader    - a track reader object.
 * @param [out] codingName     - track coding name.
 * @param [out] codingNameSize - track coding name size (size is less than or equal to 4).
 * @returns ISOBMFF_OK if the track coding name was obtained successfully.
 *
 * @note The memory for the codingName pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackCodingName(TrackReader* trackReader, char** codingName,
                                                      uint32_t* codingNameSize);
/**
 * @brief Gets track duration of the given track.
 *
 * @param [in]  trackReader   - a track reader object.
 * @param [out] trackDuration - track duration.
 * @returns ISOBMFF_OK if the track duration was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackDuration(TrackReader* trackReader,
                                                    uint64_t* trackDuration);
/**
 * @brief Gets track time scale of the given track.
 *
 * @param [in]  trackReader    - a track reader object.
 * @param [out] trackTimeScale - track time scale.
 * @returns ISOBMFF_OK if the track time scale was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackTimeScale(TrackReader* trackReader,
                                                     uint32_t* trackTimeScale);
/**
 * @brief Gets maximum sample size of the track.
 *
 * @param [in]  trackReader   - a track reader object.
 * @param [out] maxSampleSize - maximum sample size of the track.
 * @returns ISOBMFF_OK if the maximum sample size of the track was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getMaxSampleSize(TrackReader* trackReader,
                                                    uint64_t* maxSampleSize);
/**
 * @brief Gets sample count of the given track.
 *
 * @param [in]  trackReader - a track reader object.
 * @param [out] sampleCount - sample count of the track.
 * @returns ISOBMFF_OK if the sample count of the track was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleCount(TrackReader* trackReader, uint64_t* sampleCount);
/**
 * @brief Gets the number of edit list entries contained in the given track.
 *
 * @param [in]  trackReader        - a track reader object.
 * @param [out] editListEntryCount - count of edit list entries in the track.
 * @returns ISOBMFF_OK if the count of the edit list entries contained in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getEditListEntryCount(TrackReader* trackReader,
                                                         uint32_t* editListEntryCount);
/**
 * @brief Gets the segment duration of an edit list entry in a given track.
 *
 * @param [in]  trackReader        - a track reader object.
 * @param [in]  editListEntryIndex - edit list entry index.
 * @param [out] segmentDuration    - segment duration of an edit list entry.
 * @returns ISOBMFF_OK if the segment duration of an edit list entry in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getEditListEntrySegmentDuration(
    TrackReader* trackReader, const uint32_t editListEntryIndex, uint64_t* segmentDuration);
/**
 * @brief Gets the media time of an edit list entry in a given track.
 *
 * @param [in]  trackReader        - a track reader object.
 * @param [in]  editListEntryIndex - edit list entry index.
 * @param [out] mediaTime          - media time of an edit list entry.
 * @returns ISOBMFF_OK if the media time of an edit list entry in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getEditListEntryMediaTime(TrackReader* trackReader,
                                                             const uint32_t editListEntryIndex,
                                                             int64_t* mediaTime);
/**
 * @brief Gets the media rate of an edit list entry in a given track.
 *
 * @param [in]  trackReader        - a track reader object.
 * @param [in]  editListEntryIndex - edit list entry index.
 * @param [out] mediaRate          - media rate of an edit list entry.
 * @returns ISOBMFF_OK if the media rate of an edit list entry in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getEditListEntryMediaRate(TrackReader* trackReader,
                                                             const uint32_t editListEntryIndex,
                                                             float* mediaRate);
/**
 * @brief Gets the number of user data entries contained in the given track.
 *
 * @param [in]  trackReader        - a track reader object.
 * @param [out] count              - count of user data entries in the track.
 * @returns ISOBMFF_OK if the count of the user data entries contained in the track was obtained
 * successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackUserDataEntryCount(TrackReader* trackReader,
                                                              uint32_t* count);
/**
 * @brief Retrieves the user data (of the specific index) contained in the track
 *
 * @param [in] trackReader - a track reader object.
 * @param [in] index       - the index of the user data entry (0-based).
 * @param [out] data       - buffer containing the user data: this buffer is formatted as a isobmff
 * box (or 'atom') specified in ISO/IEC 14496-12, Clause 4.2: 'size' [4 bytes] - 'type' (aka Fourcc)
 * [4 bytes] - payload ['size' - 8 bytes]
 *                           \b Note: 'size' is a big-endian unsigned integer.
 * @param [out] size       - the size of the data buffer (same as the 'size' stored at the beginning
 * of 'data').
 * @returns ISOBMFF_OK if the user data was obtained successfully.
 *
 * @note The memory for the data pointer returned is managed by the library and shall not be freed
 * manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackUserDataEntryByIndex(TrackReader* trackReader,
                                                                const uint32_t index,
                                                                uint8_t** data, uint32_t* size);
/**
 * @brief Gets initial 'ludt' loudness data ('tlou', 'alou', etc. concatenated) of this track.
 *
 * @param [in]  trackReader   - a track reader object.
 * @param [out] ludtInitData  - initial 'ludt' loudness data.
 * @param [out] size          - size of the initial 'ludt' loudness data. A size 0 indicates that
 * there is no initial 'ludt' data found on movie level for this track
 * @returns ISOBMFF_OK if the initial 'ludt' loudness data was obtained successfully.
 *
 * @note The memory for the ludtInitData pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getInitLudtData(TrackReader* trackReader, uint8_t** ludtInitData,
                                                   uint32_t* size);
/**
 * @brief Checks if this track has any 'ludt' updates (besides the 'ludt' data contained in the init
 * segment).
 *
 * @note It is enough to call this functions once for each track.
 *
 * @param [in] trackReader     - a track reader object.
 * @param [out] hasLudtUpdates - If 1, 'ludt' updates are available via fragments.
 * If 0, no updates are available for this track.
 * @returns ISOBMFF_OK if the value of hasLudtUpdates was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_hasLudtUpdates(TrackReader* trackReader,
                                                  uint8_t* hasLudtUpdates);
/**
 * @brief Gets 'ludt' loudness data from a specific fragment of a track.
 *
 * @note The function @ref isobmff_hasLudtUpdates should be called once before to
 * see if it is required to use this function at all.
 *
 * @param [in]  trackReader      - a track reader object.
 * @param [in]  fragmentNum      - Fragment number. The fragmentNum can be retrieved from the
 * sample. It is enough to issue a request once for each new fragment number.
 * @param [out] fragmentLudtData - fragment 'ludt' loudness data
 * @param [out] size             - Size of the 'ludt' loudness data. If size is 0, there is no
 * 'ludt' data found on fragment level for this track.
 * @returns ISOBMFF_OK if the 'ludt' loudness data from a specific fragment of a track was obtained
 * successfully.
 *
 * @note The memory for the fragmentLudtData pointer returned is managed by the library and shall
 * not be freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getFragmentLudtData(TrackReader* trackReader,
                                                       uint32_t fragmentNum,
                                                       uint8_t** fragmentLudtData, uint32_t* size);
/**
 * @brief Gets decoder specific config (configuration record, etc.) of the track
 *
 * @param [in] trackReader - a track reader object.
 * @param [out] dscData    - decoder specific config data.
 * @param [out] size       - size of the decoder specific config
 * @returns ISOBMFF_OK if the decoder specific config was obtained successfully.
 *
 * @note The memory for the dscData pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getDecoderSpecificConfig(TrackReader* trackReader,
                                                            uint8_t** dscData, uint32_t* size);
/**
 * @brief Gets the 3 character language type of the track
 *
 * @param [in] trackReader    - a track reader object.
 * @param [out] trackLanguage - language of this track
 * @param [out] trackLanguageSize  - size in bytes of the language string
 * @returns ISOBMFF_OK if the track language string was obtained successfully.
 *
 * @note The memory for the trackLanguage pointer returned is managed by the library and shall not
 * be freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getTrackLanguage(TrackReader* trackReader, char** trackLanguage,
                                                    uint32_t* trackLanguageSize);
/**
 * @brief Gets the sample rate of an audio track (stored in the audio sample entry box)
 *
 * @param [in] trackReader      - a track reader object.
 * @param [out] audioSamplerate - audio sample rate in Hz
 * @returns ISOBMFF_OK if the audio sample rate was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getAudioSampleRate(TrackReader* trackReader,
                                                      uint32_t* audioSamplerate);
/**
 * @brief Gets the channel count of an audio track. This is only valid for an mp4a audio track (e.g.
 * AAC).
 *
 * @note An MPEG-H track will always return 0 here.
 *
 * @param [in] trackReader        - a track reader object.
 * @param [out] audioChannelCount - audio channel count
 * @returns ISOBMFF_OK if the audio channel count rate was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getAudioChannelCount(TrackReader* trackReader,
                                                        uint32_t* audioChannelCount);
/**
 * @brief Gets the maxBitrate value from an MP4a audio track (e.g. AAC) stored in the config record.
 *
 * @note An MPEG-H track will always return 0 here.
 *
 * @param [in] trackReader          - a track reader object.
 * @param [out] mp4aAudioMaxBitrate - maximum audio bitrate value in bits per second of the encoded
 * data
 * @returns ISOBMFF_OK if the maximum audio bitrate was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getDcrMp4aAudioMaxBitrate(TrackReader* trackReader,
                                                             uint32_t* mp4aAudioMaxBitrate);
/**
 * @brief Gets the avgBitrate value from an MP4a audio track (e.g. AAC) stored in the config record.
 *
 * @note An MPEG-H track will always return 0 here.
 *
 * @param [in] trackReader          - a track reader object.
 * @param [out] mp4aAudioAvgBitrate - average audio bitrate value in bits per second of the encoded
 * data
 * @returns ISOBMFF_OK if the average audio bitrate was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getDcrMp4aAudioAvgBitrate(TrackReader* trackReader,
                                                             uint32_t* mp4aAudioAvgBitrate);
/**
 * @brief Gets the bufferSizeDb value from an MP4a audio track (e.g. AAC) stored in the config
 * record.
 *
 * @note An MPEG-H track will always return 0 here.
 *
 * @param [in] trackReader            - a track reader object.
 * @param [out] mp4aAudioBufferSizeDb - the size of the decoding buffer for the elementary stream in
 * bytes (ISO/IEC 14496-1).
 * @returns ISOBMFF_OK if the buffer size DB was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getDcrMp4aAudioBufferSizeDb(TrackReader* trackReader,
                                                               uint32_t* mp4aAudioBufferSizeDb);
/**
 * @brief Gets the next sample (requires an instantiated sample)
 *
 * @see isobmff_createSample.
 * @param [in] trackReader - a track reader object.
 * @param [out] sample     - sample.
 * @returns ISOBMFF_OK if the next sample in the track was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getNextSample(TrackReader* trackReader, Sample* sample);
/**
 * @brief Gets the sample by index (requires an instantiated sample)
 *
 * @note This also sets the index for the next call to @ref isobmff_getNextSample.
 *
 * @see isobmff_createSample.
 * @param [in] trackReader - a track reader object.
 * @param [out] sample     - sample.
 * @param [in]  index      - sample index.
 * @returns ISOBMFF_OK if the sample with the given index was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleByIndex(TrackReader* trackReader, Sample* sample,
                                                    const size_t index);

/**@}*/

/************* MMTISOBMFF WRITE INTERFACE ********************/

/** @name MMTISOBMFF WRITE INTERFACE */
/**@{*/

/**
 * @brief Creates an 'iods' config object (optional).
 *
 * This function is optional and needs only be called when the 'iods' box should be written.
 * @see isobmff_setIodsConfig
 * @param [out] iodsConfig - an 'iods' config object.
 * @returns ISOBMFF_OK if the 'iods' config object was successfully created.
 *
 * @note Created isobmff_createIodsConfig object must be be destroyed with @ref
 * isobmff_destroyIodsConfig.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createIodsConfig(IodsConfig** iodsConfig);
/**
 * @brief Sets the audio profile level indication.
 *
 * @see isobmff_createIodsConfig
 * @param [in] iodsConfig - an iods config object.
 * @param [in] audioProfileLevelIndication - value of the audio profile and level indication
 * @returns ISOBMFF_OK if the major brand was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setAudioProfileLevelIndication(
    IodsConfig* iodsConfig, const uint8_t audioProfileLevelIndication);
/**
 * @brief Destroys an iods config object.
 *
 * @see isobmff_createIodsConfig
 * @param [in] iodsConfig - an iods config object.
 * @returns ISOBMFF_OK if the iods config object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyIodsConfig(IodsConfig* iodsConfig);
/**
 * @brief Creates an 'sidx' config object (optional).
 *
 * This function is optional and needs only be called when the 'sidx' box should be written.
 * @see isobmff_setSidxConfig
 * @param [out] sidxConfig - an 'sidx' config object.
 * @returns ISOBMFF_OK if the 'sidx' config object was created successfully.
 *
 * @note Created SidxConfig object must be be destroyed with @ref isobmff_destroySidxConfig.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createSidxsConfig(SidxConfig** sidxConfig);
/**
 * @brief Sets the 'sidx' stream access point type
 *
 * @see isobmff_createSidxConfig
 * @see SapType_C
 * @param [in] sidxConfig - an 'sidx' config object.
 * @param [in] sapType    - the stream access type;
 * @returns ISOBMFF_OK if the sap type was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setStreamAccessPointType(SidxConfig* sidxConfig,
                                                            const SapType_C sapType);
/**
 * @brief Destroys an 'sidx' config object.
 *
 * @see isobmff_createSidxConfig
 * @param [in] sidxConfig - an 'sidx' config object.
 * @returns ISOBMFF_OK if the 'sidx' config object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroySidxConfig(SidxConfig* sidxConfig);
/**
 * @brief Creates a movie config object.
 *
 * This config struct holds the global configuration of an MP4 file (movie section). It is always
 * required when writing an MP4 file.
 * @param [out] movieConfig - a movie config object.
 * @returns ISOBMFF_OK if the movie config object was successfully created.
 *
 * @note Created MovieConfig object must be be destroyed with @ref isobmff_destroyMovieConfig.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMovieConfig(MovieConfig** movieConfig);
/**
 * @brief Sets the major brand.
 *
 * @note The input majorBrand shall be 4 characters long. Meaning that the size shall always be 4!
 *
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig - a movie config object.
 * @param [in] majorBrand  - major brand to be set in the movie config object.
 * @param [in] size        - size of the major brand (max 4 chars).
 * @returns ISOBMFF_OK if the major brand was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setMajorBrand(MovieConfig* movieConfig, const char* majorBrand,
                                                 const uint32_t size);
/**
 * @brief Adds a compatible brand.
 *
 * Movie config can have several compatible brands. Therefore, this function
 * can be called multiple times to add multiple compatible brands.\n
 *
 * @note The brands must be unique.
 * @note The input compatibleBrand should be 4 characters long. Meaning that the size should always
 * be 4!
 *
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig     - movie config object.
 * @param [in] compatibleBrand - compatible brand to be set in the movie config object.
 * @param [in] size            - size of the major brand (max 4 chars).
 * @returns ISOBMFF_OK if the compatible brand was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addCompatibleBrand(MovieConfig* movieConfig,
                                                      const char* compatibleBrand,
                                                      const uint32_t size);
/**
 * @brief Sets movie time scale of the movie (optional)
 *
 * This function is optional. If it is not called a default value of 600 will be used.\n
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig - a movie config object.
 * @param [in] timeScale   - movie time scale.
 * @returns ISOBMFF_OK if movie time scale was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setTimeScale(MovieConfig* movieConfig, const uint32_t timeScale);
/**
 * @brief Sets the current time in UTC (optional)
 *
 * This function is optional. If it is not called the value will be auto generated.\n
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig      - a movie config object.
 * @param [in] currentTimeInUTC - the current time in UTC.
 * @returns ISOBMFF_OK if the current time in UTC was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setcurrentTimeInUtc(MovieConfig* movieConfig,
                                                       const uint64_t currentTimeInUTC);
/**
 * @brief Forces base media decode time to be 64bit (optional).
 *
 * This function is optional. If used the 'tfdt' box will be forced to version 1.
 *
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig - a movie config object.
 * @param [in] use64bitMDT - a flag to force base media decode time to 64bit. Only allowed values
 * are: 1 or 0.
 * @returns ISOBMFF_OK if the 64bit base media decode time flag was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_force64bitMediaDecodeTime(MovieConfig* movieConfig,
                                                             const uint8_t use64bitMDT);
/**
 * @brief Sets the 'iods' config (optional)
 *
 * This function is optional. It is used to create an 'iods' box. The config must be filled before.
 * If not set the 'iods' box is not written.\n
 * @see isobmff_createMovieConfig
 * @see isobmff_createIodsConfig
 * @param [in] movieConfig - a movie config object.
 * @param [in] iodsConfig  - an 'iods' config object.
 * @returns ISOBMFF_OK if the 'iods' config was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setIodsConfig(MovieConfig* movieConfig,
                                                 const IodsConfig* iodsConfig);
/**
 * @brief Sets the 'sidx' config (optional)
 *
 * This function is optional. It is used to create an 'sidx' box for fragmented file writing.
 * It can only be used with the @ref isobmff_createFragFileWriter. The config must be filled before.
 * If not set the 'sidx' box is not written.
 *
 * @see isobmff_createMovieConfig
 * @see isobmff_createSidxConfig
 * @param [in] movieConfig - a movie config object.
 * @param [in] sidxConfig  - an 'sidx' config object.
 * @returns ISOBMFF_OK if the 'sidx' config was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSidxConfig(MovieConfig* movieConfig,
                                                 const SidxConfig* sidxConfig);
/**
 * @brief Add user defined data to the 'moov'.
 *
 * The data will be stored as part of the 'udta' box carried in the 'moov' container.
 * Each call to this function will result in a child box of the 'udta' container box. This config is
 * optional.
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig   - a movie config object.
 * @param [in] data          - buffer containing the user data: this buffer is expected to be
 * formatted as a isobmff box (or 'atom') specified in ISO/IEC 14496-12, Clause 4.2: 'size' [4
 * bytes] - 'type' (aka Fourcc) [4 bytes] - payload ['size' - 8 bytes]
 *                             \b Note: 'size' is a big-endian unsigned integer.
 * @param [in] size          - the size of the data buffer (same as the 'size' stored at the
 * beginning of 'data').
 * @returns ISOBMFF_OK if the user data was successfully added to the movie config.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addMovieUserDataEntry(MovieConfig* movieConfig,
                                                         const uint8_t* data, const uint32_t size);
/**
 * @brief Destroys a movie config object.
 *
 * @see isobmff_createMovieConfig
 * @param [in] movieConfig - a movie config object.
 * @returns ISOBMFF_OK if the movie config object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyMovieConfig(MovieConfig* movieConfig);
/**
 * @brief Creates a fragmented MP4 file writer.
 *
 * This function must be called for writing fragmented MP4 files (one file with fragments).
 * It is one of the entry points for writing MP4 files.
 *
 * All samples added later will be written into one output file, but registered in their respective
 * fragments.
 *
 * The fragment number (where the samples should be added) can be configured directly on the sample
 * (see sample definition @ref Sample)
 *
 * A new segment (group of fragments) can be written by calling @ref isobmff_newMediaSegment
 *
 * If 'sidx' type is configured in @ref MovieConfig than each call to @ref isobmff_newMediaSegment
 * creates a new 'sidx' entry. This then creates a segmented MP4, but as one file (opposed to file
 * segments created with @ref isobmff_createFragFileSegWriter)
 * @see isobmff_createMovieConfig
 * @param [out] isobmffWriter - an isobmff writer instance.
 * @param [in]  movieConfig   - a movie config object.
 * @param [in]  outFileUri    - output file uri.
 * @returns ISOBMFF_OK if a fragmented MP4 file writer was successfully created.
 *
 * @note Created ISOBMFF_Writer instance must be be destroyed with @ref isobmff_destroyWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createFragFileWriter(ISOBMFF_Writer** isobmffWriter,
                                                        const MovieConfig* movieConfig,
                                                        const char* outFileUri);
/**
 * @brief Creates a segment of fragments
 *
 * When a segment is created all samples added to the library by @ref isobmff_addSample are written
 * into a continuous fragmented MP4 file (uri is set at the @ref isobmff_createFragFileWriter
 * interface).\n A segment can contain multiple fragments, but usually it contains only one.\n
 *
 * @note Calling this function does nothing besides writing the fragments to disk if the library is
 * not configured to write an'sidx' box.
 *
 * @see isobmff_createFragFileWriter
 * @param [in] isobmffWriter - an isobmff writer instance.
 * @returns ISOBMFF_OK if the new media segment was successfully created and written.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_newMediaSegment(ISOBMFF_Writer* isobmffWriter);
/**
 * @brief Creates a fragmented MP4 file segment writer.
 *
 * This function must be called for writing file segmented MP4 files (separate files).\n
 * It is one of the entry points for writing MP4 files.\n
 * This mode produces an initialization segment (containing the 'moov' box) and several media
 * segments (containing one or many 'moof' + 'mdat' pairs). A segment can have multiple fragments,
 * but usually a segment holds one fragment.
 * @see isobmff_createMovieConfig
 * @param [out] isobmffWriter - an isobmff writer instance.
 * @param [in]  movieConfig   - a movie config object.
 * @returns ISOBMFF_OK if the fragmented MP4 file segment writer was successfully created.
 *
 * @note Created ISOBMFF_Writer instance must be be destroyed with @ref isobmff_destroyWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createFragFileSegWriter(ISOBMFF_Writer** isobmffWriter,
                                                           const MovieConfig* movieConfig);
/**
 * @brief Creates the init segment and writes it to a file located at outFileUri
 *
 * This call creates the initialization segment containing the 'moov' box. It holds all the static
 * information, but no samples.
 *
 * @see isobmff_createFragFileSegWriter
 * @param [in] isobmffWriter - an isobmff writer instance.
 * @param [in] outFileUri    - output file uri.
 * @returns ISOBMFF_OK if the init segment was successfully created and written.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_newInitFileSegment(ISOBMFF_Writer* isobmffWriter,
                                                      const char* outFileUri);
/**
 * @brief Creates a media segment and writes it to a file located at outFileUri
 *
 * This call creates the media segment. It writes all the added samples since its last call into a
 * file segment. If the added samples belong to different fragments, the resulting media segment
 * will contain multiple fragments ('moof' + 'mdat' pairs). The last segment must be signalled via
 * isLastSegment.\n Usually segments contain one fragment. To achieve this, just make sure that the
 * function is called before adding a sample of a new fragment.\n
 * @see isobmff_createFragFileSegWriter
 * @param [in] isobmffWriter - an isobmff writer instance.
 * @param [in] outFileUri    - output file uri.
 * @param [in] isLastSegment - the last segment flag. The flag value must be either 1 or 0.
 * @returns ISOBMFF_OK if the media segment was successfully created and written.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_newMediaFileSegment(ISOBMFF_Writer* isobmffWriter,
                                                       const char* outFileUri,
                                                       const uint8_t isLastSegment);
/**
 * @brief Creates a fragmented MP4 memory writer.
 *
 * This function must be called for creating file segments in memory.
 *
 * It is one of the entry points for writing MP4 files. This mode does not write the data to disc
 * but uses an API to retrieve the segments as a ByteBuffer.
 * @see isobmff_createMovieConfig
 * @param [out] isobmffWriter - an isobmff writer instance.
 * @param [in]  movieConfig   - a movie config object.
 * @returns ISOBMFF_OK if the fragmented MP4 memory writer was successfully created.
 *
 * @note Created ISOBMFF_Writer instance must be be destroyed with @ref isobmff_destroyWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createFragMemoryWriter(ISOBMFF_Writer** isobmffWriter,
                                                          const MovieConfig* movieConfig);
/**
 * @brief Creates the init segment and provides it via a buffer + size interface
 *
 * This call creates the initialization segment containing the 'moov' box. It works similar to
 * @ref isobmff_newInitFileSegment, but returns the data as buffer instead of writing it to a file.
 * @see isobmff_createFragMemoryWriter
 * @param [in]  isobmffWriter - an isobmff writer instance.
 * @param [out] dataBuffer    - the data buffer of the init segment.
 * @param [out] size          - size of the data buffer.
 * @returns ISOBMFF_OK if the init segment was successfully created and the data buffer and the
 * size were successfully provided.
 *
 * @note The memory for the dataBuffer pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_newInitMemorySegment(ISOBMFF_Writer* isobmffWriter,
                                                        uint8_t** dataBuffer, uint64_t* size);
/**
 * @brief Creates the media segment and provides it via a buffer + size interface
 *
 * This call creates the media segment containing the 'moof' and 'mdat'. It works similar to
 * @ref isobmff_newMediaFileSegment, but returns the data as buffer instead of writing it to a file.
 * @see isobmff_createFragMemoryWriter
 * @param [in]  isobmffWriter - an isobmff writer instance.
 * @param [out] dataBuffer    - the data buffer of the media segment.
 * @param [out] size          - size of the data buffer.
 * @param [in]  isLastSegment - the last segment flag. The flag value must be either 1 or 0.
 * @returns ISOBMFF_OK if the media segment was successfully created and the data buffer and the
 * size were successfully provided.
 *
 * @note The memory for the dataBuffer pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_newMediaMemorySegment(ISOBMFF_Writer* isobmffWriter,
                                                         uint8_t** dataBuffer, uint64_t* size,
                                                         const uint8_t isLastSegment);
/**
 * @brief Creates a non-fragmented (plain) MP4 file writer
 *
 * It is one of the entry points for writing MP4 files. This mode creates a standard
 * (plain) MP4 files without fragments. The atom order is suitable for storage and
 * progressive download. This mode requires the creation of a temporary file that can
 * either be auto created at the systems default tmp folder or specified by the user via
 * tmpFileUri. The temoprary file is automatically deleted when the process is finished.
 * @see isobmff_createMovieConfig
 * @param [out] isobmffWriter         - an isobmff writer instance.
 * @param [in]  movieConfig           - a movie config object.
 * @param [in]  outFileUri            - output file uri.
 * @param [in]  tmpFileUri (optional) - temporary output file uri. If the tmpFileUri is a null
 * pointer, a default temporary file uri is generated.
 * @returns ISOBMFF_OK if a non-fragmented MP4 file writer was successfully created.
 *
 * @note Created ISOBMFF_Writer instance must be be destroyed with @ref isobmff_destroyWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createFileWriter(ISOBMFF_Writer** isobmffWriter,
                                                    MovieConfig* movieConfig,
                                                    const char* outFileUri, const char* tmpFileUri);
/**
 * @brief Creates a non-fragmented (plain) MP4 memory writer
 *
 * It is one of the entry points for writing MP4 files. This mode creates a standard
 * (plain) MP4 files without fragments. The atom order is suitable for storage and
 * progressive download. This mode creates the complete MP4 file in memory.
 * @see isobmff_createMovieConfig
 * @param [out] isobmffWriter         - an isobmff memory writer instance.
 * @param [in]  movieConfig           - a movie config object.
 * @returns ISOBMFF_OK if a non-fragmented MP4 memory writer was successfully created.
 *
 * @note Created ISOBMFF_Writer instance must be be destroyed with @ref isobmff_destroyWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMemoryWriter(ISOBMFF_Writer** isobmffWriter,
                                                      MovieConfig* movieConfig);
/**
 * @brief Serializes the complete plain/non-fragmented MP4 data structure to a memory buffer
 *
 * This call creates a serialized byte representation of an MP4 file and stores it in a byte buffer.
 * @see isobmff_createMemoryWriter
 * @param [in]  isobmffWriter - an isobmff writer instance.
 * @param [out] dataBuffer    - the data buffer the serialized MP4 file is stored in.
 * @param [out] size          - size of the data buffer.
 * @returns ISOBMFF_OK if the MP4 file was successfully serialized and the data buffer and the
 * size were successfully provided.
 *
 * @note The memory for the dataBuffer pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_serializeToMemoryBuffer(ISOBMFF_Writer* isobmffWriter,
                                                           uint8_t** dataBuffer, uint64_t* size);
/**
 * @brief Destroys the isobmff writer
 *
 * @see isobmff_destroyWriter
 * @param [in] isobmffWriter - an isobmff writer instance
 * @returns ISOBMFF_OK if the isobmff writer instance was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyWriter(ISOBMFF_Writer* isobmffWriter);
/**
 * @brief Creates an MPEG-H decoder config record object
 *
 * @param [out] mpeghDcr - an MPEG-H decoder config record object.
 * @returns ISOBMFF_OK if the MPEG-H decoder config record object was successfully created.
 *
 * @note Created MpeghDecoderConfigRecord object must be be destroyed with @ref
 * isobmff_destroyMpeghDecoderConfigRecord.
 */
MMTISOBMFF_DLL ISOBMFF_ERR
isobmff_createMpeghDecoderConfigRecord(MpeghDecoderConfigRecord** mpeghDcr);
/**
 * @brief Sets the decoder config record configuration version.
 *
 * @see isobmff_createMpeghDecoderConfigRecord
 * @param [in] mpeghDcr             - an MPEG-H decoder config record object.
 * @param [in] configurationVersion - the decoder config record configuration version.
 * @returns ISOBMFF_OK if the decoder config record configuration version was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrConfigurationVersion(MpeghDecoderConfigRecord* mpeghDcr,
                                                              const uint8_t configurationVersion);
/**
 * @brief Sets the decoder config record profile level indication.
 *
 * @see isobmff_createMpeghDecoderConfigRecord
 * @param [in] mpeghDcr               - an MPEG-H decoder config record object.
 * @param [in] profileLevelIndication - the decoder config record profile level indication.
 * @returns ISOBMFF_OK if the decoder config record profile level indication was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrProfileLevelIndication(
    MpeghDecoderConfigRecord* mpeghDcr, const uint8_t profileLevelIndication);
/**
 * @brief Sets the decoder config record reference channel layout.
 *
 * @see isobmff_createMpeghDecoderConfigRecord
 * @param [in] mpeghDcr               - an MPEG-H decoder config record object.
 * @param [in] referenceChannelLayout - the decoder config record reference channel layout.
 * @returns ISOBMFF_OK if the decoder config record reference channel layout was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrReferenceChnlLayout(MpeghDecoderConfigRecord* mpeghDcr,
                                                             const uint8_t referenceChannelLayout);
/**
 * @brief Sets the decoder config record mpegh3da config.
 *
 * @see isobmff_createMpeghDecoderConfigRecord
 * @param [in] mpeghDcr       - an MPEG-H decoder config record object.
 * @param [in] mpegh3daConfig - the mpegh3da config data.
 * @param [in] size           - the mpegh3da config data size.
 * @returns ISOBMFF_OK if the decoder config record mpegh3da config was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrMpegh3daConfig(MpeghDecoderConfigRecord* mpeghDcr,
                                                        const uint8_t* mpegh3daConfig,
                                                        const uint64_t size);
/**
 * @brief Destroys the MPEG-H decoder config record object.
 *
 * @see isobmff_createMpeghDecoderConfigRecord
 * @param [in] mpeghDcr - an MPEG-H decoder config record object.
 * @returns ISOBMFF_OK if the mpegh decoder config record object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR
isobmff_destroyMpeghDecoderConfigRecord(MpeghDecoderConfigRecord* mpeghDcr);
/**
 * @brief Creates an mp4a decoder config record object.
 *
 * @param [out] mp4aDcr - an mp4a decoder config record object.
 * @returns ISOBMFF_OK if the mp4a decoder config record object was successfully created.
 *
 * @note Created Mp4aDecoderConfigRecord object must be be destroyed with @ref
 * isobmff_destroyMp4aDecoderConfigRecord.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMp4aDecoderConfigRecord(Mp4aDecoderConfigRecord** mp4aDcr);
/**
 * @brief Sets the decoder config record max bitrate value.
 *
 * @see isobmff_createMp4aDecoderConfigRecord
 * @param [in] mp4aDcr    - an mp4a decoder config record object.
 * @param [in] maxBitrate - the decoder config record max bitrate value.
 * @returns ISOBMFF_OK if the decoder config record max bitrate value was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrMaxBitrate(Mp4aDecoderConfigRecord* mp4aDcr,
                                                    const uint32_t maxBitrate);
/**
 * @brief Sets the decoder config record average bitrate value.
 *
 * @see isobmff_createMp4aDecoderConfigRecord
 * @param [in] mp4aDcr    - an mp4a decoder config record object.
 * @param [in] avgBitrate - the decoder config record average bitrate value.
 * @returns ISOBMFF_OK if the decoder config record average bitrate value was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrAvgBitrate(Mp4aDecoderConfigRecord* mp4aDcr,
                                                    const uint32_t avgBitrate);
/**
 * @brief Sets the decoder config record buffer size db value.
 *
 * @see isobmff_createMp4aDecoderConfigRecord
 * @param [in] mp4aDcr    - an mp4a decoder config record object.
 * @param [in] bufferSizeDB - the decoder config record buffer size db value.
 * @returns ISOBMFF_OK if the decoder config record buffer size db value was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrBufferSizeDB(Mp4aDecoderConfigRecord* mp4aDcr,
                                                      const uint32_t bufferSizeDB);
/**
 * @brief Sets the decoder config record ASC.
 *
 * @see isobmff_createMp4aDecoderConfigRecord
 * @param [in] mp4aDcr  - an mp4a decoder config record object.
 * @param [in] ascData  - the ASC data.
 * @param [in] size     - the ASC data size.
 * @returns ISOBMFF_OK if the decoder config record asc was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrAsc(Mp4aDecoderConfigRecord* mp4aDcr,
                                             const uint8_t* ascData, const uint64_t size);
/**
 * @brief Destroys the mp4a decoder config record object.
 *
 * @param [in] mp4aDcr - an mp4a decoder config record object.
 * @returns ISOBMFF_OK if the mp4a decoder config record object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyMp4aDecoderConfigRecord(Mp4aDecoderConfigRecord* mp4aDcr);
/**
 * @brief Creates an edit list entry object (can be re-used).
 *
 * @param [out] editListEntry - an edit list entry object.
 * @returns ISOBMFF_OK if the edit list entry object was successfully created.
 *
 * @note Created EditListEntry object must be be destroyed with @ref isobmff_destroyEditListEntry.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createEditListEntry(EditListEntry** editListEntry);
/**
 * @brief Sets the segment duration of an edit list entry.
 *
 * @see isobmff_createEditListEntry
 * @param [in] editListEntry   - an edit list entry object.
 * @param [in] segmentDuration - the segment duration of the edit list entry.
 * @returns ISOBMFF_OK if the segment duration was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setEditListEntrySegmentDuration(EditListEntry* editListEntry,
                                                                   const uint64_t segmentDuration);
/**
 * @brief Sets the media time of an edit list entry.
 *
 * @see isobmff_createEditListEntry
 * @param [in] editListEntry - an edit list entry object.
 * @param [in] mediaTime     - the media time of the edit list entry.
 * @returns ISOBMFF_OK if the media time was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setEditListEntryMediaTime(EditListEntry* editListEntry,
                                                             const int64_t mediaTime);
/**
 * @brief Sets the media rate of an edit list entry.
 *
 * @see isobmff_createEditListEntry
 * @param [in] editListEntry - an edit list entry object.
 * @param [in] mediaRate     - the media rate of the edit list entry.
 * @returns ISOBMFF_OK if the media rate was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setEditListEntryMediaRate(EditListEntry* editListEntry,
                                                             const float mediaRate);
/**
 * @brief Destroys the edit list entry object
 *
 * @note Call this function after adding all editListEntries to the track to clean up the memory
 * allocated by editListEntry.
 *
 * @param [in] editListEntry - an edit list entry object.
 * @returns ISOBMFF_OK if the edit list entry object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyEditListEntry(EditListEntry* editListEntry);
/**
 * @brief Creates an MPEG-H multi stream config object
 *
 * @note The created config can be added to a track without setting any of the other optional
 * setters for this config.\n Setting such an "empty" config to a track directly after calling this
 * create function enables basic multi stream writing support.
 *
 * @param [out] mpeghMsc - an MPEG-H multi stream config
 * @returns ISOBMFF_OK if the MPEG-H multi stream config object was successfully created.
 *
 * @note Created MpeghMultiStreamConfig object must be be destroyed with @ref
 * isobmff_destroyMpeghMultiStreamConfig.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMpeghMultiStreamConfig(MpeghMultiStreamConfig** mpeghMsc);
/**
 * @brief Destroys the MPEG-H multi stream config object
 *
 * @note Call this function after setting the config to a track to clean up.
 *
 * @param [in] mpeghMsc - an MPEG-H multi stream config object
 * @returns ISOBMFF_OK if the MPEG-H multi stream config object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyMpeghMultiStreamConfig(MpeghMultiStreamConfig* mpeghMsc);
/**
 * @brief Creates a track config object.
 *
 * @param [out] trackConfig - a track config object.
 * @returns ISOBMFF_OK if the track config object was successfully created.
 *
 * @note Created TrackConfig object must be be destroyed with @ref isobmff_destroyTrackConfig.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createTrackConfig(TrackConfig** trackConfig);
/**
 * @brief Sets the track codec.
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] codec       - the track codec.
 * @returns ISOBMFF_OK if the track codec was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setTrackCodec(TrackConfig* trackConfig, const Codec_C codec);
/**
 * @brief Sets the track time scale.
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig    - a track config object.
 * @param [in] trackTimeScale - the track time scale.
 * @returns ISOBMFF_OK if the track time scale was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setTrackTimeScale(TrackConfig* trackConfig,
                                                     const uint32_t trackTimeScale);
/**
 * @brief Sets the track id. (optional - is auto-computed by default)
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] trackId     - the track id.
 * @returns ISOBMFF_OK if the track id was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setTrackId(TrackConfig* trackConfig, const uint32_t trackId);
/**
 * @brief Sets the sample rate of an audio codec
 *
 * Call this when using any audio codecs.
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] sampleRate  - the track sample rate.
 * @returns ISOBMFF_OK if the track sample rate was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setAudioSampleRate(TrackConfig* trackConfig,
                                                      const uint32_t sampleRate);
/**
 * @brief Sets the number of audio channels.
 *
 * @note Setting this has not effects for MPEG-H tracks.
 *
 * Call this when using mp4a based audio codecs (AAC family).
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig       - a track config object.
 * @param [in] audioChannelCount - the number of audio channels in the track.
 * @returns ISOBMFF_OK if the number of audio channels was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setAudioChannelCount(TrackConfig* trackConfig,
                                                        const uint16_t audioChannelCount);
/**
 * @brief Sets the audio track language. (optional - default is "und")
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] language    - the audio track language.
 * @param [in] size        - the language size. The language must be 3 characters long!
 * @returns ISOBMFF_OK if the audio track language was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setAudioTrackLanguage(TrackConfig* trackConfig,
                                                         const char* language, const uint32_t size);
/**
 * @brief Sets the MPEG-H decoder config record (only mandatory for MHA tracks).
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] mpeghDcr    - the MPEG-H decoder config record.
 * @returns ISOBMFF_OK if the MPEG-H decoder config record was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setMpeghDecoderConfigRecord(
    TrackConfig* trackConfig, const MpeghDecoderConfigRecord* mpeghDcr);
/**
 * @brief Sets the mp4a decoder config record.
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] mp4aDcr     - the mp4a decoder config record.
 * @returns ISOBMFF_OK if the mp4a decoder config record was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setMp4aDecoderConfigRecord(
    TrackConfig* trackConfig, const Mp4aDecoderConfigRecord* mp4aDcr);
/**
 * @brief Creates a default sample group table ('sgpd') in the track.
 *
 * If samples are added belonging to this sample group, the 'sgpd' table is only
 * written once for this type and not repeated in fragments.
 *
 * @note This is only useful for fragmented MP4 files and reduces the MP4 overhead.
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig     - a track config object.
 * @param [in] sampleGroupType - the sample group type.
 * @param [in] rollDistance    - the roll distance.
 * @returns ISOBMFF_OK if sample group and value were successfully set.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDefaultSampleGroup(TrackConfig* trackConfig,
                                                         const SampleGroup_C sampleGroupType,
                                                         const int16_t rollDistance);
/**
 * @brief Sets the MPEG-H multi stream config.
 *
 * If set, the library will produce a SampleEntry of type mhm2 needed for multi stream support.
 *
 * @note This config is required to configure the library for MPEG-H multi stream support.
 *          This is currently only supported for Codec_Mpegh_Mhm.
 *
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] mpeghMsc    - the MPEG-H decoder multi stream config.
 * @returns ISOBMFF_OK if the MPEG-H decoder multi stream config was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR
isobmff_setMpeghMultiStreamConfig(TrackConfig* trackConfig, const MpeghMultiStreamConfig* mpeghMsc);
/**
 * @brief Sets the MPEG-H Profile and Level Compatibility Sets.
 *
 * @note This is only supported for MPEG-H.
 *
 * If called, the library will produce the 'mhap' box containing the set of compatible profile and
 * levels. The function can be called multiple times to add more compatible set indications to the
 * list.
 * @see isobmff_createTrackConfig
 * @param [in] trackConfig - a track config object.
 * @param [in] PLcompatibleSet - the compatible profile and level.
 * @returns ISOBMFF_OK if the compatible profile and level was successfully added to the set.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addMpeghPLcompatibleSet(TrackConfig* trackConfig,
                                                           const uint8_t PLcompatibleSet);
/**
 * @brief Destroys the track config object.
 *
 * @param [in] trackConfig - a track config object.
 * @returns ISOBMFF_OK if the track config object was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyTrackConfig(TrackConfig* trackConfig);
/**
 * @brief Creates and registers a new track and track writer
 *
 * @param [in]  isobmffWriter - an isobmff writer instance.
 * @param [out] trackWriter   - a track writer instance.
 * @param [in]  trackConfig   - a track config object.
 * @returns ISOBMFF_OK if the track writer was successfully created.
 *
 * @note Created TrackWriter object must be be destroyed with @ref isobmff_destroyTrackWriter.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createTrackWriter(ISOBMFF_Writer* isobmffWriter,
                                                     TrackWriter** trackWriter,
                                                     const TrackConfig* trackConfig);
/**
 * @brief Add a new sample to the track.
 *
 * @see isobmff_createTrackWriter must be called first.
 * @param [in] trackWriter - a track writer instance.
 * @param [in] sample      - an isobmff sample.
 * @returns ISOBMFF_OK if the sample was successfully added to the track.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addSample(TrackWriter* trackWriter, Sample* sample);
/**
 * @brief Add an edit list entry to the track.
 *
 * @see isobmff_createTrackWriter must be called first.
 * @param [in] trackWriter   - a track writer instance.
 * @param [in] editListEntry - an edit list entry.
 * @returns ISOBMFF_OK if the edit list entry was successfully added to the track.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addEditListEntry(TrackWriter* trackWriter,
                                                    const EditListEntry* editListEntry);
/**
 * @brief Add user defined data to the track.
 *
 * The data will be stored as part of the 'udta' box carried in the 'trak' container.
 * Each call to this function will result in a child box of the 'udta' container box.
 * @see isobmff_createTrackWriter
 * @param [in] trackWriter   - a track writer instance.
 * @param [in] data          - buffer containing the user data: this buffer is expected to be
 * formatted as a isobmff box (or 'atom') specified in ISO/IEC 14496-12, Clause 4.2: 'size' [4
 * bytes] - 'type' (aka 4cc) [4 bytes] - payload ['size' - 8 bytes]
 *                             \b Note: 'size' is a big-endian unsigned integer.
 * @param [in] size          - the size of the data buffer (same as the 'size' stored at the
 * beginning of 'data').
 * @returns ISOBMFF_OK if the user data was successfully added to the track.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_addTrackUserDataEntry(TrackWriter* trackWriter,
                                                         const uint8_t* data, const uint32_t size);
/**
 * @brief Destroys the track writer instance.
 *
 * @param [in] trackWriter - a track writer instance.
 * @returns ISOBMFF_OK if the track writer instance was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroyTrackWriter(TrackWriter* trackWriter);

/**@}*/

/************* MMTISOBMFF SAMPLE INTERFACE ********************/

/** @name MMTISOBMFF SAMPLE INTERFACE */
/**@{*/

/**
 * @brief Creates a sample (data structure that contains media samples and their meta data)
 *
 * This sample structure is meant to represent a so called isobmff sample. It can contain
 * several audio samples (forming one audio frame) or one video frame.\n
 * Please refer to ISO/IEC 14496-12 regarding how a sample is defined for a specific track type.
 * @param [out] sample             - an isobmff sample.
 * @param [in]  preAllocSampleSize - pre-allocated sample size (optional, 0 : automatic).
 * @returns ISOBMFF_OK if the sample was successfully created.
 *
 * @note Created Sample object must be be destroyed with @ref isobmff_destroySample.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createSample(Sample** sample, const uint64_t preAllocSampleSize);
/**
 * @brief Destroys the sample.
 *
 * @param [in] sample - an isobmff sample.
 * @returns ISOBMFF_OK if the sample was successfully destroyed.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_destroySample(Sample* sample);
/**
 * @brief Resets the sample (media and meta data).
 *
 * @param [in] sample - an isobmff sample.
 * @returns ISOBMFF_OK if the sample was successfully reset.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_resetSample(Sample* sample);
/**
 * @brief Gets the sample media data.
 *
 * @see isobmff_createSample
 * @param [in]  sample    - an isobmff sample.
 * @param [out] mediaData - the sample media data.
 * @param [out] size      - size of the sample media data.
 * @returns ISOBMFF_OK if the sample media data was obtained successfully.
 *
 * @note The memory for the mediaData pointer returned is managed by the library and shall not be
 * freed manually.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleData(Sample* sample, uint8_t** mediaData,
                                                 uint64_t* size);
/**
 * @brief Gets the sample duration.
 *
 * @note The duration is counted in ticks of the track timescale.
 *
 * @see isobmff_createSample
 * @param [in]  sample   - an isobmff sample.
 * @param [out] duration - the sample duration.
 * @returns ISOBMFF_OK if the sample duration was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleDuration(Sample* sample, uint64_t* duration);
/**
 * @brief Gets the sample composition time stamp (CTS) offset.
 *
 * The CTS offset is the difference between the presentation and decoding timestamp.
 *
 * @note The CTS offset is counted in ticks of the track timescale.
 * @note CTS offset are typically only used for video.
 *
 * @see isobmff_createSample
 * @param [in]  sample    - an isobmff sample.
 * @param [out] ctsOffset - the sample CTS offset.
 * @returns ISOBMFF_OK if the sample CTS offset was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleCtsOffset(Sample* sample, int64_t* ctsOffset);
/**
 * @brief Gets the sample fragment number.
 *
 * This function can be used to check to which fragment an isobmff sample belongs.
 * Only useful for fragmented MP4 files. In case of a plain MP4 file it will always
 * report the same value.
 * @see isobmff_createSample
 * @param [in]  sample         - an isobmff sample.
 * @param [out] fragmentNumber - the sample fragment number.
 * @returns ISOBMFF_OK if the sample fragment number was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleFragmentNum(Sample* sample, uint32_t* fragmentNumber);
/**
 * @brief Gets the sync sample flag.
 *
 * @see isobmff_createSample
 * @param [in]  sample       - an isobmff sample.
 * @param [out] isSyncSample - the flag to determine if a sample is a sync sample.
 * The only two allowed values are 0 and 1.
 * @returns ISOBMFF_OK if the synch sample flag was obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleSyncFlag(Sample* sample, uint8_t* isSyncSample);
/**
 * @brief Gets the sample group and value.
 *
 * @see isobmff_createSample
 * @param [in]  sample          - an isobmff sample.
 * @param [out] sampleGroupType - the sample group type.
 * @param [out] value    - for type roll/prol the value parameter is the roll-distance value (as
 * signed type), for sap it is the sap-type (as 8bit unsigned type)
 * @returns ISOBMFF_OK if  sample group and value were obtained successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_getSampleGroup(Sample* sample, SampleGroup_C* sampleGroupType,
                                                  int16_t* value);
/**
 * @brief Sets the sample media data.
 *
 * @see isobmff_createSample
 * @param [in] sample    - an isobmff sample.
 * @param [in] mediaData - the sample media data.
 * @param [in] size      - size of the sample media data.
 * @returns ISOBMFF_OK if the sample media data was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleData(Sample* sample, const uint8_t* mediaData,
                                                 const uint64_t size);
/**
 * @brief Sets the sample duration.
 *
 * @note The duration is counted in ticks of the track timescale.
 *
 * @see isobmff_createSample
 * @param [in] sample   - an isobmff sample.
 * @param [in] duration - the sample duration.
 * @returns ISOBMFF_OK if the sample duration was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleDuration(Sample* sample, const uint64_t duration);
/**
 * @brief Sets the sample composition time stamp (CTS) offset.
 *
 * The CTS offset is the difference between the presentation and decoding timestamp.
 *
 * @note The CTS offset is counted in ticks of the track timescale.
 * @note CTS offset are typically only used for video.
 *
 * @see isobmff_createSample
 * @param [in] sample    - an isobmff sample.
 * @param [in] ctsOffset - the sample CTS offset.
 * @returns ISOBMFF_OK if the sample CTS offset was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleCtsOffset(Sample* sample, const int64_t ctsOffset);
/**
 * @brief Sets the sample fragment number.
 *
 * @see isobmff_createSample
 * @param [in] sample         - an isobmff sample.
 * @param [in] fragmentNumber - the sample fragment number.
 * @returns ISOBMFF_OK if the sample fragment number was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleFragmentNum(Sample* sample,
                                                        const uint32_t fragmentNumber);
/**
 * @brief Sets the sync sample flag.
 *
 * @see isobmff_createSample
 * @param [in] sample       - an isobmff sample.
 * @param [in] isSyncSample - the flag to determine if a sample is a sync sample.
 * The only two allowed values are 0 and 1.
 * @returns ISOBMFF_OK if the synch sample flag was set successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleSyncFlag(Sample* sample, const uint8_t isSyncSample);
/**
 * @brief Sets the sample group and value.
 *
 * @see isobmff_createSample
 * @param [in] sample          - an isobmff sample.
 * @param [in] sampleGroupType - the sample group type.
 * @param [in] value    - for type roll/prol the value parameter is the roll-distance value (as
 * signed type), for sap it is the sap-type (as 8bit unsigned type)
 * @returns ISOBMFF_OK if the movie config object was successfully created.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setSampleGroup(Sample* sample,
                                                  const SampleGroup_C sampleGroupType,
                                                  const int16_t value);

/**@}*/

/************* MMTISOBMFF COPY INTERFACE ********************/

/** @name MMTISOBMFF COPY INTERFACE */
/**@{*/

/**
 * @brief Creates a movieConfig that can be used to create a writer instance by using an existing
 * reader instance to get the data from.
 *
 * @note The movieConfig can be further tweaked by calling the standard API setter methods on it,
 * before creating the write instance.
 * @note If there is existing global movie user data ('udta' fields on 'moov' level), it will only
 * be copied when copyMoovUdta flag is set to true. Otherwise it will be ignored. To manipulate user
 * data, please use the existing getter and setter in combination with this function.
 * @note The movieConfig must be deleted with isobmff_destroyMovieConfig again, after the writer was
 * created.
 *
 * @see isobmff_createFileReader
 * @see isobmff_createMemoryReader
 * @see isobmff_createFileWriter
 * @see isobmff_createFragFileWriter
 * @see isobmff_createFragFileSegWriter
 * @see isobmff_createFragMemoryWriter
 * @see isobmff_destroyMovieConfig
 * @param [in] isobmffReader - an isobmff reader instance.
 * @param [in] copyMovieUdta - a boolean flag to control whether to copy existing global movie (moov
 * udata) user data from the reader instance or to discard them. The only two allowed values are 0
 * and 1.
 * @param [out] movieConfig  - a movie config object.
 * @returns ISOBMFF_OK if movie config object was created successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createMovieConfigFromReader(ISOBMFF_Reader* isobmffReader,
                                                               const uint8_t copyMovieUdta,
                                                               MovieConfig** movieConfig);
/**
 * @brief Creates a TrackConfig that can be used to create a track writer instance by using an
 * existing track reader instance to get the data from.
 *
 * @note The trackConfig can be further tweaked by calling the standard API setter methods on it,
 * before creating the write instance.
 * @note It will **not** copy track based user data ('udta' on 'trak' level). Track based user data
 * can be added, re-added or manipulated by using the existing getters and setters on trackReader
 * and trackWriter. Alternatively (or in combination) the @ref isobmff_copyUdataFromTrack function
 * can be used.
 * @note It will **not** copy track based edit lists ('elst' on 'trak' level). Track based edit
 * lists can be added, re-added or manipulated by using the existing getters and setters on
 * trackReader and trackWriter. Alternatively (or in combination) the @ref
 * isobmff_copyEditListsFromTrack function can be used.
 * @note It will **not** copy track based isobmff container based 'ludt' loudness boxes ('tlou',
 * 'alou', etc.) Please use existing getters and setters for this.
 * @note The trackConfig must be deleted with @ref isobmff_destroyTrackConfig again, after the
 * writer was created.
 *
 * @see isobmff_getTrack
 * @see isobmff_createTrackWriter
 * @see isobmff_destroyTrackConfig
 * @param [in] trackReader - a track reader object.
 * @param [out] trackConfig - a track config object.
 * @returns ISOBMFF_OK if track config object was created successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_createTrackConfigFromReader(TrackReader* trackReader,
                                                               TrackConfig** trackConfig);
/**
 * @brief Copies existing track based user data ('udta') from an existing track reader to a track
 * writer. Can be used to replicate tracks in combination with @ref
 * isobmff_createTrackConfigFromReader.
 *
 * @note This is just a convenience function and does the same as using the C interface functions to
 * get and set all user data information from a track reader to a track writer.
 * @note You can still add additional user data entries afterwards and existing ones you set
 * beforehand on the track writer are **not** overwritten.
 *
 * @see isobmff_getTrack
 * @see isobmff_createTrackWriter
 * @see isobmff_getTrackUserDataEntryCount
 * @see isobmff_getTrackUserDataEntryByIndex
 * @see isobmff_addTrackUserDataEntry
 * @param [in] trackReader - a track reader object.
 * @param [in] trackWriter - a track writer object.
 * @returns ISOBMFF_OK if track based user data was copied over successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_copyUdataFromTrack(TrackReader* trackReader,
                                                      TrackWriter* trackWriter);
/**
 * @brief Copies existing track based edit lists from an existing track reader to a track writer.
 * Can be used to replicate tracks in combination with @ref isobmff_createTrackConfigFromReader.
 *
 * @note This is just a convenience function and does the same as using the C interface functions to
 * get and set all edit list information from a track reader to a track writer.
 * @note You can still add additional edit list entries afterwards and existing ones you set
 * beforehand on the track writer are **not** overwritten.
 *
 * @see isobmff_getTrack
 * @see isobmff_createTrackWriter
 * @see isobmff_getEditListEntryCount
 * @see isobmff_createEditListEntry
 * @see isobmff_addEditListEntry
 * @param [in] trackReader - a track reader object.
 * @param [in] trackWriter - a track writer object.
 * @returns ISOBMFF_OK if track based edit lists were copied over successfully.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_copyEditListsFromTrack(TrackReader* trackReader,
                                                          TrackWriter* trackWriter);
/**
 * @brief Copies all samples from a track reader to a track writer. Can be used to replicate tracks
 * in combination with @ref isobmff_createTrackConfigFromReader.
 *
 * @note This is just a convenience function and does the same as using the C interface functions to
 * get and set all samples from a track reader to a track writer.
 * @note This is only supported for a non-fragmented, plain (flat) MP4 file writer, since
 * fragmentation needs additional information. If a fragmented writer is used, an error will be
 * returned.
 *
 * @see isobmff_getTrack
 * @see isobmff_createTrackWriter
 * @param [in] trackReader - a track reader object.
 * @param [in] trackWriter - a track writer object.
 * @returns ISOBMFF_OK if all samples were copied successfully from the reader to the writer.
 */
MMTISOBMFF_DLL ISOBMFF_ERR isobmff_copySamplesFromTrack(TrackReader* trackReader,
                                                        TrackWriter* trackWriter);

/**@}*/

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* MMTISOBMFF_C_H */
