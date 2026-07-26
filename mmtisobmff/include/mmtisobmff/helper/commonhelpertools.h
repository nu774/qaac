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
 * Content: C++ interface, common helper tools
 */

/*!
 * @file commonhelpertools.h
 * @brief Collection of common, unspecific helper tools
 *
 * Common helper tools used for various use-cases (e.g. simplified track copying, writing, mp4 type
 * detection, etc.)
 */

#pragma once

// System includes
#include <memory>
#include <deque>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"
#include "mmtisobmff/reader/reader.h"
#include "mmtisobmff/reader/trackreader.h"
#include "mmtisobmff/writer/writer.h"

namespace mmt {
namespace isobmff {
namespace tools {
//! Extracts and copies all samples from the specified track to a sample queue
std::deque<std::unique_ptr<CSample>> getAllSamples(
    std::unique_ptr<CGenericTrackReader>& trackReader);

/*! \addtogroup CommonHelperTools Collection of generic helper tools for all sorts of use-cases
 *  @{
 */

//! Returns the current system time in UTC as seconds
uint64_t currentUTCTime();
//! Converts an UTC time value into a humand readly string
std::string UTCTimeToString(uint64_t time);

/*!
 * @brief Converts sample flags value to custom struct format
 *
 * For details see ISO/IEC 14496-12 - 8.8.3.1
 *
 * @param value Flags value according to ISO/IEC 14496-12 - 8.8.3.1 to be parsed
 * @return Parsed flags value
 *
 * @note Only applicable for sample related flags
 */
SSampleFlags valueToSampleFlags(uint32_t value);
/*!
 * @brief Converts custom sample flags struct format to standardised flags value
 *
 * For details about the format see ISO/IEC 14496-12 - 8.8.3.1
 *
 * @param sampleFlags Custom sample flags struct to be converted
 * @return Flags value according to ISO/IEC 14496-12 - 8.8.3.1
 *
 * @note Only applicable for sample related flags
 */
uint32_t sampleFlagsToValue(const SSampleFlags& sampleFlags);

/**@}*/

/*! \addtogroup TrackCopyHelperTools Collection of helper tools for track copying
 *  @{
 */

/*!
 * @brief Configuration of how to perform track copying
 *
 * This config allows detailed configuration on how samples and/or tracks
 * should be copied.
 *
 * It allows manipulation while making the copy to e.g. convert a fragmented
 * to a plain/flat MP4 file or vice versa.
 *
 * It is also possible to multiplex or demultiplex tracks via this config.
 */
struct SCopyConfig {
  /*!
   * @brief Configure if file should be defragmented during copy
   *
   * Affects @ref CSample::fragmentNumber. Useful to create a
   * defragmentation tool that takes in fragmented MP4 files and
   * converts them into flat/plain ones.
   *
   * - True : Sample fragment number is not altered. That means a plain
   * file stays plain and a fragmented file stays fragmented.
   * - False : Sample fragment number is cleared. This is a no-op for plain
   * file. A fragmented one will be defragmented during the copy process.
   *
   * @note Must be False if @ref SCopyConfig::fragmentDuration > 0.
   */
  bool keepFragNumber = false;

  /*!
   * @brief Configure warning/error behaviour during fragmentation
   *
   * Only used if fragmentDuration is set to a value > 0 to enable manual
   * fragmentation.
   *
   * - True : No exception is thrown if fragments do not start with a sync sample.
   * A warning is logged for every fragment that does not start with a sync sample.
   * - False : An exception is thrown if a fragment does not start with a sync sample.
   *
   * @warning It is advised to leave this option to False, otherwise the created
   * MP4 file might be invalid. Only use for special or debugging reasons.
   */
  bool ignoreSyncSample = false;

  /*!
   * @brief Enable automatic fragmentation at sync samples
   *
   * Affects @ref CSample::fragmentNumber. Useful to create a
   * fragmentation tool that takes in plain/flat MP4 files and
   * converts them into fragmented ones.
   *
   * - True : Automatically fragment at every sync sample found
   * - False : It will leave the input MP4 format as is
   *
   * @note Must be False if @ref SCopyConfig::fragmentDuration > 0.
   */
  bool fragmentEverySyncSample = false;

  /*!
   * @brief Enable manual fragmentation at intervals
   *
   * Affects @ref CSample::fragmentNumber. Useful to create an
   * advanced fragmentation tool that takes in plain/flat mp4 files and
   * converts them into fragmented ones.
   *
   * This allows to manually set a fragmentation interval to configure the
   * fragment length.
   *
   * The fragment duration must be given in ticks of the track timescale.
   *
   * @note This should only be used if the file has periodic and equidistant sync samples.
   *
   * @note Usually, fragmentation is only safe at sync sample, the duration
   * must therefore be a multiple of the sync sample distance.
   *
   * @note If this feature is used, @ref SCopyConfig::fragmentEverySyncSample
   * and @ref SCopyConfig::keepFragNumber must be False.
   *
   * @see @ref SCopyConfig::ignoreSyncSample
   */
  uint32_t fragmentDuration = 0;

  /*!
   * @brief Track info of the track that should be copied
   *
   * This info will be used to determine which track to copy
   * and is also used to create a new track from it.
   *
   * To get this info object, use the @ref CIsobmffReader::trackInfos function
   * and copy the track info object of the track that should be copied from the vector
   * into this struct.
   *
   * @note It is advise to not handcraft this info object since it must match the internal
   * data structure of the track reader being used to read the selected track.
   */
  CTrackInfo trackInfo;

  /*!
   * @brief Movie timescale of the source MP4 file
   *
   * This is the movie timescale of the source MP4 file (timescale in 'moov').
   * It is important to set it to the value that is reported via @ref CIsobmffReader::movieInfo.
   *
   * This value is used to re-compute various fields in case source and destination timescale
   * differ.
   */
  uint64_t oldMovieTimescale = 0;

  /*!
   * @brief Movie timescale of the target MP4 file
   *
   * This is the movie timescale of the target MP4 file (timescale in 'moov').
   * It is important to set it to the same value that is used to create the target
   * @ref CIsobmffWriter
   *
   * This value is used to re-compute various fields in case source and destination timescale
   * differ.
   *
   * This is important for example during multiplexing. If two separate MP4 files with one track
   * each are copied into a multiplexed file, the target file should use a movie timescale that can
   * properly represent both movie timescales of the source files to avoid rounding issues.
   * If the timescales differ, fields in e.g. the EditList need to be re-computed in the new
   * target timescale.
   */
  uint64_t newMovieTimescale = 0;
};

/*!
 * @brief Copies samples according to the copy config
 *
 * Low level function to copy only samples. Requires already set up track
 * reader and writer.
 *
 * @param tReader Source track reader to read samples from
 * @param tWriter Target track writer to write samples to
 * @param config Copy config on how to perform the copying
 *
 * @note Source reader and target writer must be of the same track/codec type
 */
template <typename Tsample, class Treader, class Twriter>
void copyAUs(const Treader& tReader, const Twriter& tWriter, const SCopyConfig& config);

/*!
 * @brief Copies basic track data and all the samples according to the copy config
 *
 * Function to copy a complete track including all samples and track related metadata like edit
 * list, user data, etc.
 *
 * The track to copy from the input reader is configured via @ref SCopyConfig::trackInfo and must be
 * taken from the source reader via @ref CIsobmffReader::trackInfos.
 *
 * The writer must be created s.t. it matches the fragmentation settings in config. If the use-case
 * is to defragment or multiplex, a plain file or memory writer like @ref CIsobmffFileWriter must be
 * used. If the use-case is to fragment a fragment writer like @ref CIsobmffFragFileWriter must be
 * used.
 *
 * @param reader Source MP4 reader to read tracks and samples from
 * @param writer Target MP4 writer to write tracks and samples to
 * @param config Copy config specifying how to perform the copying
 *
 * @note Make sure the target writer matches the fragmentation settings in config. For example: A
 * plain/flat file writer cannot write be used with @ref SCopyConfig::fragmentEverySyncSample =
 * True.
 */
void copyTrack(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config);

/**@}*/

/*! \addtogroup TrackWriteHelperTools Collection of helper tools to abstract/ease track creation
 *  @{
 */

/*!
 * @brief Config to automate/ease track creation
 *
 * This config can be used in combination with @ref createTrackWriter to automatically
 * create a track writer based on the values provided in the config.
 *
 * It is a convenience wrapper for the @ref CTrackWriter interface, but less powerful.
 */
struct SEasyTrackConfig {
  /*!
   * @brief Codec type as fourCC to configure which track writer to use (required)
   *
   * For supported codecs see @ref CTrackWriter
   */
  ilo::Fourcc codecType = ilo::toFcc("0000");
  /*!
   * @brief ID being inserted into 'tkhd' or 'tfhd' track_ID field (optional)
   *
   * If left at 0, the value will be auto-computed
   */
  uint32_t trackId = 0;
  /*!
   * @brief Track sample rate in Hz (required for audio)
   *
   * Only valid for audio codecs
   */
  uint32_t sampleRate = 0;
  /*!
   * @brief Track channel count (required for AAC audio)
   *
   * Only valid for AAC-based codecs
   */
  uint16_t channelCount = 0;
  /*!
   * @brief Track language (optional)
   *
   * Only valid for audio codecs. Leave at "und" if it should not be set
   */
  ilo::IsoLang language = ilo::toIsoLang("und");
  /*!
   * @brief Timescale for the media contained in this track (required)
   *
   * For audio: It is recommended to set this to the value of the audio sample rate.
   * For video: It is recommended to set this to the value of the frame rate (the denominator part
   * of it)
   */
  uint32_t timescale = 0;
  /*!
   * @brief Track width in pixel (required for video)
   *
   * Only valid for video codecs
   */
  uint16_t width = 0;
  /*!
   * @brief Track height in pixel (required for video)
   *
   * Only valid for video codecs
   */
  uint16_t height = 0;
  /*!
   * @brief MPEG-H compatible profile and levels (optional)
   *
   * Only valid for MPEG-H Audio codec
   * If set, the library will generate an 'mhap' box containing the set of compatible profiles and
   * levels.
   */
  std::vector<uint8_t> compatibleProfileLevels;
  /*!
   * @brief Serialized byte stream of decoder config record (required for most codecs)
   *
   * See codec specific version of @ref STrackConfig on whether it is required.
   * Use codec specific config record class to fill values and create the
   * serialized byte stream.
   */
  ilo::ByteBuffer decoderConfigRecord;
  /*!
   * @brief Configures a default sample group table of type 'sgpd' in 'trak' (optional)
   *
   * If samples are added to this sample group, the 'sgpd' table is only
   * written once for this type and not repeated in fragments.
   *
   * @note This is only useful for fragmented MP4 files and reduces the MP4 overhead.
   */
  SSampleGroupInfo defaultSampleGroup;
};

/*!
 * @brief Convenience function to ease track creation via a common config.
 *
 * Creates a track writer for the given writer based on the given config.
 *
 * Does not allow the same amount of flexibility as the full @ref CTrackWriter interface.
 *
 * @param writer MP4 writer instance to create the track writer for.
 * @param config Config defining what track writer to create and with what values.
 * @return Track writer instance.
 *
 * @note Beware that the returned track writer pointer is of type @ref ITrackWriter.
 * It allows access to the generic write functions, but not to codec specific writer functions
 * (if any). If required, the pointer can be cast to a specific writer type to gain access to
 * codec specific functions.
 */
std::unique_ptr<ITrackWriter> createTrackWriter(CIsobmffWriter& writer,
                                                const SEasyTrackConfig& config);

/**@}*/

/*! \addtogroup Mp4DetectionHelperTools Collection of helper tools to detect MP4 file types
 *  @{
 */

/*!
 * @brief MP4 format types
 *
 **/
enum class EMp4Type {
  unknown,       /**< Type cannot be deduced. It is either no MP4 file or an unknown type */
  initSegment,   /**< Contains only 'moov', but no 'moof' and no 'mdat' */
  mediaSegment,  /**< Contains only 'moof' and 'mdat', but no 'moov' */
  fragmentedMp4, /**< Contains 'moov', 'moof' and 'mdat' */
  flatMp4        /**< Contains 'moov' and 'mdat', but no 'moof' */
};

/*!
 * @brief Get the potential MP4 format from byte buffer (e.g. init segment, media fragment, etc ...)
 *
 * Can be used to detect what type of MP4 format the buffer probably contains. For example, calling
 * this function on a buffer containing only 'moov' will result in @ref EMp4Type::initSegment
 *
 * @param inputBuffer Buffer to probe.
 * @return MP4 format found.
 */
EMp4Type getMp4TypeFromBuffer(const ilo::ByteBuffer& inputBuffer);

/**@}*/
}  // namespace tools
}  // namespace isobmff
}  // namespace mmt
