/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2022 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file vvc_decoderconfigrecord.h
 * @brief Definition of a VVC config record
 *
 * Config record class holding VVC specific data required to
 * initialize a decoder
 */

#pragma once

// System includes
#include <vector>
#include <map>

// External includes
#include "ilo/common_types.h"
#include "ilo/bitparser.h"
#include "ilo/bitbuffer.h"

// Internal includes
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
namespace config {

/*!
 * @brief Allowed NALU types in config record
 *
 * According to ISO/IEC 14496-15 section 11.2.4.2.2
 * only these NALU type are allowed in the config record
 */
static const std::array<uint8_t, 7> ALLOWED_VVC_NALU_TYPES{
    12, /**< OPI */
    13, /**< DCI */
    14, /**< VPS */
    15, /**< SPS */
    16, /**< PPS */
    17, /**< prefix APS */
    23  /**< prefix SEI */
};

/*!
 * @brief The VVC decoder config record holding data needed to initialize a decoder out of band.
 *
 * Details on the fields contained here can be taken from ISO/IEC 14496-15 section 11.2.4.2
 */
class CVvcDecoderConfigRecord {
 public:
  /*!
   * @brief Vector holding non-VCL NALUs of same type
   *
   * @see @ref SVvcArray
   */
  using NaluVector = std::vector<ilo::ByteBuffer>;

  /*!
   * @brief Struct describing details of one non-VCL NALU type
   *
   * According to the standard, there can be several non-VCL NALUs
   * of the same type in the same config record. If so, they also might
   * have a fixed order. This structure allows to preserve this.
   *
   * It is also possible that some of the NALUs are present in the config
   * record and the bitstream, that all of the NALUs of a type are in the
   * config record or that none of the NALUs of a type are in the config
   * record. See @ref SVvcArray::arrayCompleteness for details on this.
   */
  struct SVvcArray {
    /*!
     * @brief Indicator if all NALUs of @ref SVvcArray::naluType
     * are completely part of the config record or not
     *
     * If true, all NALUs of @ref SVvcArray::naluType are part of the
     * config record and no in-band reconfiguration will happen.
     *
     * If false, not all NALUs of @ref SVvcArray::naluType are part of
     * the config record and in-band reconfiguration might happen.
     */
    bool arrayCompleteness = false;
    /*!
     * @brief Type of NALUs stored in @ref SVvcArray::nalus
     *
     * According to 14496-15 section 11.2.4.2.2, not all NALUs are
     * allowed to be stored in the config record. Please see
     * ALLOWED_VVC_NALU_TYPES  for details
     */
    uint8_t naluType = 0;
    /*!
     * @brief NALUs stored in the config record of type SVvcArray::naluType
     *
     * @note According to ISO/IEC 14496-15 section 11.2.4.2.2 NALUs of
     * type DCI_NUT and APS_NUT do not contain payload, so for these types
     * the NALU vector is empty.
     */
    NaluVector nalus;
  };

  /*!
   * @brief Array of different non-VCL NALU groups
   *
   * Describes all the NALUs found as part of the config record
   * including their order.
   *
   * @note The order of the NALUs described by this array
   * can matter. Please consult 14496-15 for details.
   */
  using NonVclArrays = std::vector<SVvcArray>;

  /*!
   * @brief Collection of fields related to Profile Tier Level record (PTL)
   *
   * User-facing info/config struct about all fields connected to the
   * VVC PTL of the config record. For details about the content please
   * see ISO/IEC 14496-15 section 11.2.4.1 and 11.2.4.2
   */
  struct SVvcPtl {
    /*!
     * @brief Profile Tier Level record (PTL)
     *
     * User-facing info/config struct about VVC PTL section
     * of the config record. For details about the content please
     * see ISO/IEC 14496-15 section 11.2.4.1
     */
    struct SVvcPtlRecord {
      uint8_t generalProfileIdc = 0;
      bool generalTierFlag = false;
      uint8_t generalLevelIdc = 0;
      bool ptlFrameOnlyConstraintFlag = false;
      bool ptlMultiLayerEnabledFlag = false;
      /*!
       * @brief GCI structure as defined in ISO/IEC 23090-3 - 7.3.3.2 (general_constraints_info())
       *
       * The data must follow the structure from ISO/IEC 23090-3, so the buffer must
       * be read and written MSBF incl. potential 0-filling for byte-alignment at the end.
       */
      ilo::ByteBuffer generalConstraintInfo;
      /*!
       * @brief Mapping of sublayer index to sublayerLevelIdcs
       *
       * @note Not all sublayers have an explicit idc entry! Make sure
       * to check map before accessing.
       * @note Read ISO/IEC 14496-15 section 11.2.4.1.3 on how to
       * interpret missing idcs. Order of the sublayerLevelIdcs matters!
       */
      std::map<uint8_t, uint8_t> sublayerLevelIdcs;
      /*!
       * @brief Combination of ptl_num_sub_profiles and general_sub_profile_idc
       *
       * @note The vector size of generalSubProfileIdcs is equal to ptl_num_sub_profiles in the
       * standard.
       */
      std::vector<uint32_t> generalSubProfileIdcs;
    };

    uint16_t olsIdx = 0;
    uint8_t numSublayers = 0;
    uint8_t constantFrameRate = 0;
    uint8_t chromaFormatIdc = 0;
    uint8_t bitDepthMinus8 = 0;
    SVvcPtlRecord nativePtl;
    uint16_t maxPictureWidth = 0;
    uint16_t maxPictureHeight = 0;
    uint16_t avgFrameRate = 0;
  };

  //! Constructor for creating the config record by parsing a buffer
  CVvcDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                          const ilo::ByteBuffer::const_iterator& end);
  //! Constructor for creating an empty config record for manual filling
  CVvcDecoderConfigRecord() = default;

  //! Query the lengthSizeMinusOne field
  uint8_t lengthSizeMinusOne() const;
  /*!
   * @brief Check if @ref SVvcPtl struct is present.
   *
   * @note Must be called before accessing @ref CVvcDecoderConfigRecord::vvcPtl.
   */
  bool vvcPtlPresent() const;
  /*!
   * @brief Get access to the @ref SVvcPtl fields
   *
   * @note Will throw an exception if no valid data is present. Call @ref
   * CVvcDecoderConfigRecord::vvcPtlPresent() first.
   */
  const SVvcPtl& vvcPtl() const;
  //! Get access to the non-VCL NALUs stored in the config record
  const NonVclArrays& nonVclArrays() const;

  /*!
   * @brief Set the lengthSizeMinusOne field (required)
   *
   * Configures the length prefix in bytes - 1 for NALUs in
   * 'mdat' payload.
   *
   * @note Allowed values are 0, 1 and 3
   */
  void setLengthSizeMinusOne(uint8_t lengthSizeMinusOne);
  /*!
   * @brief Set the @ref SVvcPtl struct (optional)
   *
   * Only required if data from the PTL section in the standard
   * should be set.
   *
   * @note This covers everything in ISO/IEC 14496-15 section 11.2.4.2.2
   * that is covered by the 'ptl_present_flag' if statement.
   */
  void setPtl(const SVvcPtl& ptl);
  /*!
   * @brief Set the non-VCL NALUs that shall be contained in the config record (required/optional)
   *
   * Only required if non-VCL NALUs shall be added to the config record. This depends on the
   * VVC track type that should be written.
   *
   * @see @ref SVvc1TrackConfig and @ref SVvi1TrackConfig
   */
  void setNonVclArrays(const NonVclArrays& nonVclArrays);

  /*!
   * @brief Serialize the class into a byte buffer according to ISO/IEC 14496-15
   *
   * @note This is only required for the @ref tools::SEasyTrackConfig feature from
   * 'commonhelpertools.h'. The standard track writer will do this themselves.
   */
  void write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position);

  /*!
   * @brief Query the size of this class structure
   *
   * Needed in combination with @ref CVvcDecoderConfigRecord::write to create a buffer big enough to
   * serialize into.
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
  void parsePtl(ilo::CBitParser& bitParser);
  void parsePtlRecord(ilo::CBitParser& bitParser);
  void parseConstraintInfo(ilo::CBitParser& bitParser, uint8_t numBytesConstraintInfo);
  void writePtl(ilo::CBitBuffer& bitWriter);
  void writePtlRecord(ilo::CBitBuffer& bitWriter);
  uint32_t numValidBitsConstraintInfo(ilo::ByteBuffer::const_iterator& begin,
                                      const ilo::ByteBuffer::const_iterator& end) const;
  void writeConstraintInfo(ilo::CBitBuffer& bitWriter, uint32_t nrOfValidBits) const;
  uint32_t generalConstraintInfoSizeInBytes(uint32_t validGeneralConstraintInfoBits) const;

  uint8_t m_lengthSizeMinusOne = 255;
  bool m_ptlPresentFlag = false;
  SVvcPtl m_ptl;
  NonVclArrays m_nonVclArrays;
};
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
