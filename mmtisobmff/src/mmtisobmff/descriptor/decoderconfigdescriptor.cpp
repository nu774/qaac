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
 * Content: decoder config descriptor class
 */

// System includes
#include <limits>

// External includes
#include "ilo/bytebuffertools.h"
#include "common/logging.h"

// Internal includes
#include "decoderconfigdescriptor.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace descriptor {
CDecoderConfigDescriptor::CDecoderConfigDescriptor() {}

CDecoderConfigDescriptor::CDecoderConfigDescriptor(ilo::ByteBuffer::const_iterator& begin,
                                                   const ilo::ByteBuffer::const_iterator& end)
    : CBaseDescriptor(begin, end),
      m_objectTypeIndication(0),
      m_streamType(0),
      m_upStream(0),
      m_reserved(0),
      m_bufferSizeDB(0),
      m_maxBitrate(0),
      m_avgBitrate(0),
      m_remainingPayload(0) {
  CDecoderConfigDescriptor::parse(begin, begin + size());
}

CDecoderConfigDescriptor::CDecoderConfigDescriptor(
    const SDecoderConfigDescriptorWriteConfig& descriptorData)
    : CBaseDescriptor(descriptorData),
      m_objectTypeIndication(descriptorData.objectTypeIndication),
      m_streamType(descriptorData.streamType),
      m_upStream(descriptorData.upStream),
      m_reserved(descriptorData.reserved),
      m_bufferSizeDB(descriptorData.bufferSizeDB),
      m_maxBitrate(descriptorData.maxBitrate),
      m_avgBitrate(descriptorData.avgBitrate),
      m_decoderSpecificInfo(descriptorData.decoderSpecificInfo) {
  updateSize(0);
}

void CDecoderConfigDescriptor::updateSize(uint32_t sizeValue) {
  sizeValue += 1;  // objectTypeIndication
  sizeValue += 1;  // (streamType + upStream + reserved)
  sizeValue += 3;  // buffersizeDB
  sizeValue += 4;  // maxBitrate
  sizeValue += 4;  // avgBitrate + decoderspecificinfo(tag + size + byteblob)
  sizeValue += 1;  // decoderSpecificInfo tag
  if (m_decoderSpecificInfo) {
    sizeValue +=
        (m_decoderSpecificInfo->size() / MAX_SIZE_IN_ONE_BYTE) +
        ((m_decoderSpecificInfo->size() % MAX_SIZE_IN_ONE_BYTE) ? 1
                                                                : 0);  // decoderSpecificInfo size
    sizeValue += m_decoderSpecificInfo->size();  // decoderSpecificInfo payload
  }
  CBaseDescriptor::updateSize(sizeValue);
}

SAttributeList CDecoderConfigDescriptor::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Object Type Indication";
  attribute.value = std::to_string(m_objectTypeIndication);
  attributesList.push_back(attribute);

  attribute.key = "Stream Type";
  attribute.value = std::to_string(m_streamType);
  attributesList.push_back(attribute);

  attribute.key = "Up Stream";
  attribute.value = std::to_string(m_upStream);
  attributesList.push_back(attribute);

  attribute.key = "Buffer Size DB";
  attribute.value = std::to_string(m_bufferSizeDB);
  attributesList.push_back(attribute);

  attribute.key = "Max Bitrate";
  attribute.value = std::to_string(m_maxBitrate);
  attributesList.push_back(attribute);

  attribute.key = "Avg Bitrate";
  attribute.value = std::to_string(m_avgBitrate);
  attributesList.push_back(attribute);

  attribute.key = "Decoder Specific Info";
  auto dsc = m_decoderSpecificInfo ? m_decoderSpecificInfo->getByteBlob() : ilo::ByteBuffer{};
  std::stringstream dsi;

  for (auto& byte : dsc) {
    dsi << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
        << static_cast<int>(byte);
    dsi << " ";
  }

  attribute.value = dsi.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
  attributesList.push_back(attribute);

  if (!m_remainingPayload.empty()) {
    attribute.key = "Remaining Payload";
    std::stringstream ss;
    for (auto& byte : m_remainingPayload) {
      ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
         << static_cast<int>(byte);
      ss << " ";
    }
    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CDecoderConfigDescriptor::parse(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::DecoderConfigDescriptor, std::invalid_argument,
                  "CDecoderConfigDescriptor: tag is %d and it should be %d",
                  static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::DecoderConfigDescriptor));

  ILO_ASSERT_WITH(size() <= (static_cast<size_t>(end - begin)), std::logic_error,
                  "CDecoderConfigDescriptor: not enough data in buffer");

  ilo::ByteBuffer::const_iterator dcdStart = begin;

  m_objectTypeIndication = readUint8(begin, end);

  uint8_t tmpByte = readUint8(begin, end);
  m_streamType = (tmpByte & 0xFC) >> 2;
  m_upStream = (tmpByte & 0x02) >> 1;
  m_reserved = (tmpByte & 0x01);

  m_bufferSizeDB = readUint24(begin, end);
  m_maxBitrate = readUint32(begin, end);
  m_avgBitrate = readUint32(begin, end);

  // parsing decoder specific info
  if (static_cast<size_t>(begin - dcdStart) != size() &&
      peekTag(begin, end) == EDescriptorTag::DecoderSpecificInfo) {
    m_decoderSpecificInfo = std::make_shared<CGenericDecoderSpecificInfo>(begin, end);
  }

  // parsing any profileLevelIndicationIndexDescriptor (we are not interested in this data)
  if (begin != end) {
    m_remainingPayload = ilo::ByteBuffer(begin, end);
    begin = end;
  }
  return;
}

void CDecoderConfigDescriptor::writeDescriptor(ilo::ByteBuffer& buffer,
                                               ilo::ByteBuffer::iterator& position) const {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::DecoderConfigDescriptor, std::invalid_argument,
                  "CDecoderConfigDescriptor: tag is %d and it should be %d",
                  static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::DecoderConfigDescriptor));

  ILO_ASSERT_WITH(static_cast<size_t>(buffer.end() - position) >= size(), std::logic_error,
                  "CDecoderConfigDescriptor: not enough space in buffer");

  writeUint8(buffer, position, m_objectTypeIndication);

  uint8_t tmpByte = 0;
  tmpByte = static_cast<uint8_t>(m_streamType << 2);
  tmpByte |= (m_upStream << 1);
  tmpByte |= m_reserved;

  writeUint8(buffer, position, tmpByte);
  writeUint24(buffer, position, m_bufferSizeDB);
  writeUint32(buffer, position, m_maxBitrate);
  writeUint32(buffer, position, m_avgBitrate);

  if (m_decoderSpecificInfo && m_decoderSpecificInfo->size() != 0) {
    m_decoderSpecificInfo->write(buffer, position);
  }

  // writing possible profileLevelIndicationIndexDescriptor
  for (size_t i = 0; i < m_remainingPayload.size(); i++) {
    writeUint8(buffer, position, m_remainingPayload.at(i));
  }
  return;
}
}  // namespace descriptor
}  // namespace isobmff
}  // namespace mmt
