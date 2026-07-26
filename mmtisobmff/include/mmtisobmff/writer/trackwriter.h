/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2017 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file trackwriter.h
 * @brief Interface for creating MP4 tracks and writing samples
 * \defgroup mp4trackwriter Main interface for creating MP4 tracks and writing samples
 *
 * Main interface for creating MP4 tracks and writing samples
 */

#pragma once

// System includes
#include <memory>

// External includes
#include "ilo/common_types.h"
#include "ilo/string_utils.h"

// Internal includes
#include "mmtisobmff/writer/writer.h"
#include "mmtisobmff/configdescriptor/mha_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/avc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/hevc_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/jxs_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/vvc_decoderconfigrecord.h"

namespace mmt {
namespace isobmff {
//! Config for trackwriters that is common to all writers
struct ITrackConfig {
  /*!
   * @brief Fourcc of the sample entry to be written (required)
   *
   * @note This value is automatically filled by specific track writers.
   */
  ilo::Fourcc codingName = ilo::toFcc("0000");
};

/*!
 * @brief Interface for codec specific track writers
 *
 * A track writer allows inserting track specific metadata into an MP4 file
 * (like sample related information). The metadata supported is defined
 * per specific track writer type.
 */
struct ITrackWriter {
  virtual ~ITrackWriter() {}

  /*!
   * @brief Adds an isobmff sample
   *
   * For audio this means one access unit (AU).\n
   * For video this means NALUs belonging to 1 picture prefixed with their sizes (No AnnexB).
   */
  virtual void addSample(const CSample& sample) = 0;
  //! Add an edit list entry that further describes this track (optional)
  virtual void addEditListEntry(const SEdit& entry) = 0;
  /*!
   * @brief Add track based user defined data at track level (optional)
   *
   * Each call of this function will generate a child box in the 'udta' container box in 'trak'.
   *
   * The structure of the user data buffer is defined in ISO/IEC 14496-12, Clause 4.2 and looks
   * likes this:
   * @code
   * unsigned int(32) size (in bytes)
   * unsigned int(32) fourCC
   * unsigned int(8*(size - 8 bytes)) payload
   * @endcode
   *
   * @param [in] data The data to be added. The buffer is expected to be formatted as previously
   * described.
   *
   * @note The buffer structure must all be big endian style.
   */
  virtual void addUserData(const ilo::ByteBuffer& data) = 0;
};

//! Basic track writer
class CTrackWriter : public ITrackWriter {
 public:
  friend class CAdvancedTrackWriter;
  /*!
   * @brief Creates CTrackWriter object from an active writer instance and track config
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call
   */
  template <typename TConfig>
  CTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl, const TConfig& config);
  virtual ~CTrackWriter() override;

  virtual void addSample(const CSample& sample) override;
  void addEditListEntry(const SEdit& entry) override final;
  void addUserData(const ilo::ByteBuffer& data) override final;

 protected:
  struct SPimpl;
  std::unique_ptr<SPimpl> m_pimpl;

 private:
  /*!
   * @brief Advanced function to overwrite the base media decode time.
   *
   * Only accessible via CAdvancedTrackWriter
   */
  void overwriteBaseMediaDecodeTime(uint64_t newBmdtOffset);
};

//! Basic config valid for all track types
struct STrackConfig : ITrackConfig {
  /*!
   * @brief ID being inserted into 'tkhd' or 'tfhd' track_ID field (optional)
   *
   * If left at 0, the value will be auto-computed
   */
  uint32_t trackID = 0;
  /*!
   * @brief Timescale for the media contained in this track (required)
   *
   * For audio: It is recommended to set this to the value of the audio sample rate.
   * For video: It is recommended to set this to the value of the frame rate (the denominator part
   * of it)
   */
  uint32_t mediaTimescale = 0;
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

//! Basic audio config valid for all audio track types
struct SBaseAudioConfig : STrackConfig {
  //! Audio sample rate in Hz (required)
  uint32_t sampleRate = 0;
  //! Track audio language (required) (if multiple or undefined use "und")
  ilo::IsoLang language = ilo::toIsoLang("und");
};

//! Basic video config valid for all video track types
struct SBaseVideoConfig : STrackConfig {
  //! Width of the picture frame in pixels (required)
  uint16_t width = 0;
  //! Height of the picture frame in pixels (required)
  uint16_t height = 0;
  /*!
   * @brief Informative coding system name (optional)
   *
   * @note A suitable default value is automatically set by the
   * specific track writers.
   */
  std::string compressorName = "";
};

/* ######---MPEGH Track Writer---###### */

//! General MPEG-H config valid for all MPEG-H track types
struct SMpeghTrackConfig : SBaseAudioConfig {
  /*!
   * @brief Config record defining multiple aspects of the coding system (required/optional)
   *
   * For mha based systems this is required.\n
   * For mhm based systems this is optional, but recommended.
   */
  std::unique_ptr<config::CMhaDecoderConfigRecord> configRecord = nullptr;
  /*!
   * @brief MPEG-H profile and level compatibility sets (optional)
   *
   * If set, the library will generate an 'mhap' box containing the set of supported profile and
   * levels.
   */
  std::vector<uint8_t> profileAndLevelCompatibleSets;
};

//! MPEG-H config for MHM1 (MHAS)
struct SMpeghMhm1TrackConfig : SMpeghTrackConfig {
  SMpeghMhm1TrackConfig() { codingName = ilo::toFcc("mhm1"); }
};

//! MPEG-H config for MHM2 (MHAS with multi stream capabilities)
struct SMpeghMhm2TrackConfig : SMpeghTrackConfig {
  SMpeghMhm2TrackConfig() { codingName = ilo::toFcc("mhm2"); }
};

//! MPEG-H config for MHA1 (Raw AUs)
struct SMpeghMha1TrackConfig : SMpeghTrackConfig {
  SMpeghMha1TrackConfig() { codingName = ilo::toFcc("mha1"); }
};

/*!
 * @brief Track writer for the MPEG-H codec
 *
 * The format of the @ref CSample payload follows the structure defined in ISO/IEC 23008-3 chapter
 * 20 (Carriage of MPEG-H 3D audio in ISO base media file format).
 *
 * @note One @ref CSample shall only contain one MPEG-H access unit (AU). For RAW (mha) samples
 * without encapsulation this is a 1:1 mapping between an MPEG-H AU and a @ref CSample. For MHAS
 * (mhm) encapsulation all MHAS packets belonging to one audio AU must be packed into one @ref
 * CSample.
 *
 * \ingroup mp4trackwriter
 */
class CMpeghTrackWriter : public CTrackWriter {
 public:
  /*!
   * @brief Creates an MHM1 (MHAS in MP4) based MPEG-H track writer
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CMpeghTrackWriter>(config) @endcode
   * @note The @ref CSample structure shall contain MHAS packets belonging to one single audio
   * access unit (AU).
   */
  CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                    const SMpeghMhm1TrackConfig& config);
  /*!
   * @brief Creates an MHM2 (multi stream, MHAS in MP4) based MPEG-H track writer
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CMpeghTrackWriter>(config) @endcode
   * @note The @ref CSample structure shall contain MHAS packets belonging to one single audio
   * access unit (AU).
   */
  CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                    const SMpeghMhm2TrackConfig& config);
  /*!
   * Creates an MHA1 (Raw AUs in MP4) based MPEG-H track writer
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CMpeghTrackWriter>(config) @endcode
   * @note The @ref CSample structure shall contain one raw access unit (AU).
   */
  CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                    const SMpeghMha1TrackConfig& config);
  virtual ~CMpeghTrackWriter() override;
};

/* ######---MP4a Track Writer---###### */

//! AAC config for MP4A (Raw AUs)
struct SMp4aTrackConfig : SBaseAudioConfig {
  SMp4aTrackConfig() { codingName = ilo::toFcc("mp4a"); }
  //! Config record defining multiple aspects of the coding system (required)
  std::unique_ptr<config::CMp4aDecoderConfigRecord> configRecord;
  //! Number of audio channels
  uint16_t channelCount = 0;
};

/*!
 * @brief Track writer for the AAC codec family
 *
 * The format of the @ref CSample payload is defined as one raw (no encapsulation) AAC audio access
 * unit (AU) per @ref CSample.
 *
 * @note One @ref CSample shall only contain one AAC AU. ADTS, LATM and LATM/LOAS
 * encapsulation layers are not allowed.
 *
 * \ingroup mp4trackwriter
 */
class CMp4aTrackWriter : public CTrackWriter {
 public:
  /*!
   * @brief Creates an MP4A based AAC track writer
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CMp4aTrackWriter>(config) @endcode
   * @note The @ref CSample structure shall contain one raw access unit (AU). It shall not contain
   * any encapsulation layers like ADTS, LATM or LATM/LOAS.
   */
  CMp4aTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                   const SMp4aTrackConfig& config);
  virtual ~CMp4aTrackWriter() override;
};

/* ######---AVC Track Writer---###### */

//! AVC config for avc1 (Raw AUs)
struct SAvcTrackConfig : SBaseVideoConfig {
  SAvcTrackConfig() {
    codingName = ilo::toFcc("avc1");
    compressorName = "AVC Coding";
  }

  //! Config record defining multiple aspects of the coding system (required)
  std::unique_ptr<config::CAvcDecoderConfigRecord> configRecord = nullptr;
};

/*!
 * @brief Track writer for the H.264/AVC codec
 *
 * \ingroup mp4trackwriter
 */
class CAvcTrackWriter : public CTrackWriter {
 public:
  /*!
   * Creates an AVC1 based AVC track writer
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CAvcTrackWriter>(config) @endcode
   */
  CAvcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl, const SAvcTrackConfig& config);
  virtual ~CAvcTrackWriter() override;

  /*!
   * @brief Adds an isobmff sample. For AVC this means NALUs prefixed with sizes (No AnnexB)
   *
   * Can (for example) be used if video encoder supports outputting samples in isobmff sample
   * format, when doing a track copy, when using the generic track reader (CGenericTrackReader) or
   * the generic video track reader (CGenericVideoTrackReader)
   *
   * @note The @ref CSample structure shall contain one raw access unit (AU). The AU must include
   * all NALUs belonging to a picture. Each NALU must be prefixed with a size field. It shall not
   * contain any encapsulation layers like AnnexB.
   */
  void addSample(const CSample& sample) override;
  /*!
   * @brief Adds an AVC sample. Contains CSample + NALU separation (No AnnexB)
   *
   * Special sample structure that has pointer into underlying CSample for each NALU. Useful
   * in combination with CAvcTrackReader which outputs only SAvcSample types of samples.
   *
   * @note The underlying @ref CSample structure shall contain one raw access unit (AU).
   * The AU must include all NALUs belonging to a picture. Each NALU must be prefixed with
   * a size field. It shall not contain any encapsulation layers like AnnexB.
   */
  void addSample(const SAvcSample& sample);
  /*!
   * @brief Adds AVC video NALUs with metadata (converts data structure into CSample) (also supports
   * AnnexB)
   *
   * Useful when video encoder does not provide isobmff formatted buffers, but either raw or AnnexB
   * formatted buffers with separated NALUs.
   *
   * @note If the encoder only outputs AnnexB byte stream syntax a NALU splitter must be run first
   * to use this structure.
   * @note The structure shall only contain NALUs belonging to exactly one picture.
   */
  void addSample(const SAvcNalus& nalus);

 private:
  std::unique_ptr<config::CAvcDecoderConfigRecord> m_decoderConfigRecord;
};

/* ######---HEVC Track Writer---###### */

//! General HEVC config valid for all HEVC track types
struct SHevcTrackConfig : SBaseVideoConfig {
  //! Config record defining multiple aspects of the coding system (required)
  std::unique_ptr<config::CHevcDecoderConfigRecord> configRecord = nullptr;
};

/*!
 * @brief HEVC config for HVC1 (Raw AUs)
 *
 * @note All non-VCL NALUs must be contained in the configRecord. No in-band configuration updates
 * are allowed.
 */
struct SHvc1TrackConfig : SHevcTrackConfig {
  SHvc1TrackConfig() {
    codingName = ilo::toFcc("hvc1");
    compressorName = "HEVC Coding";
  }
};

/*!
 * @brief HEVC config for HEV1 (Raw AUs)
 *
 * @note Not all non-VCL NALUs must be contained in the configRecord. In-band configuration updates
 * are allowed.
 */
struct SHev1TrackConfig : SHevcTrackConfig {
  SHev1TrackConfig() {
    codingName = ilo::toFcc("hev1");
    compressorName = "HEVC Coding";
  }
};

/*!
 * @brief Track writer for the H.265/HEVC codec
 *
 * \ingroup mp4trackwriter
 */
class CHevcTrackWriter : public CTrackWriter {
 public:
  /*!
   * @brief Creates an HEVC track writer of type hvc1 or hev1
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CHevcTrackWriter>(config) @endcode
   */
  CHevcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                   const SHevcTrackConfig& config);
  virtual ~CHevcTrackWriter() override;

  /*!
   * @brief Adds an isobmff sample. For HEVC this means NALUs prefixed with sizes (No AnnexB)
   *
   * Can (for example) be used if video encoder supports outputting samples in isobmff sample
   * format, when doing a track copy, when using the generic track reader (CGenericTrackReader) or
   * the generic video track reader (CGenericVideoTrackReader)
   *
   * @note The @ref CSample structure shall contain one raw access unit (AU). The AU must include
   * all NALUs belonging to a picture. Each NALU must be prefixed with a size field. It shall not
   * contain any encapsulation layers like AnnexB.
   */
  void addSample(const CSample& sample) override;
  /*!
   * @brief Adds an HEVC sample. Contains CSample + NALU separation (No AnnexB)
   *
   * Special sample structure that has pointer into underlying CSample for each NALU. Useful
   * in combination with CHevcTrackReader which outputs only SHevcSample types of samples.
   *
   * @note The underlying @ref CSample structure shall contain one raw access unit (AU).
   * The AU must include all NALUs belonging to a picture. Each NALU must be prefixed with
   * a size field. It shall not contain any encapsulation layers like AnnexB.
   */
  void addSample(const SHevcSample& sample);
  /*!
   * @brief Adds HEVC video NALUs with metadata (converts data structure into CSample) (supports
   * also AnnexB)
   *
   * Useful when video encoder does not provide isobmff formatted buffers, but either raw or AnnexB
   * formatted buffers with separated NALUs.
   *
   * @note If the encoder only outputs AnnexB byte stream syntax a NALU splitter must be run first
   * to use this structure.
   * @note The structure shall only contain NALUs belonging to exactly one picture.
   */
  void addSample(const SHevcNalus& nalus);

 private:
  std::unique_ptr<config::CHevcDecoderConfigRecord> m_decoderConfigRecord;
};

/* ######--- JXS Track Writer ---###### */

//! Jxs config for jxsm (Raw AUs)
struct SJxsTrackConfig : SBaseVideoConfig {
  SJxsTrackConfig() {
    codingName = ilo::toFcc("jxsm");
    compressorName = "JXS Coding";
  }

  //! Config record defining multiple aspects of the coding system (required)
  std::unique_ptr<config::CJxsDecoderConfigRecord> configRecord;
  //! Further JPEG-XS specific metadata defined in the JXS standard (required)
  std::unique_ptr<SJpegxsExtraData> jxsExtraData;
};

/*!
 * @brief Track writer for the JPEG-XS codec
 *
 * \ingroup mp4trackwriter
 */
class CJxsTrackWriter : public CTrackWriter {
 public:
  /*!
   * @brief Creates an JXS track writer of type jxsm
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CJxsTrackWriter>(config) @endcode
   * @note For further details, please see ISO/IEC 21122-3 Annex B
   */
  CJxsTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl, const SJxsTrackConfig& config);
  virtual ~CJxsTrackWriter() override;

  /*!
   * @ brief Adds an isobmff sample according to the JXS specification
   *
   * The format of the @ref CSample payload is defined as one JXS codestream (called Picture())
   * without the Codestream_Header() as defined in Annex A-5.5 of ISO/IEC 21122-3
   */
  void addSample(const CSample& sample) override;
};

/* ######---VVC Track Writer---###### */

//! General VVC config valid for all VVC track types
struct SVvcTrackConfig : SBaseVideoConfig {
  //! Config record defining multiple aspects of the coding system (required)
  std::unique_ptr<config::CVvcDecoderConfigRecord> configRecord = nullptr;
};

/*!
 * @brief VVC config for VVC1 (Raw AUs)
 *
 * @note All non-VCL NALUs must be contained in the configRecord. No in-band configuration updates
 * are allowed.
 */
struct SVvc1TrackConfig : SVvcTrackConfig {
  SVvc1TrackConfig() {
    codingName = ilo::toFcc("vvc1");
    compressorName = "VVC Coding";
  }
};

/*!
 * @brief VVC config for VVI1 (Raw AUs)
 *
 * @note Not all non-VCL NALUs must be contained in the configRecord. In-band configuration updates
 * are allowed.
 */
struct SVvi1TrackConfig : SVvcTrackConfig {
  SVvi1TrackConfig() {
    codingName = ilo::toFcc("vvi1");
    compressorName = "VVC Coding";
  }
};

/*!
 * @brief Track writer for the H.266/VVC codec
 *
 * \ingroup mp4trackwriter
 */
class CVvcTrackWriter : public CTrackWriter {
 public:
  /*!
   * @brief Creates a VVC track writer of type vvc1 or vvi1
   *
   * @note Needs to be created via CIsobmffWriter::trackWriter function call.
   * @code auto twriter = writer.trackWriter<CVvcTrackWriter>(config) @endcode
   */
  CVvcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl, const SVvcTrackConfig& config);
  virtual ~CVvcTrackWriter() override;

  /*!
   * @brief Adds an isobmff sample. For VVC this means NALUs prefixed with sizes (No AnnexB)
   *
   * Can (for example) be used if video encoder supports outputting samples in isobmff sample
   * format, when doing a track copy, when using the generic track reader (CGenericTrackReader) or
   * the generic video track reader (CGenericVideoTrackReader)
   *
   * @note The @ref CSample structure shall contain one raw access unit (AU). The AU must include
   * all NALUs belonging to a picture. Each NALU must be prefixed with a size field. It shall not
   * contain any encapsulation layers like AnnexB.
   */
  void addSample(const CSample& sample) override;
  /*!
   * @brief Adds an VVC sample. Contains CSample + NALU separation (No AnnexB)
   *
   * Special sample structure that has pointer into underlying CSample for each nalu. Useful
   * in combination with CVvcTrackReader which outputs only SVvcSample types of samples.
   *
   * @note The underlying @ref CSample structure shall contain one raw access unit (AU).
   * The AU must include all NALUs belonging to a picture. Each NALU must be prefixed with
   * a size field. It shall not contain any encapsulation layers like AnnexB.
   */
  void addSample(const SVvcSample& sample);
  /*!
   * @brief Adds VVC video NALUs with metadata (converts data structure into CSample) (supports also
   * AnnexB)
   *
   * Useful when video encoder does not provide isobmff formatted buffers, but either raw or AnnexB
   * formatted buffers with separated NALUs.
   *
   * @note If the encoder only outputs AnnexB byte stream syntax a NALU splitter must be run first
   * to use this structure.
   * @note The structure shall only contain NALUs belonging to exactly one picture.
   */
  void addSample(const SVvcNalus& nalus);

 private:
  std::unique_ptr<config::CVvcDecoderConfigRecord> m_decoderConfigRecord;
};
}  // namespace isobmff
}  // namespace mmt
