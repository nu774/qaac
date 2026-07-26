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
 * @file types.h
 * @brief Common types definitions used in public C++ interface
 */

#pragma once

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

// System includes
#include <vector>
#include <array>
#include <tuple>
#include <memory>
#include <string>
#include <stdexcept>

// External includes
#include "ilo/common_types.h"
#include "ilo/string_utils.h"

namespace mmt {
namespace isobmff {
/*!
 * @brief Supported seeking operation types
 *
 * Used to define starting point for file I/O access. @see IIsobmffInput
 */
enum class SeekingOrigin {
  beg, /**< Start file operation relative to beginning of the file */
  end, /**< Start file operation relative to ending of the file */
  cur  /**< Start file operation relative to current position in the file */
};

using pos_type = uint64_t;
using offset_type = int64_t;

/*!
 * @brief Supported track types
 *
 * The track types supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the track type is not known by the library.
 */
enum class TrackType : uint8_t {
  undefined = 0, /**< Unknown track */
  audio,         /**< Audio track */
  video,         /**< Video track */
  hint           /**< Hint track */
};

/*!
 * @brief Supported codecs
 *
 * The codec types supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the codec type is not known by the library.
 */
enum class Codec : uint8_t {
  undefined = 0, /**< Unknown codec */

  mp4a = 4,  /**< MP4a based audio codec (AAC, HE-AAC, HE-AACv2, xHE-AAC, etc.) */
  mpegh_mha, /**< MPEG-H MHA audio codec. RAW-AU in MP4 */
  mpegh_mhm, /**< MPEG-H MHM audio codec. MHAS in MP4 */

  mp4v = 100, /**< MPEG4 video codecs */
  avc,        /**< AVC/H.264 video codec */
  hevc,       /**< HEVC/H.265 video codec */
  jxs,        /**< JPEG XS video codec */
  vvc         /**< VVC/H.266 video codec */
};

/*!
 * @brief Sample groups
 *
 * The sample groups supported by this library (for reading and writing).
 * The value undefined is invalid for writing, but can occur while reading
 * if the sample group is not known by the library.
 */
enum class SampleGroupType : uint8_t {
  undefined = 0, /**< Unknown sample group */
  none,          /**< No sample group */
  prol,          /**< Sample group of type Roll-Recovery */
  roll,          /**< Sample group of type Pre-Roll */
  sap            /**< Sample group of type Stream-Access-Point */
};

/*!
 * @brief MP4 container overhead information
 *
 * Structure for storing information related to MP4 container overhead information.
 * Useful to see how much space is occupied by container metadata vs. payload.
 *
 * @note: This does not take 'mdat' alignment/stuffing bytes into account, if used.
 */
struct SOverheadInfo {
  /*! Size of the pure payload data contained in 'mdat' */
  uint64_t sizePayload = 0;
  /*! Size of the container metadata overhead */
  uint64_t sizeOverhead = 0;
};

/*!
 * @brief Sample group information
 *
 * Struct containing all meta data relevant for sample groups.
 */
struct SSampleGroupInfo {
  /*! Sample group type this sample belongs to */
  SampleGroupType type;
  /*!
   * The roll distance for this sample.
   * @note The definition of this depends on the sample group type. Only valid for sample groups of
   * type roll and prol.
   */
  int16_t rollDistance;
  /*!
   * The stream access type for this sample.
   * @note This can only be used for sample group type sap. Allowed range is [1 - 6] (inclusive)
   */
  uint8_t sapType;

  SSampleGroupInfo() : type(SampleGroupType::none), rollDistance(0), sapType(0U) {}

  SSampleGroupInfo(SampleGroupType t, int16_t r, uint8_t s)
      : type(t), rollDistance(r), sapType(s) {}

  void clear() {
    type = SampleGroupType::none;
    rollDistance = 0;
    sapType = 0;
  }

  bool empty() const { return (*this == SSampleGroupInfo()); }

  bool operator==(const SSampleGroupInfo& other) const {
    return (other.type == type && other.rollDistance == rollDistance && other.sapType == sapType);
  }

  bool operator!=(const SSampleGroupInfo& other) const { return !(*this == other); }
};

/*!
 * @brief Isobmff sample
 *
 * Contains both sample data as well as sample meta data (isobmff sample as defined in ISO/IEC
 * 14496-14 and 14496-15)
 */
struct CSample {
  /*! Constructs an empty sample and optionally reserves preallocByteSize bytes of raw memory */
  explicit CSample(size_t preallocByteSize = 0U)
      : duration(0), ctsOffset(0), isSyncSample(false), fragmentNumber(0) {
    rawData.reserve(preallocByteSize);
  }

  /*! Clear the sample - the sample is empty after the call */
  void clear() {
    duration = 0;
    ctsOffset = 0;
    rawData.clear();
    isSyncSample = false;
    fragmentNumber = 0;
    sampleGroupInfo.clear();
  }

  /*! Returns true if the sample it empty - newly constructed and cleared samples are empty */
  bool empty() const { return rawData.empty() && duration == 0; }
  /*!
   * Data block containing sample data as defined in ISO/IEC 14496-14 and 14496-15
   * @note Depending on the sample type the representation of data in this buffer is different.
   */
  ilo::ByteBuffer rawData;
  /*! Sample duration in ticks of track timescale */
  uint64_t duration;
  /*!
   * Sample composition time offset. The CTS offset is the difference between the presentation and
   * decoding timestamp.
   * @note The CTS offset is counted in ticks of the track timescale.
   * @note CTS offset is typically only used for video.
   */
  int64_t ctsOffset;
  /*!
   * Marks a sample as a SyncSample.
   * @note The definition of a sync sample is defined differently depending on the sample type and
   * is defined in the respective specification.
   */
  bool isSyncSample;
  /*!
   * Specifies whether this sample is part of a fragment and if yes, its index.\n
   * 0 : not part of a fragment, >= 1 : part of the numbered fragment
   */
  uint32_t fragmentNumber;
  /*! Describes what sample group this sample is part of (if any) */
  SSampleGroupInfo sampleGroupInfo;
};

/*!
 * @brief Sample for codecs that make use of Network Abstraction Layer Units (NALU)
 *
 * Requires an existing CSample and marks NALUs that are contained in the underlying C-Sample buffer
 * without copying.
 * Mostly used by specific track readers / writers to avoid extra buffer copy to access NALUs.
 *
 * NALUs are not stored as continuous data inside CSample but have size fields before each NALU. The
 * iterators of the sparse buffer point to each NALU start and end. NALUs also (in general) must be
 * in RAW format (without AnnexB encapsulation) for isobmff storage. <PRE>
 *  <-------------CSampleStructure------------>
 *  +--------+ +--------+ +--------+ +--------+
 *  |  Size  | |  NALU  | |  Size  | |  NALU  |
 *  +--------+ +--------+ +--------+ +--------+
 *             |->    <-|            |->    <-|
 *             beg    end            beg    end
 * </PRE>
 * @see VideoHelper for helper tools to help creating this samples or convert them to a different
 * format.
 * @see SBaseNalus for an AnnexB capable structure that can be converted into and from SNaluSample.
 */
struct SNaluSample {
  /*!
   * @brief Special buffer abstraction holding iterators into underlying real buffer.
   *
   * Useful to mark ranges in an existing buffer that can then be stored.
   */
  class CSparseBuffer {
    friend struct SNaluSample;

   public:
    ilo::ByteBuffer::const_iterator begin() const {
      validate(m_begin);
      return m_begin;
    }
    ilo::ByteBuffer::const_iterator end() const {
      validate(m_end);
      return m_end;
    }
    size_t size() const { return static_cast<size_t>(end() - begin()); }

   private:
    CSparseBuffer(const ilo::ByteBuffer& mother, ilo::ByteBuffer::const_iterator begin,
                  ilo::ByteBuffer::const_iterator end);

    void validate(const ilo::ByteBuffer::const_iterator& check) const {
      if (m_mother->begin() > check || m_mother->end() < check) {
        throw std::runtime_error("sparse buffer and raw data out of sync");
      }
    }

    ilo::ByteBuffer::const_iterator m_begin;
    ilo::ByteBuffer::const_iterator m_end;
    const ilo::ByteBuffer* m_mother;
  };

 public:
  /*! Constructs an empty SNaluSample. Can optionally reserve memory. */
  explicit SNaluSample(size_t preallocByteSize = 0U) : sample(preallocByteSize) {}

  /*! Add a NALU using begin and end iterators pointing to the raw data */
  void addNalu(ilo::ByteBuffer::const_iterator begin, ilo::ByteBuffer::const_iterator end) {
    nalus.push_back(CSparseBuffer(sample.rawData, begin, end));
  }

  /*! Clear sample - the sample is empty after the call */
  void clear() {
    nalus.clear();
    sample.clear();
  }

  /*! Check if the sample is empty (newly constructed or cleared samples are empty) */
  bool empty() const { return sample.empty(); }

  /*! Vector containing NALUs of sample as sparse buffers */
  std::vector<CSparseBuffer> nalus;
  /*! Underlying isobmff sample */
  CSample sample;
};

/*!
 * @brief Sample structure for AVC (aka H.264) (isobmff sample)
 *
 * Holds iterators to separate NALUs backed by a CSample (see SNaluSample)
 */
struct SAvcSample : SNaluSample {
  /*! Constructs an empty SAvcSample. Can optionally reserve memory. */
  SAvcSample(size_t preallocByteSize = 0U) : SNaluSample(preallocByteSize) {}
};

/*!
 * @brief Sample structure for HEVC (aka H.265) (isobmff sample)
 *
 * Holds iterators to separate NALUs backed by a CSample (see SNaluSample)
 */
struct SHevcSample : SNaluSample {
  /*! Constructs an empty SHevcSample. Can optionally reserve memory. */
  SHevcSample(size_t preallocByteSize = 0U) : SNaluSample(preallocByteSize) {}
};

/*!
 * @brief Sample structure for VVC (aka H.266) (isobmff sample)
 *
 * Holds iterators to separate NALUs backed by a CSample (see SNaluSample)
 */
struct SVvcSample : SNaluSample {
  /*! Constructs an empty SVvcSample. Can optionally reserve memory. */
  SVvcSample(size_t preallocByteSize = 0U) : SNaluSample(preallocByteSize) {}
};

/*!
 * @brief Definition for generic NALUs (vcl and non-VCL) with AnnexB support
 *
 * Useful when dealing with ByteBuffer input e.g. from a video encoder that either outputs RAW
 * byte format or AnnexB format. Each SBaseNalus is a collection for all NALUs belonging to **one**
 * picture.
 *
 * @note Each NALU belonging to one video frame must be added separately.
 *       If the encoder outputs an AnnexB stream, an extra AnneB parser is required to split the
 * stream into separate nalus.
 * @see VideoHelper for helper tools convert to other sample formats.
 */
struct SBaseNalus {
  /*!
   * Create SBaseNalus
   *
   * @param annexB Defines if the buffer should later contain RAW or AnnexB formatted NALUs.
   */
  SBaseNalus(const bool annexB) : m_isAnnexB(annexB) {}

  /*! Adds a buffer containing exactly 1 NALU */
  void addNalu(const ilo::ByteBuffer& naluBuffer) { m_nalus.push_back(naluBuffer); }

  /*! Get stored NALUs */
  const std::vector<ilo::ByteBuffer>& getNalus() const { return m_nalus; }

  /*! Returns true if SBaseNalus was created with annexB = true */
  bool isAnnexB() const { return m_isAnnexB; }

  /*! Resets all sample and buffer data */
  virtual void clear() {
    m_isAnnexB = false;
    m_nalus.clear();
  }

 private:
  bool m_isAnnexB;
  std::vector<ilo::ByteBuffer> m_nalus;
};

/*!
 * @brief Definition for video coding nalus (vcl) belonging to one picture
 *
 * @see SBaseNalus for more details
 */
struct SVideoNalus : SBaseNalus {
  /*!
   * @brief Meta data definition for video NALUs belonging to one picture
   *
   * For explanation of the members please @see CSample.
   */
  struct SMetaData {
    /*! Frame duration in unit ticks of track timescale */
    uint64_t duration = 0;
    /*! Composition Time Stamp Offset @see CSample */
    int64_t ctsOffset = 0;
    /*! Marks a SyncSample @see CSample */
    bool isSyncSample = false;
    /*! 0 == not part of a fragment, >= 1 part of the numbered fragment @see CSample */
    uint32_t fragmentNumber = 0;
    /*! Information, if this sample is part of a sample group @see CSample */
    SSampleGroupInfo sampleGroupInfo;

    /*! Clear all meta data information */
    void clear() {
      duration = 0;
      ctsOffset = 0;
      isSyncSample = false;
      fragmentNumber = 0;
      sampleGroupInfo.clear();
    }
  };

  SVideoNalus(const SMetaData& metaData, const bool annexB)
      : SBaseNalus(annexB), m_metaData(metaData) {}

  /*! Access metadata of this sample */
  const SMetaData& getMetaData() const { return m_metaData; }

  /*! Clear all sample data (payload and metadata) */
  void clear() override {
    SBaseNalus::clear();
    m_metaData.clear();
  }

 private:
  SMetaData m_metaData;
};

/*!
 * @brief Definition for AVC NALUs belonging to one picture
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SAvcNalus : SVideoNalus {
  SAvcNalus(const SMetaData& metaData, const bool annexB) : SVideoNalus(metaData, annexB) {}
};

/*!
 * @brief Definition for HEVC NALUs belonging to one picture
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SHevcNalus : SVideoNalus {
  SHevcNalus(const SMetaData& metaData, const bool annexB) : SVideoNalus(metaData, annexB) {}
};

/*!
 * @brief Definition for VVC NALUs belonging to one picture
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SVvcNalus : SVideoNalus {
  SVvcNalus(const SMetaData& metaData, const bool annexB) : SVideoNalus(metaData, annexB) {}
};

/*!
 * @brief Definition for AVC non-VCL NALUs for the AVC config record
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SAvcNonVclNalus : SBaseNalus {
  SAvcNonVclNalus(const bool annexB) : SBaseNalus(annexB) {}
};

/*!
 * @brief Definition for HEVC non-VCL NALUs for the HEVC config record
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SHevcNonVclNalus : SBaseNalus {
  SHevcNonVclNalus(const bool annexB) : SBaseNalus(annexB) {}
};

/*!
 * @brief Definition for VVC non-VCL NALUs for the HEVC config record
 *
 * @see SVideoNalus and @see SBaseNalus for more details
 */
struct SVvcNonVclNalus : SBaseNalus {
  SVvcNonVclNalus(const bool annexB) : SBaseNalus(annexB) {}
};

/*!
 * @brief Sample flags definition
 *
 * Details can be found in ISO/IEC 14496-12 - 8.8.3.1)
 */
struct SSampleFlags {
  enum class Leading : uint8_t { unknown = 0, yes_hasdep_nodec = 1, no = 2, yes_nodep_dec = 3 };

  enum class SampleDependsOn : uint8_t { unknown = 0, yes = 1, no = 2, reserved = 3 };

  enum class SampleIsDependedOn : uint8_t {
    unknown = 0,
    yes_not_disposable = 1,
    no_disposable = 2,
    reserved = 3
  };

  enum class SampleHasRedundancy : uint8_t { unknown = 0, yes = 1, no = 2, reserved = 3 };

  // 4 bits: reserved
  // 2 bits: is_leading
  // 2 bits: sample_depends_on
  // 2 bits: sample_is_depended_on
  // 2 bits: sample_has_redundancy
  // 3 bits: sample_padding_value
  // 1 bit : sample_is_non_sync_sample
  // 16 bits: sample_degradation_priority
  Leading isLeading = Leading::unknown;
  SampleDependsOn dependsON = SampleDependsOn::unknown;
  SampleIsDependedOn isDependedOn = SampleIsDependedOn::unknown;
  SampleHasRedundancy hasRedundancy = SampleHasRedundancy::unknown;
  uint8_t paddingValue = 0;
  bool isNonSyncSample = false;
  uint16_t degradationPriority = 0;
};

/*!
 * @brief Structure defining one Edit entry of an EditList
 *
 * Details can be found in ISO/IEC 14496-12 - 8.6.6)
 */
struct SEdit {
  /*! Duration of the edit in ticks based on movie time scale @see CMovieInfo */
  uint64_t segmentDuration = 0;
  /*! Start time within the media of the edit in media time scale @see CTrackInfo */
  int64_t mediaTime = 0;
  /*! Relative rate at which to play the media in the edit (0 specifies a dwell, should be 1
   * otherwise) */
  float mediaRate = 1.0;
};
using SEditList = std::vector<SEdit>;

//! Generic attribute definition. Used for generic printing
struct SAttribute {
  std::string key;
  std::string value;
};
using SAttributeList = std::vector<SAttribute>;

//! JpegXS color information struct
struct SColourInformation {
  ilo::Fourcc colourType = ilo::toFcc("0000");
  uint16_t colourPrimaries = 0;
  uint16_t transferCharacteristics = 0;
  uint16_t matrixCoefficients = 0;
  bool fullRangeFlag = true;
  ilo::ByteBuffer iccProfile;
};
using SColourInformationList = std::vector<SColourInformation>;

//! JPEG XS extra data struct
struct SJpegxsExtraData {
  uint32_t brat = 0;
  uint32_t frat = 0;
  uint16_t schar = 0;
  uint32_t tcod = 0;
  uint16_t ppih = 0;
  uint16_t plev = 0;
  SColourInformationList colourInformations;
};

//! Time stamp definition
class CIsoTimestamp {
 public:
  CIsoTimestamp();
  /*!
   * Creates a time stamp
   *
   * @param timescale Timescale used for PTS and DTS value
   * @param ptsValue Presentation timestamp in unit ticks of timescale
   * @param dtsValue Decoding timestamp in unit ticks of timescale
   */
  CIsoTimestamp(uint32_t timescale, uint64_t ptsValue, int64_t dtsValue);

  /*! Returns true if constructor with parameters was called */
  bool isValid() const;
  /*! Get timescale */
  uint32_t timescale() const;
  /*! Get presentation timestamp */
  uint64_t ptsValue() const;
  /*! Get decoding timestamp */
  int64_t dtsValue() const;

 private:
  uint32_t m_timescale = 0u;
  uint64_t m_ptsValue = 0u;
  int64_t m_dtsValue = 0;
  bool m_hasValue = false;
};

//! Sample duration definition
class CTimeDuration {
 public:
  CTimeDuration() {}
  /*!
   * Creates a CTimeDuration
   *
   * @param timescale Timescale used for duration value
   * @param duration Duration of the sample in unit ticks of timescale
   */
  CTimeDuration(uint32_t timescale, uint64_t duration);

  /*! Returns true if constructor with parameters was called */
  bool isValid() const;
  /*! Get timescale */
  uint32_t timescale() const;
  /*! Get duration */
  uint64_t duration() const;

 private:
  uint32_t m_timescale = 0u;
  uint64_t m_duration = 0u;
  bool m_hasValue = false;
};

//! Mode defining where to start after a seeking operation
enum class ESampleSeekMode : uint8_t {
  /*! Cannot be used */
  invalid = 0,
  /*! After seeking, continue at nearest sync sample position around a user provided target */
  nearestSyncSample = 1,
  /*! After seeking, continue at next sync sample position that is greater than a user provided
     target */
  nextSyncSampleGreater = 2,
  /*! After seeking, continue at last sync sample position that is smaller than a user provided
     target */
  lastSyncSampleSmaller = 3
};

//! Config defining a seeking operation
struct SSeekConfig {
  /*!
   * Create a seeking config
   *
   * @param sSeekPoint Time point where to seek to in the mp4 file
   * @param sSeekMode Mode defining how to perform the seeking operation
   */
  SSeekConfig(const CTimeDuration& sSeekPoint, ESampleSeekMode sSeekMode)
      : seekPoint(sSeekPoint), seekMode(sSeekMode) {}

  SSeekConfig() {}

  /*! Time point where to seek to in the MP4 file */
  CTimeDuration seekPoint;
  /*! Mode defining how to perform the seeking operation */
  ESampleSeekMode seekMode = ESampleSeekMode::invalid;
};

//! Additional sample related information not carried via CSample structure
struct SSampleExtraInfo {
  CIsoTimestamp timestamp;
};
}  // namespace isobmff
}  // namespace mmt

#ifdef _WIN32
#pragma warning(pop)
#endif
