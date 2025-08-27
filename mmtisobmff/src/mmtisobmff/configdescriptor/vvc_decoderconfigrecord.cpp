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

/*
 * Project: MPEG-4 ISO Base Media File Format (ISO BMFF) library
 * Content: Vvc decoder configuration record
 */

// System includes
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <numeric>
#include <cmath>

// External includes
#include "common/logging.h"
#include "ilo/bytebuffertools.h"
#include "ilo/bitparser.h"
#include "ilo/bitbuffer.h"

// Internal includes
#include "mmtisobmff/configdescriptor/vvc_decoderconfigrecord.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace config {
CVvcDecoderConfigRecord::CVvcDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                                                 const ilo::ByteBuffer::const_iterator& end) {
  parse(begin, end);
}

uint8_t CVvcDecoderConfigRecord::lengthSizeMinusOne() const {
  return m_lengthSizeMinusOne;
}

bool CVvcDecoderConfigRecord::vvcPtlPresent() const {
  return m_ptlPresentFlag;
}

const CVvcDecoderConfigRecord::SVvcPtl& CVvcDecoderConfigRecord::vvcPtl() const {
  ILO_ASSERT_WITH(m_ptlPresentFlag, std::logic_error,
                  "vvcPtl() cannot be called if there are no entries. Make sure to check with "
                  "vvcPtlPresent() first.");
  return m_ptl;
}

const CVvcDecoderConfigRecord::NonVclArrays& CVvcDecoderConfigRecord::nonVclArrays() const {
  return m_nonVclArrays;
}

void CVvcDecoderConfigRecord::setLengthSizeMinusOne(uint8_t lengthSizeMinusOneValue) {
  ILO_ASSERT_WITH(
      lengthSizeMinusOneValue == 0 || lengthSizeMinusOneValue == 1 || lengthSizeMinusOneValue == 3,
      std::invalid_argument,
      "Invalid lengthSizeMinusOneValue of %d. Allowed values are 0, 1 and 3.",
      lengthSizeMinusOneValue);
  m_lengthSizeMinusOne = lengthSizeMinusOneValue;
}

void CVvcDecoderConfigRecord::setPtl(const SVvcPtl& ptl) {
  if (ptl.numSublayers < 2) {
    ILO_ASSERT_WITH(
        ptl.nativePtl.sublayerLevelIdcs.empty(), std::invalid_argument,
        "NumSublayers in VvcPtlRecord is %d. If the value is smaller than 2 there cannot "
        "be any sublayerLevelIdcs present, but sublayerLevelIdc map contains %d entries.",
        ptl.numSublayers, ptl.nativePtl.sublayerLevelIdcs.size());
  } else {
    ILO_ASSERT_WITH(
        ptl.nativePtl.sublayerLevelIdcs.size() < ptl.numSublayers, std::invalid_argument,
        "The max allowed number of sublayerLevelIdcs (%d) must be smaller than numSublayers (%d).",
        ptl.nativePtl.sublayerLevelIdcs.size(), ptl.numSublayers);
  }

  ILO_ASSERT_WITH(!ptl.nativePtl.generalConstraintInfo.empty(), std::invalid_argument,
                  "generalConstraintInfo shall not be empty.");
  if (ptl.nativePtl.generalConstraintInfo.size() == 1) {
    ILO_ASSERT_WITH((ptl.nativePtl.generalConstraintInfo.at(0) >> 7) == 0, std::invalid_argument,
                    "A generalConstraintInfo structure with a size of 1 byte shall have the "
                    "gciPresentFlag bit (first bit) set to 0.");
  } else {
    ILO_ASSERT_WITH((ptl.nativePtl.generalConstraintInfo.at(0) >> 7) == 1, std::invalid_argument,
                    "A generalConstraintInfo structure with a size bigger than 1 byte shall have "
                    "the gciPresentFlag bit (first bit) set to 1.");
    ILO_ASSERT_WITH(ptl.nativePtl.generalConstraintInfo.size() >= 10, std::invalid_argument,
                    "A generalConstraintInfo info buffer must either 1 byte in size in case "
                    "gciPresentFlag bit (first bit) "
                    "is set to 0 or bigger than 9 bytes.");
  }

  m_ptlPresentFlag = true;
  m_ptl = ptl;
}

void CVvcDecoderConfigRecord::setNonVclArrays(const NonVclArrays& nonVclArraysValue) {
  ILO_ASSERT(nonVclArraysValue.size() <= std::numeric_limits<uint8_t>::max(),
             "NonVclArrays count exceeded expected size (8 bits)");

  for (const auto& nonVclArray : nonVclArraysValue) {
    ILO_ASSERT_WITH(std::find(ALLOWED_VVC_NALU_TYPES.begin(), ALLOWED_VVC_NALU_TYPES.end(),
                              nonVclArray.naluType) != ALLOWED_VVC_NALU_TYPES.end(),
                    std::invalid_argument, "Nalu type %d is not allowed in vvc config record.",
                    nonVclArray.naluType);

    // 12 == OPI_NUT, 13 == DCI_NUT
    if (nonVclArray.naluType == 13 || nonVclArray.naluType == 12) {
      ILO_ASSERT_WITH(nonVclArray.nalus.size() == 1, std::invalid_argument,
                      "There can only be 1 nalu of type %d in this array, but %d were found.",
                      nonVclArray.naluType, nonVclArray.nalus.size());
    }
  }

  m_nonVclArrays = nonVclArraysValue;
}

void CVvcDecoderConfigRecord::write(ByteBuffer& buffer, ByteBuffer::iterator& position) {
  ILO_ASSERT_WITH(m_lengthSizeMinusOne != 255U, std::invalid_argument,
                  "lengthSizeMinusOne must be set before calling write.");

  ILO_ASSERT(buffer.size() <= std::numeric_limits<uint32_t>::max(),
             "Bitwriter cannot handle buffers that big.");
  ilo::CBitBuffer bitWriter{buffer, static_cast<uint32_t>(buffer.size() * 8)};

  // Align with start position
  bitWriter.seek(static_cast<int32_t>(position - buffer.begin()) * 8, ilo::EPosType::begin);

  // Write reserved '11111'b
  bitWriter.write(uint8_t{0x1F}, 5);
  bitWriter.write(m_lengthSizeMinusOne, 2);
  bitWriter.write(m_ptlPresentFlag);

  if (m_ptlPresentFlag) {
    writePtl(bitWriter);
  }

  bitWriter.write(m_nonVclArrays.size(), 8);

  for (const auto& nonVclArray : m_nonVclArrays) {
    bitWriter.write(nonVclArray.arrayCompleteness);
    // Write reserved '00'b
    bitWriter.write(uint8_t{0}, 2);
    bitWriter.write(nonVclArray.naluType, 5);
    // 12 == OPI_NUT, 13 == DCI_NUT
    if (nonVclArray.naluType != 13 && nonVclArray.naluType != 12) {
      bitWriter.write(nonVclArray.nalus.size(), 16);
    }

    for (const auto& nalu : nonVclArray.nalus) {
      bitWriter.write(nalu.size(), 16);
      uint32_t pos = bitWriter.tell();

      // Set the position iterator to the current position
      position = buffer.begin() + pos / 8;

      ILO_ASSERT(static_cast<size_t>(buffer.end() - position) >= nalu.size(),
                 "Nalu data does not fit in buffer.");

      std::copy(nalu.begin(), nalu.end(), position);
      position += nalu.size();

      // Update bitwriter writer pointer position
      bitWriter.seek(static_cast<int32_t>(nalu.size() * 8), ilo::EPosType::cur);
    }
  }
}

uint64_t CVvcDecoderConfigRecord::size() const {
  // reserved(5), LengthSizeMinusOne(2), ptlPresentFlag(1)
  uint64_t recSize = 1;
  if (m_ptlPresentFlag) {
    // olsIdx(9) + numSubLayers(3), constantFrameRate(2), chrmeFormatIfc(2),
    // bitDepthMinus8(3), reserved (5)
    recSize += 1 + 1 + 1;

    // -- Start of VvcPtlrecord  --
    // reserved(2), numBytesConstraintInfo(6), generelProfileIdc(7),
    // generalTierFlag(1), generalLevelIdc(8),
    recSize += 1 + 1 + 1;

    // Calculate size needed for generalConstraintInfo according to ISO/IEC 14496-15 - 11.2.4.1.3
    // Cannot be calculated statically, but depends on actual data and bit-layout.
    ilo::ByteBuffer::const_iterator constraintBuffBeg =
        m_ptl.nativePtl.generalConstraintInfo.begin();
    const ilo::ByteBuffer::const_iterator constraintBuffEnd =
        m_ptl.nativePtl.generalConstraintInfo.end();
    uint32_t validGeneralConstraintInfoBits =
        numValidBitsConstraintInfo(constraintBuffBeg, constraintBuffEnd);
    uint32_t generalConstraintInfoBytes =
        generalConstraintInfoSizeInBytes(validGeneralConstraintInfoBits);

    // ptlFrameOnlyConstraintFlag(1), ptlMultilayerEnabledFlag(1), 8*numBytesConstraintInfo-2
    recSize += generalConstraintInfoBytes;

    if (m_ptl.numSublayers >= 2) {
      // n*ptlSublayerLevelPresentFlag(1), n*ptlReservedZeroBit(1)
      // with max total amount of 1 Byte combined
      recSize += 1;
    }

    if (!m_ptl.nativePtl.sublayerLevelIdcs.empty()) {
      recSize += m_ptl.nativePtl.sublayerLevelIdcs.size();
    }

    // ptlNumSubProfiles(8), n*generalsubProfileidc32)
    recSize += 1 + m_ptl.nativePtl.generalSubProfileIdcs.size() * 4;
    // -- End of VvcPtlrecord  --

    // maxPictureWidth(16), maxPictureHeight(16), avgFrameRate(16),
    recSize += 2 + 2 + 2;
  }
  // numOfArrays(8)
  recSize += 1;

  for (const auto& naluArray : m_nonVclArrays) {
    // arrayCompleteness(1), reserved(2), nalUnitType(5)
    recSize += 1;

    if (naluArray.naluType != 13 && naluArray.naluType != 12) {
      // numNalus(16)
      recSize += 2;
    }

    for (const auto& nalu : naluArray.nalus) {
      // nalUnitLength(16), nalUnit(8*n)
      recSize += 2 + nalu.size();
    }
  }
  return recSize;
}

SAttributeList CVvcDecoderConfigRecord::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Length Size Minus One";
  attribute.value = std::to_string(m_lengthSizeMinusOne);
  attributesList.push_back(attribute);

  attribute.key = "Ptl Present Flag";
  attribute.value = std::to_string(m_ptlPresentFlag);
  attributesList.push_back(attribute);

  if (m_ptlPresentFlag) {
    attribute.key = "Ols Idx";
    attribute.value = std::to_string(m_ptl.olsIdx);
    attributesList.push_back(attribute);

    attribute.key = "Num Sublayers";
    attribute.value = std::to_string(m_ptl.numSublayers);
    attributesList.push_back(attribute);

    attribute.key = "Constant Frame Rate";
    attribute.value = std::to_string(m_ptl.constantFrameRate);
    attributesList.push_back(attribute);

    attribute.key = "Chroma Format Idc";
    attribute.value = std::to_string(m_ptl.chromaFormatIdc);
    attributesList.push_back(attribute);

    attribute.key = "Bit Depth Minus8";
    attribute.value = std::to_string(m_ptl.bitDepthMinus8);
    attributesList.push_back(attribute);

    attribute.key = "General Profile Idc";
    attribute.value = std::to_string(m_ptl.nativePtl.generalProfileIdc);
    attributesList.push_back(attribute);

    attribute.key = "General Tier Flag";
    attribute.value = std::to_string(m_ptl.nativePtl.generalTierFlag);
    attributesList.push_back(attribute);

    attribute.key = "General Level Idc";
    attribute.value = std::to_string(m_ptl.nativePtl.generalLevelIdc);
    attributesList.push_back(attribute);

    attribute.key = "Ptl Frame Only Constraint Flag";
    attribute.value = std::to_string(m_ptl.nativePtl.ptlFrameOnlyConstraintFlag);
    attributesList.push_back(attribute);

    attribute.key = "Ptl Multi Layer Enabled Flag";
    attribute.value = std::to_string(m_ptl.nativePtl.ptlMultiLayerEnabledFlag);
    attributesList.push_back(attribute);

    attribute.key = "General Constraint Info";
    std::stringstream ss;
    for (uint8_t value : m_ptl.nativePtl.generalConstraintInfo) {
      ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
         << static_cast<int>(value) << ' ';
    }
    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);

    attribute.key = "Sublayer Level Idcs";
    if (m_ptl.nativePtl.sublayerLevelIdcs.empty()) {
      attribute.value = "None";
    } else {
      std::stringstream valueStr;
      for (const auto& ptlSubglayerFlag : m_ptl.nativePtl.sublayerLevelIdcs) {
        valueStr << "Layer: " << std::to_string(ptlSubglayerFlag.first)
                 << ", Value: " << std::to_string(ptlSubglayerFlag.second) << "; ";
      }
      attribute.value = valueStr.str();
      attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
    }
    attributesList.push_back(attribute);

    attribute.key = "General Sub Profile Idcs";
    if (m_ptl.nativePtl.sublayerLevelIdcs.empty()) {
      attribute.value = "None";
    } else {
      std::stringstream valueStr;
      for (auto generalSubProfileIdc : m_ptl.nativePtl.generalSubProfileIdcs) {
        valueStr << std::to_string(generalSubProfileIdc) << ", ";
      }
      attribute.value = valueStr.str();
      attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
    }
    attributesList.push_back(attribute);

    attribute.key = "Max Picture Width";
    attribute.value = std::to_string(m_ptl.maxPictureWidth);
    attributesList.push_back(attribute);

    attribute.key = "Max Picture Height";
    attribute.value = std::to_string(m_ptl.maxPictureHeight);
    attributesList.push_back(attribute);

    attribute.key = "Avg Frame Rate";
    attribute.value = std::to_string(m_ptl.avgFrameRate);
    attributesList.push_back(attribute);
  }

  attribute.key = "Number of Non Vcl Arrays";
  attribute.value = std::to_string(m_nonVclArrays.size());
  attributesList.push_back(attribute);

  if (!m_nonVclArrays.empty()) {
    attribute.key = "Non Vcl Arrays";
    std::stringstream vcl;
    for (auto& nonVclArray : m_nonVclArrays) {
      vcl << "Array Completeness: " << (nonVclArray.arrayCompleteness ? "True" : "False")
          << ", Nalu Type: "
          << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase
          << static_cast<int>(nonVclArray.naluType) << ", Nalus: ";

      uint32_t index = 0;
      for (auto& nalu : nonVclArray.nalus) {
        vcl << '[';
        for (size_t i = 0; i < nalu.size(); ++i) {
          vcl << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
              << static_cast<int>(nalu[i]);

          if (i != nalu.size() - 1) {
            vcl << ' ';
          } else {
            vcl << ']';
          }
        }
        index++;
        if (index != nonVclArray.nalus.size()) {
          vcl << ", ";
        }
      }
      vcl << "; ";
    }
    attribute.value = vcl.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
    attributesList.push_back(attribute);
  }
  return attributesList;
}

void CVvcDecoderConfigRecord::parse(ilo::ByteBuffer::const_iterator& begin,
                                    const ilo::ByteBuffer::const_iterator& end) {
  ilo::ByteBuffer::difference_type totalDcrSize = end - begin;
  ILO_ASSERT(static_cast<uint64_t>(totalDcrSize) <= std::numeric_limits<uint32_t>::max() / 8U,
             "VVC Decoder Config Record size of %d bytes exceeds bitparser capabilities.",
             totalDcrSize);

  ilo::CBitParser bitParser{begin, end};
  ILO_ASSERT_WITH(bitParser.nofBitsLeft() == static_cast<uint32_t>(totalDcrSize) * 8U,
                  std::logic_error, "Failed to create bitparser from buffer.");

  // Read and check reserved bits
  if (bitParser.read<uint8_t>(5) != 0x1F) {
    ILO_LOG_WARNING("Reserved field1 is not '11111'b in vvc decoder config record.");
  }

  m_lengthSizeMinusOne = bitParser.read<uint8_t>(2);
  if (m_lengthSizeMinusOne != 0 && m_lengthSizeMinusOne != 1 && m_lengthSizeMinusOne != 3) {
    ILO_LOG_WARNING(
        "Invalid lengthSizeMinusOneValue of %d in vvc decoder config record. Allowed values are 0, "
        "1 and 3.",
        m_lengthSizeMinusOne);
  }

  m_ptlPresentFlag = bitParser.read<uint8_t>(1) > 0;
  if (m_ptlPresentFlag) {
    parsePtl(bitParser);
  }

  uint8_t numOfArrays = bitParser.read<uint8_t>(8);
  for (uint8_t i = 0; i < numOfArrays; ++i) {
    SVvcArray array;
    array.arrayCompleteness = bitParser.read<uint8_t>(1) > 0;
    // Read and check reserved bits
    if (bitParser.read<uint8_t>(2) != 0) {
      ILO_LOG_WARNING("Reserved field5 is not '0'b in vvc decoder config record.");
    }
    array.naluType = bitParser.read<uint8_t>(5);
    if (std::find(ALLOWED_VVC_NALU_TYPES.begin(), ALLOWED_VVC_NALU_TYPES.end(), array.naluType) ==
        ALLOWED_VVC_NALU_TYPES.end()) {
      ILO_LOG_WARNING(
          "Potentially forbidden non-VCL NALU type of %d found in VVC decoder config record.",
          array.naluType);
    }

    // According to ISO/IEC 14496-15 - 11.2.4.2.3 numValues
    // is inferred to be equal to 1 if the field is not present
    uint16_t numNalus = 1;
    // 12 == OPI_NUT, 13 == DCI_NUT
    if (array.naluType != 13 && array.naluType != 12) {
      numNalus = bitParser.read<uint16_t>(16);
    }

    for (uint16_t j = 0; j < numNalus; ++j) {
      uint16_t nalUnitLength = bitParser.read<uint16_t>(16);
      ILO_ASSERT_WITH(bitParser.nofReadBits() % 8 == 0, std::logic_error,
                      "Bitreader is not byte-aligned when copying non-VCL nalus.");
      ilo::ByteBuffer::const_iterator naluBegin = begin + bitParser.nofReadBits() / 8;
      ilo::ByteBuffer::const_iterator naluEnd = begin + bitParser.nofReadBits() / 8 + nalUnitLength;
      array.nalus.emplace_back(naluBegin, naluEnd);
      ILO_ASSERT(array.nalus.back().size() == nalUnitLength, "Failed to read nalu.");
      bitParser.seek(nalUnitLength * 8, ilo::EPosType::cur);
    }

    m_nonVclArrays.emplace_back(array);
  }

  // Sanity checks if we have read all the data
  ILO_ASSERT(bitParser.nofBitsLeft() == 0, "Still bits left after parsing finished.");

  // Update the iterator
  begin = end;
}

void CVvcDecoderConfigRecord::parsePtl(ilo::CBitParser& bitParser) {
  m_ptl.olsIdx = bitParser.read<uint16_t>(9);
  m_ptl.numSublayers = bitParser.read<uint8_t>(3);
  m_ptl.constantFrameRate = bitParser.read<uint8_t>(2);
  m_ptl.chromaFormatIdc = bitParser.read<uint8_t>(2);
  m_ptl.bitDepthMinus8 = bitParser.read<uint8_t>(3);
  // Read and check reserved bits
  if (bitParser.read<uint8_t>(5) != 0x31) {
    ILO_LOG_WARNING("Reserved field2 is not '11111'b in vvc decoder config record.");
  }

  parsePtlRecord(bitParser);

  m_ptl.maxPictureWidth = bitParser.read<uint16_t>(16);
  m_ptl.maxPictureHeight = bitParser.read<uint16_t>(16);
  m_ptl.avgFrameRate = bitParser.read<uint16_t>(16);
}

void CVvcDecoderConfigRecord::parsePtlRecord(ilo::CBitParser& bitParser) {
  // Read and check reserved bits
  if (bitParser.read<uint8_t>(2) != 0) {
    ILO_LOG_WARNING("Reserved field3 is not '0'b in vvc decoder config record.");
  }

  uint8_t numBytesConstraintInfo = bitParser.read<uint8_t>(6);
  ILO_ASSERT_WITH(
      numBytesConstraintInfo > 0, std::logic_error,
      "numBytesConstraintInfo shall not be 0 according to the standard. Parsing cannot continue, "
      "because of byte alignment issues. The bistream might be defective.");
  m_ptl.nativePtl.generalProfileIdc = bitParser.read<uint8_t>(7);
  m_ptl.nativePtl.generalTierFlag = bitParser.read<uint8_t>(1) > 0;
  m_ptl.nativePtl.generalLevelIdc = bitParser.read<uint8_t>(8);
  m_ptl.nativePtl.ptlFrameOnlyConstraintFlag = bitParser.read<uint8_t>(1) > 0;
  m_ptl.nativePtl.ptlMultiLayerEnabledFlag = bitParser.read<uint8_t>(1) > 0;

  parseConstraintInfo(bitParser, numBytesConstraintInfo);

  // Mapping of sublayer index to ptlSublayerLevelPresentFlag.
  // Order of ptlSublayerLevelPresentFlags matters!
  std::map<uint8_t, bool> ptlSublayerLevelPresentFlags;

  if (m_ptl.numSublayers >= 2) {
    for (int8_t i = m_ptl.numSublayers - 2; i >= 0; --i) {
      ptlSublayerLevelPresentFlags[i] = bitParser.read<uint8_t>(1) > 0;
    }
  }

  for (uint8_t i = m_ptl.numSublayers; i <= 8 && m_ptl.numSublayers > 1; ++i) {
    // Read and check reserved bits
    if (bitParser.read<uint8_t>(1) != 0) {
      ILO_LOG_WARNING("Reserved field4 is not '0'b in vvc decoder config record.");
    }
  }

  if (m_ptl.numSublayers >= 2) {
    for (int8_t i = m_ptl.numSublayers - 2; i >= 0; --i) {
      if (ptlSublayerLevelPresentFlags.at(i)) {
        m_ptl.nativePtl.sublayerLevelIdcs[i] = bitParser.read<uint8_t>(8);
      }
    }
  }

  uint8_t ptlNumSubProfiles = bitParser.read<uint8_t>(8);
  for (uint8_t i = 0; i < ptlNumSubProfiles; ++i) {
    m_ptl.nativePtl.generalSubProfileIdcs.emplace_back(bitParser.read<uint32_t>(32));
  }
}

void CVvcDecoderConfigRecord::parseConstraintInfo(ilo::CBitParser& bitParser,
                                                  uint8_t numBytesConstraintInfo) {
  // Read numBytesConstraintInfo bytes and right shift everything by 2 bits
  ilo::ByteBuffer tmpConstraintBuffer(numBytesConstraintInfo);
  ilo::CBitBuffer tmpBitWriter(tmpConstraintBuffer, numBytesConstraintInfo * 8U);
  for (uint32_t i = 2; i < numBytesConstraintInfo * 8U;) {
    if (i == 2) {
      tmpBitWriter.write(bitParser.read<uint8_t>(6), 6);
      i += 6;
    } else {
      tmpBitWriter.write(bitParser.read<uint8_t>(8), 8);
      i += 8;
    }
  }

  // Parse tmpConstraintBuffer to get valid data bits and re-align
  ilo::ByteBuffer::const_iterator constraintBuffBeg = tmpConstraintBuffer.begin();
  const ilo::ByteBuffer::const_iterator constraintBuffEnd = tmpConstraintBuffer.end();
  uint32_t nrOfValidBits = numValidBitsConstraintInfo(constraintBuffBeg, constraintBuffEnd);

  // Calculate how much data to read including the minimum padding at the end
  size_t bytesToRead = static_cast<size_t>(std::ceil(static_cast<float>(nrOfValidBits) / 8));

  ilo::CBitParser tmpBitReader(constraintBuffBeg, constraintBuffEnd);

  for (size_t i = 0; i < bytesToRead; ++i) {
    m_ptl.nativePtl.generalConstraintInfo.emplace_back(tmpBitReader.read<uint8_t>(8));
  }
}

void CVvcDecoderConfigRecord::writePtl(ilo::CBitBuffer& bitWriter) {
  bitWriter.write(m_ptl.olsIdx, 9);
  bitWriter.write(m_ptl.numSublayers, 3);
  bitWriter.write(m_ptl.constantFrameRate, 2);
  bitWriter.write(m_ptl.chromaFormatIdc, 2);
  bitWriter.write(m_ptl.bitDepthMinus8, 3);
  // Write reserved '11111'b
  bitWriter.write(uint8_t{0x1F}, 5);

  writePtlRecord(bitWriter);

  bitWriter.write(m_ptl.maxPictureWidth, 16);
  bitWriter.write(m_ptl.maxPictureHeight, 16);
  bitWriter.write(m_ptl.avgFrameRate, 16);
}

void CVvcDecoderConfigRecord::writePtlRecord(ilo::CBitBuffer& bitWriter) {
  // Calculate size needed for generalConstraintInfo according to ISO/IEC 14496-15 - 11.2.4.1.3
  ilo::ByteBuffer::const_iterator constraintBuffBeg = m_ptl.nativePtl.generalConstraintInfo.begin();
  const ilo::ByteBuffer::const_iterator constraintBuffEnd =
      m_ptl.nativePtl.generalConstraintInfo.end();
  uint32_t validGeneralConstraintInfoBits =
      numValidBitsConstraintInfo(constraintBuffBeg, constraintBuffEnd);
  uint32_t generalConstraintInfoBytes =
      generalConstraintInfoSizeInBytes(validGeneralConstraintInfoBits);

  // Write reserved '00'b
  bitWriter.write(uint8_t{0}, 2);
  bitWriter.write(generalConstraintInfoBytes, 6);
  bitWriter.write(m_ptl.nativePtl.generalProfileIdc, 7);
  bitWriter.write(m_ptl.nativePtl.generalTierFlag);
  bitWriter.write(m_ptl.nativePtl.generalLevelIdc, 8);
  bitWriter.write(m_ptl.nativePtl.ptlFrameOnlyConstraintFlag);
  bitWriter.write(m_ptl.nativePtl.ptlMultiLayerEnabledFlag);

  writeConstraintInfo(bitWriter, validGeneralConstraintInfoBits);

  // Note: do not use uint8_t here to avoid overflow.
  for (int8_t i = static_cast<int8_t>(m_ptl.numSublayers) - 2; i >= 0; --i) {
    bool ptlSubLayerlevelPresentFlag =
        (m_ptl.nativePtl.sublayerLevelIdcs.find(i) != m_ptl.nativePtl.sublayerLevelIdcs.end());
    bitWriter.write(ptlSubLayerlevelPresentFlag);
  }

  for (uint8_t i = m_ptl.numSublayers; i <= 8 && m_ptl.numSublayers > 1; ++i) {
    // Write reserved '0'b
    bitWriter.write(uint8_t{0}, 1);
  }

  // Note: do not use uint8_t here to avoid overflow.
  for (int8_t i = static_cast<int8_t>(m_ptl.numSublayers) - 2; i >= 0; --i) {
    auto iter = m_ptl.nativePtl.sublayerLevelIdcs.find(i);
    if (iter != m_ptl.nativePtl.sublayerLevelIdcs.end()) {
      bitWriter.write(iter->second, 8);
    }
  }

  bitWriter.write(m_ptl.nativePtl.generalSubProfileIdcs.size(), 8);

  for (uint32_t value : m_ptl.nativePtl.generalSubProfileIdcs) {
    bitWriter.write(value, 32);
  }
}

uint32_t CVvcDecoderConfigRecord::numValidBitsConstraintInfo(
    ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end) const {
  ilo::CBitParser bitParser(begin, end);

  if (bitParser.read<uint8_t>(1) == 0) {
    return bitParser.nofReadBits();
  }

  // Skip all payload bits until reserved bit loop
  bitParser.seek(71, ilo::EPosType::cur);

  uint8_t reserved = bitParser.read<uint8_t>(8);

  // Skip all reserved bits
  bitParser.seek(reserved, ilo::EPosType::cur);

  return bitParser.nofReadBits();
}

void CVvcDecoderConfigRecord::writeConstraintInfo(ilo::CBitBuffer& bitWriter,
                                                  uint32_t nrOfValidBits) const {
  // Calculate how many full bytes we can write
  uint32_t bytesToWrite = nrOfValidBits / 8;
  // Calculate how many left over bits from the last not fully used byte needs to be written
  uint32_t lefOverBits = nrOfValidBits - bytesToWrite * 8;

  for (uint32_t i = 0; i < bytesToWrite; ++i) {
    bitWriter.write(m_ptl.nativePtl.generalConstraintInfo.at(i), 8);
  }

  if (lefOverBits) {
    bitWriter.write(
        static_cast<uint8_t>(m_ptl.nativePtl.generalConstraintInfo.back() >> (8U - lefOverBits)),
        lefOverBits);
  }

  // Since we might have changed the original byte-alignment of the general_constraints_info()
  // structure from ISO/IEC 23090-3 - 7.3.3.2, make sure to byte-align here
  bitWriter.byteAlign();
}

uint32_t CVvcDecoderConfigRecord::generalConstraintInfoSizeInBytes(
    uint32_t validGeneralConstraintInfoBits) const {
  uint32_t generalConstraintInfoBytes =
      static_cast<uint32_t>(std::ceil(static_cast<float>(validGeneralConstraintInfoBits) / 8));

  // Accoring to ISO/IEC 14496-15 - 11.2.4.1.3, the maximum allow payload to write is
  // 8*num_bytes_constraint_info - 2 In case validGeneralConstraintInfoBits rounded to the next
  // bigger byte border does not fit, we have to add a complete extra byte.
  if (generalConstraintInfoBytes * 8 < validGeneralConstraintInfoBits + 2) {
    generalConstraintInfoBytes++;
  }

  return generalConstraintInfoBytes;
}
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
