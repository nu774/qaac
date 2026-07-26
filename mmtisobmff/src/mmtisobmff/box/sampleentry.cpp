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
 * Content: abstract sample entry, audio sample entry and visual sample entry classes
 */

// System headers
#include <exception>
#include <numeric>
#include <stdexcept>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "sampleentry.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CSampleEntry::CSampleEntry(ilo::ByteBuffer::const_iterator& begin,
                           const ilo::ByteBuffer::const_iterator& end)
    : CBox(begin, end), m_dataReferenceIndex(1) {
  parse(begin, end);
}

CSampleEntry::CSampleEntry(const SSampleEntryWriteConfig& config)
    : box::CBox(config), m_dataReferenceIndex(config.dataReferenceIndex) {
  sanityCheck();
  updateSize(0);
}

void CSampleEntry::parse(ilo::ByteBuffer::const_iterator& begin,
                         const ilo::ByteBuffer::const_iterator& end) {
  std::vector<uint8_t> reserved = ilo::readUint8Array(begin, end, 6);

  ILO_ASSERT(std::accumulate(reserved.begin(), reserved.end(), 0) == 0,
             "All reserved values in sample entry must be 0");

  m_dataReferenceIndex = ilo::readUint16(begin, end);

  sanityCheck();
}

void CSampleEntry::updateSize(uint64_t sizeValue) {
  // size + reserved + data_reference_index
  CBox::updateSize(sizeValue + 6 + 2);
}

void CSampleEntry::writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  box::CBox::writeHeader(buffer, position);

  for (uint16_t i = 0; i < 6; ++i) {
    ilo::writeUint8(buffer, position, 0);
  }

  ilo::writeUint16(buffer, position, m_dataReferenceIndex);
}

void CSampleEntry::sanityCheck() {
  ILO_ASSERT_WITH(m_dataReferenceIndex > 0, std::invalid_argument,
                  "SampleEntry: Data reference index must be > 0");
}

CAudioSampleEntry::CAudioSampleEntry(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end)
    : CSampleEntry(begin, end), m_channelCount(0), m_sampleSize(0), m_sampleRate(0) {
  parseBox(begin, end);
}

CAudioSampleEntry::CAudioSampleEntry(const SAudioSampleEntryWriteConfig& config)
    : box::CSampleEntry(config),
      m_channelCount(config.channelCount),
      m_sampleSize(config.sampleSize),
      m_sampleRate(config.sampleRate) {
  updateSize(0);

  ILO_ASSERT_WITH(m_sampleRate <= 0xFFFFu, std::invalid_argument,
                  "AudioSampleEntry: 32 bit sample rate is not supported currently");
}

void CAudioSampleEntry::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                 const ilo::ByteBuffer::const_iterator& end) {
  std::vector<uint32_t> reserved = ilo::readUint32Array(begin, end, 2);

  ILO_ASSERT(std::accumulate(reserved.begin(), reserved.end(), 0) == 0,
             "All reserved values in audio sample entry must be 0");

  // channel count
  m_channelCount = ilo::readUint16(begin, end);

  // sample size
  m_sampleSize = ilo::readUint16(begin, end);

  // predefined
  ILO_ASSERT(ilo::readUint16(begin, end) == 0,
             "Predefined value in audio sample entry must be zero");

  // reserved
  ILO_ASSERT(ilo::readUint16(begin, end) == 0, "Reserved value in audio sample entry must be zero");

  m_sampleRate = ilo::readUint32(begin, end) >> 16;
}

SAttributeList CAudioSampleEntry::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;

  attribute.key = "Channel Count";
  attribute.value = std::to_string(m_channelCount);
  attributesList.push_back(attribute);

  attribute.key = "Sample Size";
  attribute.value = std::to_string(m_sampleSize);
  attributesList.push_back(attribute);

  attribute.key = "Sample Rate";
  attribute.value = std::to_string(m_sampleRate);
  attributesList.push_back(attribute);

  attribute.key = "Data Reference Index";
  attribute.value = std::to_string(dataReferenceIndex());
  attributesList.push_back(attribute);

  return attributesList;
}

void CAudioSampleEntry::updateSize(uint64_t sizeValue) {
  // size + reserved + channelcount + samplesize + preDefined + reserved +
  // samplerate
  CSampleEntry::updateSize(sizeValue + 8 + 2 + 2 + 2 + 2 + 4);
}

void CAudioSampleEntry::writeHeader(ilo::ByteBuffer& buffer,
                                    ilo::ByteBuffer::iterator& position) const {
  box::CSampleEntry::writeHeader(buffer, position);

  for (uint32_t i = 0; i < 2; ++i) {
    ilo::writeUint32(buffer, position, 0);
  }

  ilo::writeUint16(buffer, position, m_channelCount);
  ilo::writeUint16(buffer, position, m_sampleSize);
  ilo::writeUint16(buffer, position, 0);
  ilo::writeUint16(buffer, position, 0);
  ilo::writeUint32(buffer, position, m_sampleRate << 16);
}

CVisualSampleEntry::CVisualSampleEntry(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end)
    : CSampleEntry(begin, end),
      m_width(0),
      m_height(0),
      m_horizResolution(0x00480000),
      m_vertResolution(0x00480000),
      m_frameCount(1),
      m_depth(0x0018) {
  parseBox(begin, end);
}

CVisualSampleEntry::CVisualSampleEntry(const SVisualSampleEntryWriteConfig& config)
    : box::CSampleEntry(config),
      m_width(config.width),
      m_height(config.height),
      m_frameCount(config.frameCount),
      m_compressorName(config.compressorName),
      m_depth(config.depth) {
  ILO_ASSERT_WITH(
      config.horizResolutionDpi <= 0xFFFFu && config.vertResolutionDpi <= 0xFFFFu,
      std::invalid_argument,
      "VisualSampleEntry: 32 bit resolution (hor/vert) in dpi is not supported currently");

  ILO_ASSERT(m_compressorName.size() < 32,
             "VisualSampleEntry: compressor name is too long (max size is 31)");

  m_horizResolution = config.horizResolutionDpi << 16;
  m_vertResolution = config.vertResolutionDpi << 16;

  updateSize(0);
}

void CVisualSampleEntry::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                  const ilo::ByteBuffer::const_iterator& end) {
  std::vector<uint32_t> predefined = ilo::readUint32Array(begin, end, 4);

  ILO_ASSERT(std::accumulate(predefined.begin(), predefined.end(), 0) == 0,
             "All predefined values in visual audio sample entry must be 0");

  m_width = ilo::readUint16(begin, end);
  m_height = ilo::readUint16(begin, end);
  m_horizResolution = ilo::readUint32(begin, end);
  m_vertResolution = ilo::readUint32(begin, end);

  ILO_ASSERT(ilo::readUint32(begin, end) == 0,
             "Reserved value in visual sample entry must be zero");

  m_frameCount = ilo::readUint16(begin, end);

  ILO_ASSERT(m_frameCount > 0, "VisualSampleEntry: FrameCount must be greater than 0");

  std::vector<uint8_t> compressorVector;
  compressorVector = ilo::readUint8Array(begin, end, 32);

  // First byte contains the size of the compressor name
  ILO_ASSERT(compressorVector[0] < 32,
             "VisualSampleEntry: compressor name is too long (max size is 31)");

  // Create a string from the valid range of compressorVector
  m_compressorName =
      std::string(compressorVector.begin() + 1, compressorVector.begin() + 1 + compressorVector[0]);

  m_depth = ilo::readUint16(begin, end);

  // predefined
  ILO_ASSERT(ilo::readInt16(begin, end) == -1, "VisualSampleEntry: predefined value must be -1");
}

void CVisualSampleEntry::updateSize(uint64_t sizeValue) {
  // size + preDefined + reserved + preDefined + width + height + horRes +
  // vertRes + reserved + frameCount + compressorName + depth + preDefined
  CSampleEntry::updateSize(sizeValue + 2 + 2 + 12 + 2 + 2 + 4 + 4 + 4 + 2 + 32 + 2 + 2);
}

void CVisualSampleEntry::writeHeader(ilo::ByteBuffer& buffer,
                                     ilo::ByteBuffer::iterator& position) const {
  box::CSampleEntry::writeHeader(buffer, position);

  ilo::writeUint16(buffer, position, 0);
  ilo::writeUint16(buffer, position, 0);

  for (uint32_t i = 0; i < 3; ++i) {
    ilo::writeUint32(buffer, position, 0);
  }

  ilo::writeUint16(buffer, position, m_width);
  ilo::writeUint16(buffer, position, m_height);
  ilo::writeUint32(buffer, position, m_horizResolution);
  ilo::writeUint32(buffer, position, m_vertResolution);
  ilo::writeUint32(buffer, position, 0);
  ilo::writeUint16(buffer, position, m_frameCount);

  // Write length of compressor name (if valid)
  ilo::writeUint8(buffer, position, static_cast<uint8_t>(m_compressorName.size()));

  // Write compressor name
  for (auto compressorChar : m_compressorName) {
    ilo::writeUint8(buffer, position, compressorChar);
  }

  // Fill the rest of the compressor byte block with zeros
  for (size_t i = m_compressorName.size(); i < 31; ++i) {
    ilo::writeUint8(buffer, position, 0);
  }

  ilo::writeUint16(buffer, position, m_depth);
  ilo::writeInt16(buffer, position, -1);
}

SAttributeList CVisualSampleEntry::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Width";
  attribute.value = std::to_string(m_width);
  attributesList.push_back(attribute);

  attribute.key = "Height";
  attribute.value = std::to_string(m_height);
  attributesList.push_back(attribute);

  attribute.key = "Horiz Resolution";
  std::stringstream ss;
  ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase
     << static_cast<int>(m_horizResolution);
  attribute.value = ss.str();
  attributesList.push_back(attribute);

  attribute.key = "Vert Resolution";
  ss.clear();
  ss.str("");
  ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase
     << static_cast<int>(m_vertResolution);
  attribute.value = ss.str();
  attributesList.push_back(attribute);

  attribute.key = "Frame Count";
  attribute.value = std::to_string(m_frameCount);
  attributesList.push_back(attribute);

  attribute.key = "Depth";
  ss.clear();
  ss.str("");
  ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << std::uppercase
     << static_cast<int>(m_depth);
  attribute.value = ss.str();
  attributesList.push_back(attribute);

  attribute.key = "Compressor Name";
  attribute.value = m_compressorName;
  attributesList.push_back(attribute);

  attribute.key = "Data Reference Index";
  attribute.value = std::to_string(dataReferenceIndex());
  attributesList.push_back(attribute);

  return attributesList;
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt
