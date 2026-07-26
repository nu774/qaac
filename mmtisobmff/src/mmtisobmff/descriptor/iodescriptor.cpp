/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2018 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * Content: Initial Object descriptor class
 */

// System includes
#include <limits>

// External includes
#include "ilo/bytebuffertools.h"
#include "common/logging.h"

// Internal includes
#include "iodescriptor.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace descriptor {
CIODescriptor::CIODescriptor(ilo::ByteBuffer::const_iterator& begin,
                             const ilo::ByteBuffer::const_iterator& end)
    : CBaseDescriptor(begin, end),
      m_objectDescriptorId(0),
      m_URLflag(0),
      m_includeInlineProfileLevelFlag(0),
      m_URLlength(0),
      m_ODProfileLevelIndication(0),
      m_sceneProfileLevelIndication(0),
      m_audioProfileLevelIndication(0),
      m_visualProfileLevelIndication(0),
      m_graphicsProfileLevelIndication(0) {
  CIODescriptor::parse(begin, begin + size());
}

CIODescriptor::CIODescriptor(const SIODescriptorWriteConfig& descriptorData)
    : CBaseDescriptor(descriptorData.tag),
      m_objectDescriptorId(descriptorData.objectDescriptorId),
      m_URLflag(descriptorData.URLflag),
      m_includeInlineProfileLevelFlag(descriptorData.includeInlineProfileLevelFlag),
      m_URLlength(descriptorData.URLlength),
      m_ODProfileLevelIndication(descriptorData.ODProfileLevelIndication),
      m_sceneProfileLevelIndication(descriptorData.sceneProfileLevelIndication),
      m_audioProfileLevelIndication(descriptorData.audioProfileLevelIndication),
      m_visualProfileLevelIndication(descriptorData.visualProfileLevelIndication),
      m_graphicsProfileLevelIndication(descriptorData.graphicsProfileLevelIndication),
      m_esIdIncDescriptors(descriptorData.esIdIncDescriptors) {
  updateSize(0);
}

SAttributeList CIODescriptor::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;

  attribute.key = "Object Descriptor Id";
  attribute.value = std::to_string(m_objectDescriptorId);
  attributesList.push_back(attribute);

  attribute.key = "URLflag";
  attribute.value = std::to_string(m_URLflag);
  attributesList.push_back(attribute);

  attribute.key = "Include Inline Profile Level Flag";
  attribute.value = std::to_string(m_includeInlineProfileLevelFlag);
  attributesList.push_back(attribute);

  attribute.key = "URLlength";
  attribute.value = std::to_string(m_URLlength);
  attributesList.push_back(attribute);

  attribute.key = "OD Profile Level Indication";
  attribute.value = std::to_string(m_ODProfileLevelIndication);
  attributesList.push_back(attribute);

  attribute.key = "Scene Profile Level Indication";
  attribute.value = std::to_string(m_sceneProfileLevelIndication);
  attributesList.push_back(attribute);

  attribute.key = "Audio Profile Level Indication";
  attribute.value = std::to_string(m_audioProfileLevelIndication);
  attributesList.push_back(attribute);

  attribute.key = "Visual Profile Level Indication";
  attribute.value = std::to_string(m_visualProfileLevelIndication);
  attributesList.push_back(attribute);

  attribute.key = "Graphics Profile Level Indication";
  attribute.value = std::to_string(m_graphicsProfileLevelIndication);
  attributesList.push_back(attribute);

  attribute.key = "URLstring";
  std::stringstream st;
  for (auto url : m_URLstring) {
    st << std::to_string(url) << ";";
  }

  attribute.value = st.str();
  if (!attribute.value.empty()) {
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
  }
  attributesList.push_back(attribute);

  for (auto& esIdIncDesc : m_esIdIncDescriptors) {
    attribute.key = "ES ID Inc Descriptor";
    std::stringstream ss;
    auto descList = esIdIncDesc.getAttributeList();

    ss << "{";
    for (size_t i = 0; i < descList.size(); ++i) {
      ss << descList[i].key << ": " << descList[i].value;

      if (i != descList.size() - 1) {
        ss << ", ";
      } else {
        ss << "}";
      }
    }
    attribute.value = ss.str();
    attributesList.push_back(attribute);
  }

  if (!m_remainingPayload.empty()) {
    std::stringstream ss;
    attribute.key = "Remaining Payload";

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

void CIODescriptor::updateSize(uint32_t sizeValue) {
  sizeValue += 2;  // ObjectDescriptorID, URLflag, includeInlineProfileLevelFlag, reserved

  if (m_URLflag) {
    sizeValue += (1 + m_URLlength);  // URLlength, URLstring[URLlength]
  } else {
    sizeValue += 1;  // ODProfileLevelIndication;
    sizeValue += 1;  // sceneProfileLevelIndication;
    sizeValue += 1;  // audioProfileLevelIndication;
    sizeValue += 1;  // visualProfileLevelIndication;
    sizeValue += 1;  // graphicsProfileLevelIndication;

    for (const auto& esIdIncDescriptor : m_esIdIncDescriptors) {
      sizeValue += 1;  // ES descriptor tag
      sizeValue +=
          (esIdIncDescriptor.size() / MAX_SIZE_IN_ONE_BYTE) +
          ((esIdIncDescriptor.size() % MAX_SIZE_IN_ONE_BYTE) ? 1 : 0);  // ES descriptor size
      sizeValue += esIdIncDescriptor.size();                            // ES descriptor payload
    }
  }

  CBaseDescriptor::updateSize(sizeValue);
}

void CIODescriptor::parse(ilo::ByteBuffer::const_iterator& begin,
                          const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::MP4InitialObjectDescriptor, std::invalid_argument,
                  "CIODescriptor: tag is %d and it should be %d", static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::MP4InitialObjectDescriptor));

  ILO_ASSERT_WITH(size() <= (static_cast<size_t>(end - begin)), std::logic_error,
                  "CIODescriptor: not enough data in buffer");

  uint16_t tmp = readUint16(begin, end);
  m_objectDescriptorId = (tmp & 0xFFC0) >> 6;
  m_URLflag = (tmp & 0x0020) >> 5;
  m_includeInlineProfileLevelFlag = (tmp & 0x0010) >> 4;

  if (m_URLflag) {
    m_URLlength = readUint8(begin, end);
    if (m_URLlength) {
      for (uint8_t i = 0; i < m_URLlength; i++) {
        m_URLstring.push_back(readUint8(begin, end));
      }
    }
  } else {
    m_ODProfileLevelIndication = readUint8(begin, end);
    m_sceneProfileLevelIndication = readUint8(begin, end);
    m_audioProfileLevelIndication = readUint8(begin, end);
    m_visualProfileLevelIndication = readUint8(begin, end);
    m_graphicsProfileLevelIndication = readUint8(begin, end);

    while (begin != end && peekTag(begin, end) == EDescriptorTag::ESIdIncDescriptor) {
      m_esIdIncDescriptors.push_back(CESIdIncDescriptor(begin, end));
    }
  }

  // parsing other payload that could be in the buffer e.g some other descriptors.
  if (begin != end) {
    m_remainingPayload = ilo::ByteBuffer(begin, end);
    begin = end;
  }
}

void CIODescriptor::writeDescriptor(ilo::ByteBuffer& buffer,
                                    ilo::ByteBuffer::iterator& position) const {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::MP4InitialObjectDescriptor, std::invalid_argument,
                  "CIODescriptor: tag is %d and it should be %d", static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::MP4InitialObjectDescriptor));

  ILO_ASSERT_WITH(static_cast<size_t>(buffer.end() - position) >= size(), std::logic_error,
                  "CIODescriptor: not enough space in buffer");

  uint16_t tmp = 0;
  tmp = static_cast<uint8_t>(m_objectDescriptorId << 6);
  tmp |= (m_URLflag << 5);
  tmp |= (m_includeInlineProfileLevelFlag << 4);
  tmp |= 0x000F;

  writeUint16(buffer, position, tmp);

  if (m_URLflag) {
    writeUint8(buffer, position, m_URLlength);
    for (uint8_t i = 0; i < m_URLlength; i++) {
      writeUint8(buffer, position, m_URLstring[i]);
    }
  } else {
    writeUint8(buffer, position, m_ODProfileLevelIndication);
    writeUint8(buffer, position, m_sceneProfileLevelIndication);
    writeUint8(buffer, position, m_audioProfileLevelIndication);
    writeUint8(buffer, position, m_visualProfileLevelIndication);
    writeUint8(buffer, position, m_graphicsProfileLevelIndication);

    for (const auto& esIdIncDescriptor : m_esIdIncDescriptors) {
      esIdIncDescriptor.write(buffer, position);
    }
  }

  // writing other payload that could be in the descriptor
  if (m_remainingPayload.size() != 0) {
    for (size_t i = 0; i < m_remainingPayload.size(); i++) {
      writeUint8(buffer, position, m_remainingPayload.at(i));
    }
  }
}
}  // namespace descriptor
}  // namespace isobmff
}  // namespace mmt
