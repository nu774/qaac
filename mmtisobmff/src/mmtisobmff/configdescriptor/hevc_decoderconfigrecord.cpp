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
 * Content: hevc decoder configuration record
 */

// System includes
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <numeric>

// External includes
#include "common/logging.h"
#include "ilo/bytebuffertools.h"

// Internal includes
#include "mmtisobmff/configdescriptor/hevc_decoderconfigrecord.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace config {
CHevcDecoderConfigRecord::CHevcDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                                                   const ilo::ByteBuffer::const_iterator& end)
    : m_configurationVersion(1),
      m_generalProfileSpace(0),
      m_generalTierFlag(0),
      m_generalProfileIdc(0),
      m_generalProfileCompatabilityFlags(0),
      m_generalConstraintIndicatorFlags(0),
      m_generalLevelIdc(0),
      m_minSpatialSegmentationIdc(0),
      m_paralelismType(0),
      m_chromaFormatIdc(0),
      m_bitDepthLumaMinus8(0),
      m_bitDepthChromaMinus8(0),
      m_avgFrameRate(0),
      m_constFrameRate(0),
      m_numTemporatlLayers(0),
      m_temporalIdNested(0),
      m_lengthSizeMinusOne(0) {
  parse(begin, end);
}

CHevcDecoderConfigRecord::CHevcDecoderConfigRecord()
    : m_configurationVersion(1),
      m_generalProfileSpace(0),
      m_generalTierFlag(0),
      m_generalProfileIdc(0),
      m_generalProfileCompatabilityFlags(0),
      m_generalConstraintIndicatorFlags(0),
      m_generalLevelIdc(0),
      m_minSpatialSegmentationIdc(0),
      m_paralelismType(0),
      m_chromaFormatIdc(0),
      m_bitDepthLumaMinus8(0),
      m_bitDepthChromaMinus8(0),
      m_avgFrameRate(0),
      m_constFrameRate(0),
      m_numTemporatlLayers(0),
      m_temporalIdNested(0),
      m_lengthSizeMinusOne(0) {}

void CHevcDecoderConfigRecord::setConfigurationVersion(uint8_t configVersion) {
  m_configurationVersion = configVersion;
}

void CHevcDecoderConfigRecord::setGeneralProfileSpace(uint8_t profileSpace) {
  m_generalProfileSpace = profileSpace;
}

void CHevcDecoderConfigRecord::setGeneralTierFlag(bool tierFlag) {
  m_generalTierFlag = tierFlag;
}

void CHevcDecoderConfigRecord::setGeneralProfileIdc(uint8_t profileIdc) {
  m_generalProfileIdc = profileIdc;
}

void CHevcDecoderConfigRecord::setGeneralProfileCompatabilityFlags(uint32_t profileCompatFlags) {
  m_generalProfileCompatabilityFlags = profileCompatFlags;
}

void CHevcDecoderConfigRecord::setGeneralConstraintIndicatorFlags(uint64_t constraintIndFlags) {
  m_generalConstraintIndicatorFlags = constraintIndFlags;
}

void CHevcDecoderConfigRecord::setGeneralLevelIdc(uint8_t levelIdc) {
  m_generalLevelIdc = levelIdc;
}

void CHevcDecoderConfigRecord::setMinSpatialSegmentationIdc(uint16_t minSegmIdc) {
  m_minSpatialSegmentationIdc = minSegmIdc;
}

void CHevcDecoderConfigRecord::setParalelismType(uint8_t paralelType) {
  m_paralelismType = paralelType;
}

void CHevcDecoderConfigRecord::setChromaFormatIdc(uint8_t chromaFormatIdcValue) {
  m_chromaFormatIdc = chromaFormatIdcValue;
}

void CHevcDecoderConfigRecord::setBitDepthLumaMinus8(uint8_t bitDepthLumaMinus8Value) {
  m_bitDepthLumaMinus8 = bitDepthLumaMinus8Value;
}

void CHevcDecoderConfigRecord::setBitDepthChromaMinus8(uint8_t bitDepthChromaMinus8Value) {
  m_bitDepthChromaMinus8 = bitDepthChromaMinus8Value;
}

void CHevcDecoderConfigRecord::setAvgFrameRate(uint16_t avgFramerate) {
  m_avgFrameRate = avgFramerate;
}

void CHevcDecoderConfigRecord::setConstFrameRate(uint8_t constFramerate) {
  m_constFrameRate = constFramerate;
}

void CHevcDecoderConfigRecord::setNumTemporatlLayers(uint8_t numTempLayer) {
  m_numTemporatlLayers = numTempLayer;
}

void CHevcDecoderConfigRecord::setTemporalIdNested(bool tempIdNested) {
  m_temporalIdNested = tempIdNested;
}

void CHevcDecoderConfigRecord::setLengthSizeMinusOne(uint8_t lengthSizeMinusOneValue) {
  m_lengthSizeMinusOne = lengthSizeMinusOneValue;
}
void CHevcDecoderConfigRecord::setNonVclArrays(const NonVclArrays& nonVclArraysValue) {
  ILO_ASSERT(nonVclArraysValue.size() <= std::numeric_limits<uint8_t>::max(),
             "NonVclArrays count exceeded expected size (8 bits)");

  m_nonVclArrays = nonVclArraysValue;
}

void CHevcDecoderConfigRecord::parse(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end) {
  m_configurationVersion = readUint8(begin, end);

  ILO_ASSERT_WITH(
      m_configurationVersion == 1, std::invalid_argument,
      "HEVC Decoder Configuration Record: Configuration version of %d is not supported!",
      m_configurationVersion);

  uint8_t genProfSpaceFlagIdc = readUint8(begin, end);
  m_generalProfileSpace = (genProfSpaceFlagIdc & 0xC0) >> 6;
  m_generalTierFlag = (genProfSpaceFlagIdc & 0x20) != 0;
  m_generalProfileIdc = (genProfSpaceFlagIdc & 0x1F);

  m_generalProfileCompatabilityFlags = readUint32(begin, end);

  uint32_t constrIndFlags32 = readUint32(begin, end);
  uint16_t constrIndFlags16 = readUint16(begin, end);
  uint64_t constrIndFlags64 = 0;

  constrIndFlags64 |= constrIndFlags32;
  constrIndFlags64 = constrIndFlags64 << 16;
  constrIndFlags64 |= constrIndFlags16;

  m_generalConstraintIndicatorFlags = constrIndFlags64;
  m_generalLevelIdc = readUint8(begin, end);

  uint16_t reservedMinSpatialSegmentationIdc = readUint16(begin, end);
  uint8_t reservedCheck = (uint8_t)(reservedMinSpatialSegmentationIdc >> 12);

  if ((reservedCheck ^ 0x0F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '1111'");
  }

  m_minSpatialSegmentationIdc = (reservedMinSpatialSegmentationIdc & 0x0FFF);

  uint8_t paralelType = readUint8(begin, end);
  reservedCheck = (paralelType >> 2);

  if ((reservedCheck ^ 0x3F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '111111'");
  }

  m_paralelismType = (paralelType & 0x03);

  uint8_t chromaFormat = readUint8(begin, end);
  reservedCheck = (chromaFormat >> 2);

  if ((reservedCheck ^ 0x3F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '111111'");
  }

  m_chromaFormatIdc = (chromaFormat & 0x03);

  uint8_t bitDepthLumaMinus8Value = readUint8(begin, end);
  reservedCheck = (bitDepthLumaMinus8Value >> 3);

  if ((reservedCheck ^ 0x1F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '11111'");
  }

  m_bitDepthLumaMinus8 = (bitDepthLumaMinus8Value & 0x07);

  uint8_t bitDepthChromaMinus8Value = readUint8(begin, end);
  reservedCheck = (bitDepthChromaMinus8Value >> 3);

  if ((reservedCheck ^ 0x1F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '11111'");
  }

  m_bitDepthChromaMinus8 = (bitDepthChromaMinus8Value & 0x07);
  m_avgFrameRate = readUint16(begin, end);

  uint8_t tmp = readUint8(begin, end);
  m_constFrameRate = (tmp & 0xC0) >> 6;
  m_numTemporatlLayers = (tmp & 0x38) >> 3;
  m_temporalIdNested = (tmp & 0x04) != 0;
  m_lengthSizeMinusOne = (tmp & 0x03);

  uint8_t numOfArrays = readUint8(begin, end);

  for (uint8_t i = 0; i < numOfArrays; ++i) {
    SHevcArray hevcArray;

    uint8_t arrayCompletenessReservedNaluType = readUint8(begin, end);

    hevcArray.arrayCompleteness = (arrayCompletenessReservedNaluType & 0x80) != 0;

    if ((arrayCompletenessReservedNaluType & 0x40) != 0) {
      ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '11111'");
    }

    hevcArray.naluType = (arrayCompletenessReservedNaluType & 0x3F);

    if (std::find(ALLOWED_HEVC_NALU_TYPES.begin(), ALLOWED_HEVC_NALU_TYPES.end(),
                  hevcArray.naluType) == ALLOWED_HEVC_NALU_TYPES.end()) {
      ILO_LOG_WARNING("Potential unallowed non-VCL nalu type of %u found.", hevcArray.naluType);
    }

    uint16_t numNalus = readUint16(begin, end);

    for (uint32_t j = 0; j < numNalus; j++) {
      uint16_t naluLength = readUint16(begin, end);

      hevcArray.nalus.push_back(ByteBuffer(begin, begin + naluLength));

      begin += naluLength;
    }
    m_nonVclArrays.push_back(hevcArray);
  }
}

void CHevcDecoderConfigRecord::write(ByteBuffer& buffer, ByteBuffer::iterator& position) {
  writeUint8(buffer, position, m_configurationVersion);

  uint8_t genProfSpaceFlagIdc = 0;
  genProfSpaceFlagIdc |= (m_generalProfileSpace << 6);
  m_generalTierFlag ? genProfSpaceFlagIdc |= 0x20 : 0;
  genProfSpaceFlagIdc |= m_generalProfileIdc;
  writeUint8(buffer, position, genProfSpaceFlagIdc);

  writeUint32(buffer, position, m_generalProfileCompatabilityFlags);

  uint32_t conIndFlagsLevelIdc32 = 0;
  uint16_t conIndFlagsLevelIdc16 = 0;
  conIndFlagsLevelIdc32 |= (m_generalConstraintIndicatorFlags >> 16);
  writeUint32(buffer, position, conIndFlagsLevelIdc32);

  conIndFlagsLevelIdc16 |= (m_generalConstraintIndicatorFlags);
  writeUint16(buffer, position, conIndFlagsLevelIdc16);

  writeUint8(buffer, position, m_generalLevelIdc);

  uint16_t reservedMinSpatialSegmentationIdc = 0xF000;
  reservedMinSpatialSegmentationIdc |= m_minSpatialSegmentationIdc;
  uint8_t reservedCheck = (uint8_t)(reservedMinSpatialSegmentationIdc >> 12);

  if ((reservedCheck ^ 0x0F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '1111'");
  }

  writeUint16(buffer, position, reservedMinSpatialSegmentationIdc);

  uint8_t paralelType = 0xFC;
  paralelType |= m_paralelismType;
  reservedCheck = (paralelType >> 2);

  if ((reservedCheck ^ 0x3F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '111111'");
  }

  writeUint8(buffer, position, paralelType);

  uint8_t chromaFormat = 0xFC;
  chromaFormat |= m_chromaFormatIdc;
  reservedCheck = (chromaFormat >> 2);

  if ((reservedCheck ^ 0x3F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '111111'");
  }

  writeUint8(buffer, position, chromaFormat);

  uint8_t bitDepthLumaMinus8Value = 0xF8;
  bitDepthLumaMinus8Value |= m_bitDepthLumaMinus8;
  reservedCheck = (bitDepthLumaMinus8Value >> 3);

  if ((reservedCheck ^ 0x1F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '11111'");
  }

  writeUint8(buffer, position, bitDepthLumaMinus8Value);

  uint8_t bitDepthChromaMinus8Value = 0xF8;
  bitDepthChromaMinus8Value |= m_bitDepthChromaMinus8;
  reservedCheck = (bitDepthChromaMinus8Value >> 3);

  if ((reservedCheck ^ 0x1F) != 0) {
    ILO_LOG_WARNING("HEVC Decoder Configuration Record: reserved must be set to '11111'");
  }

  writeUint8(buffer, position, bitDepthChromaMinus8Value);
  writeUint16(buffer, position, m_avgFrameRate);

  uint8_t tmp = 0;
  tmp |= (m_constFrameRate << 6);
  tmp |= (m_numTemporatlLayers << 3);
  m_temporalIdNested ? tmp |= 0x04 : 0;
  tmp |= m_lengthSizeMinusOne;

  writeUint8(buffer, position, tmp);
  writeUint8(buffer, position, static_cast<uint8_t>(m_nonVclArrays.size()));

  for (auto hevcArray : m_nonVclArrays) {
    uint8_t arrayCompletenessReservedNaluType = 0;
    hevcArray.arrayCompleteness ? arrayCompletenessReservedNaluType |= 0x80 : 0;
    arrayCompletenessReservedNaluType |= hevcArray.naluType;
    writeUint8(buffer, position, arrayCompletenessReservedNaluType);

    writeUint16(buffer, position, static_cast<uint16_t>(hevcArray.nalus.size()));

    for (auto nalu : hevcArray.nalus) {
      writeUint16(buffer, position, static_cast<uint16_t>(nalu.size()));

      ILO_ASSERT(static_cast<size_t>(buffer.end() - position) >= nalu.size(),
                 "HEVC Decoder Configuration Record: Nalu data does not fit in buffer!");

      std::copy(nalu.begin(), nalu.end(), position);

      position += nalu.size();
    }
  }
}

uint64_t CHevcDecoderConfigRecord::size() const {
  uint64_t recSize = 1 + 1 + 4 + 4 + 2 + 1 + 2 + 1 * 4 + 2 + 1 + 1;
  recSize += m_nonVclArrays.size() * 3;

  for (auto nonVclArray : m_nonVclArrays) {
    recSize += nonVclArray.nalus.size() * 2;
    for (auto nalu : nonVclArray.nalus) {
      recSize += nalu.size();
    }
  }
  return recSize;
}

SAttributeList CHevcDecoderConfigRecord::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Configuration Version";
  attribute.value = std::to_string(m_configurationVersion);
  attributesList.push_back(attribute);

  attribute.key = "General Profile Space";
  attribute.value = std::to_string(m_generalProfileSpace);
  attributesList.push_back(attribute);

  attribute.key = "General Tier Flag";
  attribute.value = std::to_string(m_generalTierFlag);
  attributesList.push_back(attribute);

  attribute.key = "General Profile Idc";
  attribute.value = std::to_string(m_generalProfileIdc);
  attributesList.push_back(attribute);

  attribute.key = "General Profile Compatability Flags";
  attribute.value = std::to_string(m_generalProfileCompatabilityFlags);
  attributesList.push_back(attribute);

  attribute.key = "General Constraint Indicator Flags";
  attribute.value = std::to_string(m_generalConstraintIndicatorFlags);
  attributesList.push_back(attribute);

  attribute.key = "General Level Idc";
  attribute.value = std::to_string(m_generalLevelIdc);
  attributesList.push_back(attribute);

  attribute.key = "Min Spatial Segmentation Idc";
  attribute.value = std::to_string(m_minSpatialSegmentationIdc);
  attributesList.push_back(attribute);

  attribute.key = "Parallelism Type";
  attribute.value = std::to_string(m_paralelismType);
  attributesList.push_back(attribute);

  attribute.key = "Chroma Format Idc";
  attribute.value = std::to_string(m_chromaFormatIdc);
  attributesList.push_back(attribute);

  attribute.key = "Bit Depth Luma Minus 8";
  attribute.value = std::to_string(m_bitDepthLumaMinus8);
  attributesList.push_back(attribute);

  attribute.key = "Bit Depth Croma Minus 8";
  attribute.value = std::to_string(m_bitDepthChromaMinus8);
  attributesList.push_back(attribute);

  attribute.key = "Average Frame Rate";
  attribute.value = std::to_string(m_avgFrameRate);
  attributesList.push_back(attribute);

  attribute.key = "Constant Frame Rate";
  attribute.value = std::to_string(m_constFrameRate);
  attributesList.push_back(attribute);

  attribute.key = "Num Temporal Layers";
  attribute.value = std::to_string(m_numTemporatlLayers);
  attributesList.push_back(attribute);

  attribute.key = "Temporal Id Nested";
  attribute.value = std::to_string(m_temporalIdNested);
  attributesList.push_back(attribute);

  attribute.key = "Lenght Size Minus One";
  attribute.value = std::to_string(m_lengthSizeMinusOne);
  attributesList.push_back(attribute);

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
        vcl << "[";
        for (size_t i = 0; i < nalu.size(); ++i) {
          vcl << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
              << static_cast<int>(nalu[i]);

          if (i != nalu.size() - 1) {
            vcl << " ";
          } else {
            vcl << "]";
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

}  // namespace config
}  // namespace isobmff
}  // namespace mmt
