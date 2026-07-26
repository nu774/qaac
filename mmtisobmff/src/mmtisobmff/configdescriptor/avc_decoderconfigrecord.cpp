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
 * Content: avc decoder configuration record
 */

// System includes
#include <algorithm>
#include <limits>

// External includes
#include "ilo/bytebuffertools.h"

// Internal includes
#include "mmtisobmff/configdescriptor/avc_decoderconfigrecord.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace config {
static const uint8_t UINT8_T_MAX = std::numeric_limits<uint8_t>::max();

CAvcDecoderConfigRecord::CAvcDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                                                 const ilo::ByteBuffer::const_iterator& end)
    : m_configurationVersion(1),
      m_avcProfileIndication(0),
      m_profileCompatibility(0),
      m_avcLevelIndication(0),
      m_lengthSizeMinusOne(0),
      m_chromaFormat(0),
      m_bitDepthLumaMinus8(0),
      m_bitDepthChromaMinus8(0) {
  parse(begin, end);
}

CAvcDecoderConfigRecord::CAvcDecoderConfigRecord()
    : m_configurationVersion(1),
      m_avcProfileIndication(0),
      m_profileCompatibility(0),
      m_avcLevelIndication(0),
      m_lengthSizeMinusOne(0),
      m_chromaFormat(0),
      m_bitDepthLumaMinus8(0),
      m_bitDepthChromaMinus8(0) {}

void CAvcDecoderConfigRecord::setConfigurationVersion(uint8_t configVersion) {
  m_configurationVersion = configVersion;
}

void CAvcDecoderConfigRecord::setAvcProfileIndication(uint8_t profileIndication) {
  m_avcProfileIndication = profileIndication;
}

void CAvcDecoderConfigRecord::setProfileCompatibility(uint8_t profileCompat) {
  m_profileCompatibility = profileCompat;
}

void CAvcDecoderConfigRecord::setAvcLevelIndication(uint8_t levelIndication) {
  m_avcLevelIndication = levelIndication;
}

void CAvcDecoderConfigRecord::setLengthSizeMinusOne(uint8_t lengthSizeMinusOneValue) {
  m_lengthSizeMinusOne = lengthSizeMinusOneValue;
}

void CAvcDecoderConfigRecord::setChromaFormat(uint8_t chromaFormatValue) {
  m_chromaFormat = chromaFormatValue;
}

void CAvcDecoderConfigRecord::setBitDepthLumaMinus8(uint8_t bitDepthLumaMinus8Value) {
  m_bitDepthLumaMinus8 = bitDepthLumaMinus8Value;
}

void CAvcDecoderConfigRecord::setBitDepthChromaMinus8(uint8_t bitDepthChromaMinus8Value) {
  m_bitDepthChromaMinus8 = bitDepthChromaMinus8Value;
}

void CAvcDecoderConfigRecord::setSequenceParameterSets(const SAvcParamVector& spsVector) {
  // sequence parameter set count has to fit into 5 bits
  ILO_ASSERT((spsVector.size() <= 0x1Fu),
             "SequenceParameterSets count exceeded expected size (5 bits)");

  for (auto sps : spsVector) {
    ILO_ASSERT(sps.size() <= std::numeric_limits<uint16_t>::max(),
               "SPS exceeded expected size (16 bits)");

    m_sequenceParameterSets.push_back(sps);
  }
}

void CAvcDecoderConfigRecord::setPictureParameterSets(const SAvcParamVector& ppsVector) {
  ILO_ASSERT(ppsVector.size() <= std::numeric_limits<uint8_t>::max(),
             "PictureParameterSets count exceeded expected size (8 bits)");

  for (auto pps : ppsVector) {
    ILO_ASSERT(pps.size() <= std::numeric_limits<uint16_t>::max(),
               "PPS exceeded expected size (16 bits)");

    m_pictureParameterSets.push_back(pps);
  }
}

void CAvcDecoderConfigRecord::setSequenceParameterExtSets(const SAvcParamVector& spsExtVector) {
  ILO_ASSERT(spsExtVector.size() <= std::numeric_limits<uint8_t>::max(),
             "SequenceParameterExtSets count exceeded expected size (8 bits)");

  for (auto spsExt : spsExtVector) {
    ILO_ASSERT(spsExt.size() <= std::numeric_limits<uint16_t>::max(),
               "SPS ext exceeded expected size (16 bits)!");

    m_sequenceParameterExtSets.push_back(spsExt);
  }
}

CAvcDecoderConfigRecord::SAvcParamVector readParameterSets(
    ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end,
    bool isSPS) {
  CAvcDecoderConfigRecord::SAvcParamVector result;

  uint8_t parameterSetCount = ilo::readUint8(begin, end);
  if (isSPS) {
    parameterSetCount = static_cast<uint8_t>(parameterSetCount & 0x1F);
  }

  for (uint8_t i = 0; i < parameterSetCount; ++i) {
    uint16_t parameterSetLength = ilo::readUint16(begin, end);

    ILO_ASSERT(end - begin >= parameterSetLength, "parameter set too large for buffer");

    result.push_back(ilo::ByteBuffer(begin, begin + parameterSetLength));
    begin += parameterSetLength;
  }
  return result;
}

void CAvcDecoderConfigRecord::parse(ilo::ByteBuffer::const_iterator& begin,
                                    const ilo::ByteBuffer::const_iterator& end) {
  m_configurationVersion = ilo::readUint8(begin, end);

  ILO_ASSERT_WITH(m_configurationVersion == 1, std::invalid_argument,
                  "avc config record version not supported: %d", m_configurationVersion);

  m_avcProfileIndication = ilo::readUint8(begin, end);
  m_profileCompatibility = ilo::readUint8(begin, end);
  m_avcLevelIndication = ilo::readUint8(begin, end);

  uint8_t lengthSizeMinusOneValue = ilo::readUint8(begin, end);
  m_lengthSizeMinusOne = static_cast<uint8_t>(lengthSizeMinusOneValue & 0x03);

  m_sequenceParameterSets = readParameterSets(begin, end, true);
  m_pictureParameterSets = readParameterSets(begin, end, false);

  if (m_avcProfileIndication == 100 || m_avcProfileIndication == 110 ||
      m_avcProfileIndication == 122 || m_avcProfileIndication == 144) {
    if (begin == end) {
      ILO_LOG_ERROR(
          "avc config record: additional information should be parsed but not enough bits are "
          "available");
      sanityCheck();
      return;
    }

    m_chromaFormat = static_cast<uint8_t>(ilo::readUint8(begin, end) & 0x03);
    m_bitDepthLumaMinus8 = static_cast<uint8_t>(ilo::readUint8(begin, end) & 0x07);
    m_bitDepthChromaMinus8 = static_cast<uint8_t>(ilo::readUint8(begin, end) & 0x07);

    m_sequenceParameterExtSets = readParameterSets(begin, end, false);
  }
  sanityCheck();
}

void CAvcDecoderConfigRecord::write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) {
  sanityCheck();

  ilo::writeUint8(buffer, position, m_configurationVersion);
  ilo::writeUint8(buffer, position, m_avcProfileIndication);
  ilo::writeUint8(buffer, position, m_profileCompatibility);
  ilo::writeUint8(buffer, position, m_avcLevelIndication);

  uint8_t lengthSizeMinusOnePadded =
      static_cast<uint8_t>((m_lengthSizeMinusOne | (UINT8_T_MAX << 2)));
  ilo::writeUint8(buffer, position, lengthSizeMinusOnePadded);

  uint8_t numOfSequenceParameterSetsPadded = static_cast<uint8_t>(
      static_cast<uint8_t>(m_sequenceParameterSets.size()) | (UINT8_T_MAX << 5));
  ilo::writeUint8(buffer, position, numOfSequenceParameterSetsPadded);

  for (auto parameterSet : m_sequenceParameterSets) {
    ilo::writeUint16(buffer, position, static_cast<uint16_t>(parameterSet.size()));

    ILO_ASSERT(static_cast<size_t>(buffer.end() - position) >= parameterSet.size(),
               "SPS data does not fit in buffer");

    std::copy(parameterSet.begin(), parameterSet.end(), position);

    position += parameterSet.size();
  }

  ilo::writeUint8(buffer, position, static_cast<uint8_t>(m_pictureParameterSets.size()));

  for (auto parameterSet : m_pictureParameterSets) {
    ilo::writeUint16(buffer, position, static_cast<uint16_t>(parameterSet.size()));

    ILO_ASSERT(static_cast<size_t>(buffer.end() - position) >= parameterSet.size(),
               "PPS data does not fit in buffer");

    std::copy(parameterSet.begin(), parameterSet.end(), position);

    position += parameterSet.size();
  }

  if (m_avcProfileIndication == 100 || m_avcProfileIndication == 110 ||
      m_avcProfileIndication == 122 || m_avcProfileIndication == 144) {
    uint8_t chromaFormatPadded = static_cast<uint8_t>(m_chromaFormat | (UINT8_T_MAX << 2));
    ilo::writeUint8(buffer, position, chromaFormatPadded);

    uint8_t bitDepthLumaMinus8Padded =
        static_cast<uint8_t>(m_bitDepthLumaMinus8 | (UINT8_T_MAX << 3));
    ilo::writeUint8(buffer, position, bitDepthLumaMinus8Padded);

    uint8_t bitDepthChromaMinus8Padded =
        static_cast<uint8_t>(m_bitDepthChromaMinus8 | (UINT8_T_MAX << 3));
    ilo::writeUint8(buffer, position, bitDepthChromaMinus8Padded);

    ilo::writeUint8(buffer, position, static_cast<uint8_t>(m_sequenceParameterExtSets.size()));

    for (auto parameterSet : m_sequenceParameterExtSets) {
      ilo::writeUint16(buffer, position, static_cast<uint16_t>(parameterSet.size()));

      ILO_ASSERT(static_cast<size_t>(buffer.end() - position) >= parameterSet.size(),
                 "SPS-EXT data does not fit in buffer");

      std::copy(parameterSet.begin(), parameterSet.end(), position);

      position += parameterSet.size();
    }
  }
}

uint64_t CAvcDecoderConfigRecord::size() const {
  uint64_t finalSize = 0;
  finalSize++;  // m_configurationVersion
  finalSize++;  // m_avcProfileIndication
  finalSize++;  // m_profileCompatibility
  finalSize++;  // m_avcLevelIndication
  finalSize++;  // m_lengthSizeMinusOne
  finalSize++;  // m_numOfSequenceParameterSets

  for (auto seqParameterSet : m_sequenceParameterSets) {
    finalSize += 2;  // parameterSetLength
    finalSize += seqParameterSet.size();
  }

  finalSize++;  // m_numOfPictureParameterSets

  for (auto picParameterSet : m_pictureParameterSets) {
    finalSize += 2;  // parameterSetLength
    finalSize += picParameterSet.size();
  }

  if (m_avcProfileIndication == 100 || m_avcProfileIndication == 110 ||
      m_avcProfileIndication == 122 || m_avcProfileIndication == 144) {
    finalSize++;  // m_chromaFormat
    finalSize++;  // m_bitDepthLumaMinus8
    finalSize++;  // m_bitDepthChromaMinus8
    finalSize++;  // m_numOfSequenceParameterSetExt

    for (auto seqParameterSetExt : m_sequenceParameterExtSets) {
      finalSize += 2;  // parameterSetLength
      finalSize += seqParameterSetExt.size();
    }
  }
  return finalSize;
}

SAttributeList CAvcDecoderConfigRecord::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Configuration Version";
  attribute.value = std::to_string(m_configurationVersion);
  attributesList.push_back(attribute);

  attribute.key = "AVC Profile Indication";
  attribute.value = std::to_string(m_avcProfileIndication);
  attributesList.push_back(attribute);

  attribute.key = "Profile Compatiblity";
  attribute.value = std::to_string(m_profileCompatibility);
  attributesList.push_back(attribute);

  attribute.key = "AVC Level Indication";
  attribute.value = std::to_string(m_avcLevelIndication);
  ;
  attributesList.push_back(attribute);

  attribute.key = "Length Size Minus One";
  attribute.value = std::to_string(m_lengthSizeMinusOne);
  attributesList.push_back(attribute);

  attribute.key = "Number of Sequence Parameter Sets";
  attribute.value = std::to_string(m_sequenceParameterSets.size());
  attributesList.push_back(attribute);

  if (!m_sequenceParameterSets.empty()) {
    attribute.key = "Sequence Parameter Sets";
    std::stringstream ss;
    for (auto& seqParSet : m_sequenceParameterSets) {
      ss << "[";
      for (size_t i = 0; i < seqParSet.size(); ++i) {
        ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
           << static_cast<int>(seqParSet[i]);

        if (i != seqParSet.size() - 1) {
          ss << " ";
        } else {
          ss << "]";
        }
      }
      ss << "; ";
    }
    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
    attributesList.push_back(attribute);
  }

  attribute.key = "Number of Picture Parameter Sets";
  attribute.value = std::to_string(m_pictureParameterSets.size());
  attributesList.push_back(attribute);

  if (!m_pictureParameterSets.empty()) {
    attribute.key = "Picture Parameter Sets";
    std::stringstream pp;

    for (auto& picParSet : m_pictureParameterSets) {
      pp << "[";
      for (size_t i = 0; i < picParSet.size(); ++i) {
        pp << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
           << static_cast<int>(picParSet[i]);

        if (i != picParSet.size() - 1) {
          pp << " ";
        } else {
          pp << "]";
        }
      }
      pp << "; ";
    }
    attribute.value = pp.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
    attributesList.push_back(attribute);
  }

  if (m_avcProfileIndication == 100 || m_avcProfileIndication == 110 ||
      m_avcProfileIndication == 122 || m_avcProfileIndication == 144) {
    attribute.key = "Chroma Format";
    attribute.value = std::to_string(m_chromaFormat);
    attributesList.push_back(attribute);

    attribute.key = "Bit Depth Luma Minus 8";
    attribute.value = std::to_string(m_bitDepthLumaMinus8);
    attributesList.push_back(attribute);

    attribute.key = "Bit Depth Chroma Minus 8";
    attribute.value = std::to_string(m_bitDepthChromaMinus8);
    attributesList.push_back(attribute);

    attribute.key = "Number of Sequence Parameter Set Ext";
    attribute.value = std::to_string(m_sequenceParameterExtSets.size());
    attributesList.push_back(attribute);

    if (!m_sequenceParameterExtSets.empty()) {
      attribute.key = "Sequence Parameter Set Ext";
      std::stringstream sp;

      for (auto& seqParExtSet : m_sequenceParameterExtSets) {
        sp << "[";
        for (size_t i = 0; i < seqParExtSet.size(); ++i) {
          sp << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
             << static_cast<int>(seqParExtSet[i]);

          if (i != seqParExtSet.size() - 1) {
            sp << " ";
          } else {
            sp << "]";
          }
        }
        sp << "; ";
      }
      attribute.value = sp.str();
      attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
      attributesList.push_back(attribute);
    }
  }

  return attributesList;
}

void CAvcDecoderConfigRecord::sanityCheck() {
  if (m_lengthSizeMinusOne != 0 && m_lengthSizeMinusOne != 1 && m_lengthSizeMinusOne != 3) {
    throw std::invalid_argument("AVC: lengthSizeMinusOne is supposed to have a value of 0, 1 or 3");
  }

  if (m_avcProfileIndication == 100 || m_avcProfileIndication == 110 ||
      m_avcProfileIndication == 122 || m_avcProfileIndication == 144) {
    if (m_bitDepthLumaMinus8 > 4) {
      throw std::invalid_argument(
          "AVC: bitDepthLumaMinus8 is supposed to have a value of between 0 and 4 (inclusive)");
    }
    if (m_bitDepthChromaMinus8 > 4) {
      throw std::invalid_argument(
          "AVC: bitDepthChromaMinus8 is supposed to have a value of between 0 and 4 (inclusive)");
    }
  }
}
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
