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
 * @file hevc_decoderconfigrecord.h
 * @brief Definition of an HEVC config record
 *
 * Config record class holding HEVC specific data required to
 * initialize a decoder
 */

#pragma once

// System includes
#include <vector>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
namespace config {

/*!
 * @brief Allowed NALU types in config record
 *
 * According to ISO/IEC 14496-15 section 8.3.3.1.1
 * only these NALU type are allowed in the config record
 */
static const std::array<uint8_t, 4> ALLOWED_HEVC_NALU_TYPES{
    32, /**< VPS */
    33, /**< SPS */
    34, /**< PPS */
    39  /**< PREFIX_SEI */
};

/*!
 * @brief The HEVC decoder config record holding data needed to initialize a decoder out of band.
 *
 * Details on the fields contained here can be taken from ISO/IEC 14496-15 section 8.3.3.1
 */
class CHevcDecoderConfigRecord {
 public:
  /*!
   * @brief Vector holding non-VCL NALUs of same type
   *
   * @see @ref SHevcArray
   */
  using NaluVector = std::vector<ilo::ByteBuffer>;

  /*!
   * @brief Struct describing details of one non-VCL NALU type
   *
   * According to the specification, there can be several non-VCL NALUs
   * of the same type in the same config record. If so, they also might
   * have a fixed order. This structure allows to preserve this.
   *
   * It is also possible that some of the NALUs are present in the config
   * record and the bitstream, that all of the NALUs of a type are in the
   * config record or that none of the NALUs of a type are in the config
   * record. See @ref SHevcArray::arrayCompleteness for details on this.
   */
  struct SHevcArray {
    /*!
     * @brief Indicator for whether all NALUs of @ref SHevcArray::naluType
     * are completely part of the config record or not
     *
     * If true, all NALUs of @ref SHevcArray::naluType are part of the
     * config record and no in-band reconfiguration will happen.
     *
     * If false, not all NALUs of @ref SHevcArray::naluType are part of
     * the config record and in-band reconfiguration might happen.
     */
    bool arrayCompleteness = false;
    /*!
     * @brief Type of NALUs stored in @ref SHevcArray::nalus
     *
     * According to 14496-15 section 8.3.3.1.1, not all NALUs are
     * allowed to be stored in the config record. Please see
     * ALLOWED_HEVC_NALU_TYPES for details
     */
    uint8_t naluType = 0U;
    /*!
     * @brief NALUs stored in the config record of type SHevcArray::naluType
     *
     */
    NaluVector nalus;
  };

  /*!
   * @brief Array of different non-VCL NALU groups
   *
   * Describes all the NALUs found as part of the config record
   * including their order.
   *
   * @note The recommended order of the NALUs described by this array is
   * VPS, SPS, PPS, SEI.
   *
   * @see ISO/IEC 14496-15 section 8.3.3.1 for details
   */
  using NonVclArrays = std::vector<SHevcArray>;

  //! Constructor for creating the config record by parsing a buffer
  CHevcDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                           const ilo::ByteBuffer::const_iterator& end);
  //! Constructor for creating an empty config record for manual filling
  CHevcDecoderConfigRecord();

  uint8_t configurationVersion() const { return m_configurationVersion; }
  uint8_t generalProfileSpace() const { return m_generalProfileSpace; }
  bool generalTierFlag() const { return m_generalTierFlag; }
  uint8_t generalProfileIdc() const { return m_generalProfileIdc; }
  uint32_t generalProfileCompatabilityFlags() const { return m_generalProfileCompatabilityFlags; }
  uint64_t generalConstraintIndicatorFlags() const { return m_generalConstraintIndicatorFlags; }
  uint8_t generalLevelIdc() const { return m_generalLevelIdc; }
  uint16_t minSpatialSegmentationIdc() const { return m_minSpatialSegmentationIdc; }
  uint8_t paralelismType() const { return m_paralelismType; }
  uint8_t chromaFormatIdc() const { return m_chromaFormatIdc; }
  uint8_t bitDepthLumaMinus8() const { return m_bitDepthLumaMinus8; }
  uint8_t bitDepthChromaMinus8() const { return m_bitDepthChromaMinus8; }
  uint16_t avgFrameRate() const { return m_avgFrameRate; }
  uint8_t constFrameRate() const { return m_constFrameRate; }
  uint8_t numTemporatlLayers() const { return m_numTemporatlLayers; }
  bool temporalIdNested() const { return m_temporalIdNested; }
  uint8_t lengthSizeMinusOne() const { return m_lengthSizeMinusOne; }
  const NonVclArrays& nonVclArrays() const { return m_nonVclArrays; }

  void setConfigurationVersion(uint8_t configurationVersion);
  void setGeneralProfileSpace(uint8_t generalProfileSpace);
  void setGeneralTierFlag(bool generalTierFlag);
  void setGeneralProfileIdc(uint8_t generalProfileIdc);
  void setGeneralProfileCompatabilityFlags(uint32_t generalProfileCompatabilityFlags);
  void setGeneralConstraintIndicatorFlags(uint64_t generalConstraintIndicatorFlags);
  void setGeneralLevelIdc(uint8_t generalLevelIdc);
  void setMinSpatialSegmentationIdc(uint16_t minSpatialSegmentationIdc);
  void setParalelismType(uint8_t paralelismType);
  void setChromaFormatIdc(uint8_t chromaFormatIdc);
  void setBitDepthLumaMinus8(uint8_t bitDepthLumaMinus8);
  void setBitDepthChromaMinus8(uint8_t bitDepthChromaMinus8);
  void setAvgFrameRate(uint16_t avgFrameRate);
  void setConstFrameRate(uint8_t constFrameRate);
  void setNumTemporatlLayers(uint8_t numTemporatlLayers);
  void setTemporalIdNested(bool temporalIdNested);
  void setLengthSizeMinusOne(uint8_t lengthSizeMinusOne);
  void setNonVclArrays(const NonVclArrays& nonVclArrays);

  /*!
   * @brief Serialize the class into a byte buffer according to ISO/IEC 14496-15
   *
   * @note This is only required for the @ref tools::SEasyTrackConfig feature from
   * 'commonhelpertools.h'. The standard track writer will do this itself.
   */
  void write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position);
  /*!
   * @brief Query the size of this class structure
   *
   * Needed in combination with @ref CHevcDecoderConfigRecord::write to create a buffer big enough
   * to serialize into.
   */
  uint64_t size() const;

  /*!
   * @brief A key/value attribute list containing name and value as strings.
   *
   * Can be used for generic printing
   */
  SAttributeList getAttributeList() const;

 private:
  void parse(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  uint8_t m_configurationVersion;
  uint8_t m_generalProfileSpace;
  bool m_generalTierFlag;
  uint8_t m_generalProfileIdc;
  uint32_t m_generalProfileCompatabilityFlags;
  uint64_t m_generalConstraintIndicatorFlags;
  uint8_t m_generalLevelIdc;
  uint16_t m_minSpatialSegmentationIdc;
  uint8_t m_paralelismType;
  uint8_t m_chromaFormatIdc;
  uint8_t m_bitDepthLumaMinus8;
  uint8_t m_bitDepthChromaMinus8;
  uint16_t m_avgFrameRate;
  uint8_t m_constFrameRate;
  uint8_t m_numTemporatlLayers;
  bool m_temporalIdNested;
  uint8_t m_lengthSizeMinusOne;
  NonVclArrays m_nonVclArrays;
};
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
