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

/*!
 * @file trackreader.h
 * @brief Interface for reading MP4 track metadata and samples
 * \defgroup mp4trackreader Main interface for reading MP4 track metadata and samples
 *
 * Main interface for reading MP4 track metadata and samples
 */

#pragma once

// System includes
#include <memory>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"
#include "mmtisobmff/reader/reader.h"
#include "mmtisobmff/configdescriptor/mha_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/avc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/hevc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/jxs_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/vvc_decoderconfigrecord.h"

namespace mmt {
namespace isobmff {
namespace config {
class CMhaDecoderConfigRecord;
class CAvcDecoderConfigRecord;
class CHevcDecoderConfigRecord;
class CMp4aDecoderConfigRecord;
class CJxsDecoderConfigRecord;
class CVvcDecoderConfigRecord;
}  // namespace config

/*!
 * @brief Interface for codec specific track readers
 *
 * A track reader allows reading track specific metadata from an mp4 file
 * (like sample related information). The supported metadata is defined
 * per specific track reader type.
 */
struct ITrackReader {
  virtual ~ITrackReader() {}
};

/*!
 * @brief Generic track reader for arbitrary track type
 *
 * This reader is not tied to a specific codec and can always be used. It will not
 * be able to retrieve codec specific metadata, but samples can be read as @ref CSample.
 * The format of the @ref CSample payload differs depending on the underlying codec.
 *
 * \ingroup mp4trackreader
 */
class CGenericTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates a generic track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via @ref CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CGenericTrackReader>(tracknumber) @endcode
   */
  CGenericTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CGenericTrackReader() override;

  /*!
   * @brief Returns the decoder configuration record (VVC, HEVC, AVC, JXS, MPEG-H)
   * or ES descriptor (AAC) as stored one level below the 'stsd' box
   *
   * The function is returning a serialized representation of the isobmff config record structure.
   * To gain access to specific fields it must be parsed in a codec specific way. Can be used in
   * combination with the decoderconfigrecord classes that will parse the information. The codec
   * being used can be retrieved vom the CTrackInfo structure.
   *
   * @return Byte buffer containing the codec specific decoder config record in a serialized form
   */
  virtual ilo::ByteBuffer decoderConfigRecord() const;
  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  virtual SSampleExtraInfo nextSample(CSample& sample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1
   */
  virtual SSampleExtraInfo sampleByIndex(size_t sampleIndex, CSample& sample,
                                         bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  virtual SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, CSample& sample,
                                             bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  virtual SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;
  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  virtual ilo::Fourcc codingName() const;

 protected:
  struct Pimpl;
  std::unique_ptr<Pimpl> p;
};

/*!
 * @brief Generic audio track reader for arbitrary audio track type
 *
 * This reader can be used with any audio track to get some generic
 * audio related information. It will not be able to retrieve any codec specific
 * metadata. The format of the @ref CSample payload follows the structure defined
 * for the particular codec and is not interpreted.
 *
 * @note Not all generic audio values that can be read with this instance need
 * to be defined for every audio codec. They are read as they are stored in the
 * file format.
 *
 * \ingroup mp4trackreader
 */
class CGenericAudioTrackReader : public CGenericTrackReader {
 public:
  /*!
   * @brief Creates a generic audio track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CGenericAudioTrackReader>(tracknumber) @endcode
   */
  CGenericAudioTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CGenericAudioTrackReader() override;

  /*!
   * @brief Number of channels as stored in the sample entry
   *
   * @return Number of audio channels for this track
   *
   * @note This is an isobmff template field and not applicable for all codecs. It may contain
   * template default values (like 2) or 0. Read codec specific specification to see whether this
   * field contains valid data.
   */
  uint16_t channelCount() const;
  /*!
   * @brief Sample size as stored in the sample entry
   *
   * @return Sample size in bits for legacy codecs
   *
   * @note This is an isobmff template field and not applicable for all codecs. It may contain
   * template default values (like 16). Read codec specific specification to see whether this field
   * contains valid data.
   * @warning This is not the real size of an isobmff sample in bytes.
   */
  uint16_t sampleSize() const;
  /*!
   * @brief Sample rate as stored in the sample entry
   *
   * @return Sample rate in Hz
   *
   * @note This only returns the first 16 bit of the 32bit sampleRate field containing
   * the sample rate in Hz for AudioSampleEntry (of type V0). AudioSampleEntryV1
   * defines this template a bit differently, but is currently not supported by this library.
   * @note This value might deviate from the final sample rate returned by a decoder
   * depending on its configuration and potential re-sampler. Some codecs also use
   * special backwards compatible or implicit signalling that can affect this value. It is
   * advised to only use this value for potential initial audio output sink configuration and
   * later reconfigure the output based on the actual values provided by the decoder.
   */
  uint32_t sampleRate() const;

 private:
  struct PimplAudio;
  std::unique_ptr<PimplAudio> pa;
};

/*!
 * @brief Generic video track reader for arbitrary video track type
 *
 * This reader can be used with any video track to get some generic
 * video related information. It will not be able to retrieve any codec specific
 * metadata. The format of the @ref CSample payload follows the structure defined
 * for the particular codec and is not interpreted.
 *
 * @note Not all generic video values that can be read with this instance need
 * to be defined for every video codec. They are read as they are stored in the
 * file format.
 *
 * \ingroup mp4trackreader
 */
class CGenericVideoTrackReader : public CGenericTrackReader {
 public:
  /*!
   * @brief Creates a generic video track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CGenericVideoTrackReader>(tracknumber) @endcode
   */
  CGenericVideoTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CGenericVideoTrackReader() override;

  /*!
   * @brief Width in pixels of the video samples stored in this track.
   *
   * @note The exact interpretation of this value might be codec specific.
   * If not otherwise specified, it is the maximum visual width of the stream
   * described by this sample description, in pixels.
   */
  uint16_t width() const;
  /*!
   * @brief Height in pixels of the video samples stored in this track.
   *
   * @note The exact interpretation of this value might be codec specific.
   * If not otherwise specified, it is the maximum visual height of the stream
   * described by this sample description, in pixels.
   */
  uint16_t height() const;
  /*!
   * @brief Horizontal video resolution in DPI as stored in the sample entry
   *
   * @note This is an isobmff template field and not applicable for all codecs. The default
   * template value is 72 dpi. Read codec specific specification to see whether this field
   * contains valid data.
   */
  double horizontalResolutionDPI() const;
  /*!
   * @brief Vertical video resolution in DPI as stored in the sample entry
   *
   * @note This is a template field and not applicable for all codecs. The default
   * template value is 72 dpi. Read codec specific specification to see if this field
   * contains valid data.
   */
  double verticalResolutionDPI() const;
  /*!
   * @brief Number of compressed video frames per isobmff sample as stored in the sample entry
   *
   * @note This is an isobmff template field and not applicable for all codecs. The default
   * template value is 1. Read codec specific specification to see whether this field
   * contains valid data.
   */
  uint16_t frameCount() const;
  //! Compressor name as stored in the sample entry
  std::string compressorName() const;
  /*!
   * @brief Depth as stored in the sample entry (special format, not in bits, depends on video
   * codec)
   *
   * @note This is an isobmff template field and not applicable for all codecs. The default
   * template value is 0x0018 (images color with no alpha). Read codec specific specification to see
   * whether this field contains valid data.
   */
  uint16_t depth() const;

 private:
  struct PimplVideo;
  std::unique_ptr<PimplVideo> pv;
};

/*!
 * @brief MPEG-H 3D Audio specific track reader
 *
 * This reader can be used to read MPEG-H Audio tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload follows the structure defined in ISO/IEC 23008-3 chapter
 * 20 (Carriage of MPEG-H 3D audio in ISO base media file format).
 *
 * @note One @ref CSample contains exactly one MPEG-H access unit (AU). For raw (mha) samples
 * without encapsulation this is a 1:1 mapping between an MPEG-H AU and a @ref CSample. For MHAS
 * (mhm) encapsulation all MHAS packets belonging to one audio AU must be packed into one @ref
 * CSample.
 *
 * \ingroup mp4trackreader
 */
class CMpeghTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates an MPEG-H Audio track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CMpeghTrackReader>(tracknumber); @endcode
   */
  CMpeghTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CMpeghTrackReader() override;

  /*!
   * @brief Access the MPEG-H Audio decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including the MPEGH3DAConfig
   * required to initialize a decoder for 'mha' tracks.
   *
   * @note This data structure is only guaranteed to be available for 'mha' based tracks. It is
   * optional for 'mhm'. If there is no data available, the function returns a nullptr.
   */
  std::unique_ptr<config::CMhaDecoderConfigRecord> mhaDecoderConfigRecord() const;
  //! Get the list of compatible MPEG-H Audio Profiles and Levels
  std::vector<uint8_t> profileAndLevelCompatibleSets() const;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(CSample& sample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read
   * @param [out] sample Sample data containing one access unit (AU). If empty, there is no sample
   * for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, CSample& sample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to a given point in time while fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, CSample& sample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  /*!
   * @brief Sample rate as stored in the sample entry
   *
   * @return Sample rate in Hz
   *
   * @note This only returns the first 16 bit of the 32bit sampleRate field containing
   * the sample rate in Hz for AudioSampleEntry (of type V0). AudioSampleEntryV1
   * defines this template a bit differently, but is currently not supported by this library.
   * @note This value might deviate from the final sample rate returned by a decoder
   * depending on its configuration and potential re-sampler. It is advised to only use this
   * value for potential initial audio output sink configuration and later reconfigure the
   * output based on the actual values provided by the decoder.
   */
  uint32_t sampleRate() const;

 private:
  struct PimplMpegh;
  std::unique_ptr<PimplMpegh> pmpegh;
};

/*!
 * @brief AAC specific track reader (works for all AOTs)
 *
 * This reader can be used to read AAC audio tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload is defined as one raw (no encapsulation) AAC audio access
 * unit (AU) per @ref CSample.
 *
 * @note One @ref CSample only contains one AAC AU. ADTS, LATM and LATM/LOAS
 * encapsulation layers are not allowed.
 *
 * \ingroup mp4trackreader
 */
class CMp4aTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates an AAC audio track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CMp4aTrackReader>(tracknumber); @endcode
   */
  CMp4aTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CMp4aTrackReader() override;

  /*!
   * @brief Access the AAC decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including the Audio Specific
   * Config (ASC) required to initialize a decoder.
   *
   * @note This data structure is not optional for AAC and therefore, for valid MP4 input, this
   * function always returns a valid pointer. If it returns a nullptr the AAC track of the MP4 file
   * is invalid/malformed.
   */
  std::unique_ptr<config::CMp4aDecoderConfigRecord> mp4aDecoderConfigRecord() const;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(CSample& sample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read.
   * @param [out] sample Sample data containing one access unit (AU). If empty, there is no sample
   * for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, CSample& sample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] sample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, CSample& sample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  /*!
   * @brief Sample rate as stored in the sample entry
   *
   * @return Sample rate in Hz
   *
   * @note This only returns the first 16 bit of the 32bit sampleRate field containing
   * the sample rate in Hz for AudioSampleEntry (of type V0). AudioSampleEntryV1
   * defines this template a bit differently, but is currently not supported by this library.
   * @note This value might deviate from the final sample rate returned by a decoder
   * depending on its configuration and potential re-sampler. Some AAC codecs can use
   * special backwards compatible or implicit signalling modes that can affect this value. It is
   * advised to only use this value for potential initial audio output sink configuration and
   * the later reconfigure the output based on the actual values provided by the decoder.
   */
  uint32_t sampleRate() const;
  /*!
   * @brief Number of channels as stored in the sample entry
   *
   * @return Number of audio channels for this track
   *
   * @note This is a template field with a default value of 2. It is technically not defined
   * for use in AAC according to isobmff spec, but typically a lot of implementations are
   * writing meaningful values here. It is advised to not rely on this unless for initial setup
   * of the audio output sink and later reconfigure it by using the channel count given by
   * a decoder API.
   */
  uint16_t channelCount() const;

 private:
  struct PimplMp4a;
  std::unique_ptr<PimplMp4a> pmp4a;
};

/*!
 * @brief Advanced Video Coding (AVC/H.264) specific track reader
 *
 * This reader can be used to read AVC video tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload is defined as one raw (no encapsulation) AVC video access
 * unit (AU) per @ref CSample containing several NALUs, each one prefixed with its own size. For
 * details refer to ISO/IEC 14496-15.
 *
 * \ingroup mp4trackreader
 */
class CAvcTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates an AVC video track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CAvcTrackReader>(tracknumber); @endcode
   */
  CAvcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CAvcTrackReader() override;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * The @ref SAvcSample structure is a wrapper around a @ref CSample, allowing access to each
   * separate NALU. The begin iterator points directly to the NALU data and skips the prefixed size
   * field.
   *
   * @param [out] avcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(SAvcSample& avcSample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * The @ref SAvcSample structure is a wrapper around a @ref CSample, allowing access to each
   * separate NALU. The begin iterator points directly to the NALU data and already skips the
   * prefixed size field.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read.
   * @param [out] avcSample Sample data containing one access unit (AU). If empty, there is no
   * sample for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, SAvcSample& avcSample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * The @ref SAvcSample structure is a wrapper around a @ref CSample, allow access to each separate
   * NALU. The begin iterator points directly to the NALU data and already skips the prefixed size
   * field.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] avcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, SAvcSample& avcSample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  //! The cropped video frame width dimension (visual representation width) in pixel
  uint16_t width() const;
  //! The cropped video frame height dimension (visual representation height) in pixel
  uint16_t height() const;
  //! Compressor name as stored in the sample entry
  std::string compressorName() const;
  /*!
   * @brief Depth as stored in the sample entry (special format, not in bits)
   *
   * @note Allowed values are specified in ISO/IEC 14496-15 chapter 4.5 (Template fields used)
   * - 0x18 : the video sequence is in colour with no alpha
   * - 0x28 : the video sequence is in grayscale with no alpha
   * - 0x20 : the video sequence has alpha (gray or colour)
   */
  uint16_t depth() const;

  /*!
   * @brief Access the AVC decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including any non-VCL NALUs
   * required to initialize a decoder.
   *
   * @note This data structure is not optional for AVC and therefore, for valid MP4 input, this
   * function always returns a valid pointer. If the it returns a nullptr the AVC track of the MP4
   * file is invalid/malformed.
   */
  std::unique_ptr<config::CAvcDecoderConfigRecord> avcDecoderConfigRecord() const;

 private:
  struct PimplAvc;
  std::unique_ptr<PimplAvc> pavc;
};

/*!
 * @brief High Efficiency Video Coding (HEVC/H.265) specific track reader
 *
 * This reader can be used to read HEVC video tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload is defined as one raw (no encapsulation) HEVC video access
 * unit (AU) per @ref CSample containing several NALUs, each one prefixed with its own size. For
 * details refer to ISO/IEC 14496-15.
 *
 * \ingroup mp4trackreader
 */
class CHevcTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates an HEVC video track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CHevcTrackReader>(tracknumber); @endcode
   */
  CHevcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CHevcTrackReader() override;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * The @ref SHevcSample structure is a wrapper around a @ref CSample, allowing access to each
   * separate NALU. The begin iterator points directly to the NALU data and skips the prefixed size
   * field.
   *
   * @param [out] hevcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(SHevcSample& hevcSample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * The @ref SHevcSample structure is a wrapper around a @ref CSample, allowing access to each
   * separate NALU. The begin iterator points directly to the NALU data and skips the prefixed size
   * field.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read.
   * @param [out] hevcSample Sample data containing one access unit (AU). If empty, there is no
   * sample for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, SHevcSample& hevcSample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * The @ref SHevcSample structure is a wrapper around a @ref CSample, allowing access to each
   * separate NALU. The begin iterator points directly to the NALU data and skips the prefixed size
   * field.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] hevcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, SHevcSample& hevcSample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  //! The cropped video frame width dimension (visual representation width) in pixel
  uint16_t width() const;
  //! The cropped video frame height dimension (visual representation height) in pixel
  uint16_t height() const;
  //! Compressor name as stored in the sample entry
  std::string compressorName() const;
  /*!
   * @brief Depth as stored in the sample entry (special format, not in bits)
   *
   * @note Allowed values are specified in ISO/IEC 14496-15 chapter 4.5 (Template fields used)
   * - 0x18 : the video sequence is in colour with no alpha
   * - 0x28 : the video sequence is in grayscale with no alpha
   * - 0x20 : the video sequence has alpha (gray or colour)
   */
  uint16_t depth() const;

  /*!
   * @brief Access the HEVC decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including any non-VCL NALUs
   * required to initialize a decoder.
   *
   * @note This data structure is not optional for HEVC and therefore, for valid MP4 input, this
   * function always returns a valid pointer. If the it returns a nullptr the HEVC track of the MP4
   * file is invalid/malformed.
   */
  std::unique_ptr<config::CHevcDecoderConfigRecord> hevcDecoderConfigRecord() const;

 private:
  struct PimplHevc;
  std::unique_ptr<PimplHevc> phevc;
};

/*!
 * @brief JPEG XS Video Coding (JXS) specific track reader
 *
 * This reader can be used to read JXS video tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload is defined as one JXS codestream (called Picture())
 * without the Codestream_Header() as defined in Annex A-5.5 of ISO/IEC 21122-3
 *
 * \ingroup mp4trackreader
 */
class CJxsTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates a JXS video track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CJxsTrackReader>(tracknumber); @endcode
   */
  CJxsTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CJxsTrackReader() override;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * The format of the @ref CSample payload is defined as one JXS codestream (called Picture())
   * without the Codestream_Header() as defined in Annex A-5.5 of ISO/IEC 21122-3
   *
   * @param [out] jxsSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(CSample& jxsSample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * The format of the @ref CSample payload is defined as one JXS codestream (called Picture())
   * without the Codestream_Header() as defined in Annex A-5.5 of ISO/IEC 21122-3
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read.
   * @param [out] jxsSample Sample data containing one access unit (AU). If empty, there is no
   * sample for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, CSample& jxsSample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * The format of the @ref CSample payload is defined as one JXS codestream (called Picture())
   * without the Codestream_Header() as defined in Annex A-5.5 of ISO/IEC 21122-3
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] jxsSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, CSample& jxsSample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  //! The cropped video frame width dimension (visual representation width) in pixel
  uint16_t width() const;
  //! The cropped video frame height dimension (visual representation height) in pixel
  uint16_t height() const;
  //! Compressor name as stored in the sample entry
  std::string compressorName() const;
  /*!
   * @brief Depth as stored in the sample entry (special format, not in bits)
   *
   * @note Allowed values are specified in ISO/IEC 21122-3 chapter B.3.4 (Semantics)
   * - 0x18 : images are in colour with no alpha
   * - 0x28 : images are in colour with alpha
   */
  uint16_t depth() const;

  /*!
   * @brief Access the JXS decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including the
   * "Codestream_Header" required to initialize a decoder.
   *
   * @note This data structure is not optional for JXS and therefore, for valid MP4 input, this
   * function always returns a valid pointer. If the it returns a nullptr the JXS track of the MP4
   * file is invalid/malformed.
   */
  std::unique_ptr<config::CJxsDecoderConfigRecord> jxsDecoderConfigRecord() const;

  /*!
   * @brief Access extra data from the JPEG XS sample description box.
   *
   * Contains (for example) information about the color of this track.
   */
  SJpegxsExtraData jpegxsExtraData() const;

 private:
  struct PimplJxs;
  std::unique_ptr<PimplJxs> pjxs;
};

/*!
 * @brief Versatile Video Coding (VVC/H.266) specific track reader
 *
 * This reader can be used to read VVC video tracks and gives access to codec specific metadata.
 * The format of the @ref CSample payload is defined as one raw (no encapsulation) VVC video access
 * unit (AU) per @ref CSample containing several NALUs, each one prefixed with its own size. For
 * details refer to ISO/IEC 14496-15.
 *
 * \ingroup mp4trackreader
 */
class CVvcTrackReader : public ITrackReader {
 public:
  /*!
   * @brief Creates a VVC video track reader for a given track index
   *
   * @param tracknumber 0-based index of the track to read from. Can be retrieved from @ref
   * CTrackInfo structure.
   *
   * @note Needs to be created via CIsobmffReader::trackByIndex function call.
   * @code auto treader = reader.trackByIndex<CVvcTrackReader>(tracknumber); @endcode
   */
  CVvcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber);
  virtual ~CVvcTrackReader() override;

  /*!
   * @brief Reads the next sample (state is maintained in track reader)
   *
   * If preallocate is set to true, this function will resize the provided sample to the maximum
   * sample size of this track. This avoids memory reallocation if the sample is re-used for
   * multiple read operations.
   *
   * The @ref SVvcSample structure is a wrapper around a @ref CSample, allow access to each separate
   * NALU. The begin iterator points directly to the NALU data and skips the prefixed size field.
   *
   * @param [out] vvcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   */
  SSampleExtraInfo nextSample(SVvcSample& vvcSample, bool preallocate = true) const;
  /*!
   * @brief Reads sample at a specified index
   *
   * Read a particular sample specified by a 0-based index.
   *
   * The @ref SVvcSample structure is a wrapper around a @ref CSample, allow access to each separate
   * NALU. The begin iterator points directly to the NALU data and skips the prefixed size field.
   *
   * @param [in] sampleIndex 0-based index indicating which sample to read.
   * @param [out] vvcSample Sample data containing one access unit (AU). If empty, there is no
   * sample for the given index.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByIndex, the returned sample will be index + 1.
   */
  SSampleExtraInfo sampleByIndex(size_t sampleIndex, SVvcSample& vvcSample,
                                 bool preallocate = true) const;
  /*!
   * @brief Reads sample by seeking to the user given time point and fulfilling the seek mode
   * requirements
   *
   * Seeking interface to read a sample by seeking to a specific point in time. The seeking mode
   * used can be configured, see @ref SSeekConfig for more details.
   *
   * The @ref SVvcSample structure is a wrapper around a @ref CSample, allow access to each separate
   * NALU. The begin iterator points directly to the NALU data and skips the prefixed size field.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @param [out] vvcSample Sample data containing one access unit (AU). If empty, track is EOS.
   * @param [in] preallocate If set to true memory is automatically allocated to the biggest sample
   * of this track to avoid reallocation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note End of stream is signalled via an empty sample. Make sure to check for each sample.
   * @note This function will set a new reference point for future @ref nextSample calls. If @ref
   * nextSample is called after calling sampleByTimestamp, the returned sample will be the next
   * sample that follows the one returned by sampleByTimestamp.
   */
  SSampleExtraInfo sampleByTimestamp(const SSeekConfig& seekConfig, SVvcSample& vvcSample,
                                     bool preallocate = true) const;
  /*!
   * @brief Resolves the sample information for seeking to the user given time point and fulfilling
   * the seek mode requirements.
   *
   * Can be used to simulate seeking and retrieve the timestamp it would generate without actually
   * performing the seek operation.
   *
   * @param [in] seekConfig Seeking mode configuration to control the seeking operation.
   * @return Extra information containing (for example) timestamp information of the retrieved
   * sample.
   *
   * @note This function is read-only and does not set the internal position to the given seek time
   * point.
   */
  SSampleExtraInfo resolveTimestamp(const SSeekConfig& seekConfig) const;

  /*!
   * @brief Get coding name as given in the 'stsd' box
   *
   * @return Fourcc of the codec stored in this track
   */
  ilo::Fourcc codingName() const;
  //! The cropped video frame width dimension (visual representation width) in pixel
  uint16_t width() const;
  //! The cropped video frame height dimension (visual representation height) in pixel
  uint16_t height() const;
  //! Compressor name as stored in the sample entry
  std::string compressorName() const;
  /*!
   * @brief Depth as stored in the sample entry (special format, not in bits)
   *
   * @note Allowed values are specified in ISO/IEC 14496-15 chapter 4.5 (Template fields used)
   * - 0x18 : the video sequence is in colour with no alpha
   * - 0x28 : the video sequence is in grayscale with no alpha
   * - 0x20 : the video sequence has alpha (gray or colour)
   */
  uint16_t depth() const;

  /*!
   * @brief Access the VVC decoder configuration record as stored below the 'stsd' box
   *
   * This data structure contains several codec specific data fields including any non-VCL NALUs
   * required to initialize a decoder.
   *
   * @note This data structure is not optional for VVC and therefore, for valid MP4 input, this
   * function always returns a valid pointer. If the it returns a nullptr the VVC track of the MP4
   * file is invalid/malformed.
   */
  std::unique_ptr<config::CVvcDecoderConfigRecord> vvcDecoderConfigRecord() const;

 private:
  struct PimplVvc;
  std::unique_ptr<PimplVvc> pvvc;
};
}  // namespace isobmff
}  // namespace mmt
