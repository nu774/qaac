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
 * Content: ES descriptor class
 */

// System includes
#include <limits>

// External includes
#include "ilo/bytebuffertools.h"
#include "common/logging.h"

// Internal includes
#include "esdescriptor.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace descriptor {
CESDescriptor::CESDescriptor(ilo::ByteBuffer::const_iterator& begin,
                             const ilo::ByteBuffer::const_iterator& end)
    : CBaseDescriptor(begin, end),
      m_ES_id(0),
      m_streamDependenceFlag(0),
      m_URLflag(0),
      m_OCRstreamFlag(0),
      m_streamPriority(0),
      m_dependsOn_ES_ID(0),
      m_URLlength(0),
      m_OCR_ES_Id(0) {
  CESDescriptor::parse(begin, begin + size());
}

CESDescriptor::CESDescriptor(const SESDescriptorWriteConfig& descriptorData)
    : CBaseDescriptor(descriptorData),
      m_ES_id(descriptorData.ES_id),
      m_streamDependenceFlag(descriptorData.streamDependenceFlag),
      m_URLflag(descriptorData.URLflag),
      m_OCRstreamFlag(descriptorData.OCRstreamFlag),
      m_streamPriority(descriptorData.streamPriority),
      m_dependsOn_ES_ID(descriptorData.dependsOn_ES_ID),
      m_URLlength(descriptorData.URLlength),
      m_OCR_ES_Id(descriptorData.OCR_ES_Id),
      m_URLstring(descriptorData.URLstring),
      m_dcd(descriptorData.dcd),
      m_slConfigDescriptor(descriptorData.slConfigDescriptor) {
  updateSize(0);
}

void CESDescriptor::updateSize(uint32_t sizeValue) {
  sizeValue += 2;  // ES_ID
  sizeValue += 1;  // streamDependenceFlag, URLflag, OCRStreamFlag, streamPriority

  m_streamDependenceFlag ? sizeValue += 2 : 0;     // dependsOn_ES_ID
  m_URLflag ? sizeValue += (1 + m_URLlength) : 0;  // URLlength, URLstring[URLlength]
  m_OCRstreamFlag ? sizeValue += 2 : 0;            // OCR_ES_Id

  sizeValue += 1;  // dcd tag
  sizeValue += (m_dcd.size() / MAX_SIZE_IN_ONE_BYTE) +
               ((m_dcd.size() % MAX_SIZE_IN_ONE_BYTE) ? 1 : 0);  // dcd size
  sizeValue += m_dcd.size();                                     // dcd payload

  sizeValue += 1;  // slConfigDescriptor tag
  sizeValue +=
      (m_slConfigDescriptor.size() / MAX_SIZE_IN_ONE_BYTE) +
      ((m_slConfigDescriptor.size() % MAX_SIZE_IN_ONE_BYTE) ? 1 : 0);  // slconfigDescriptor size
  sizeValue += m_slConfigDescriptor.size();                            // slConfigDescriptor payload

  CBaseDescriptor::updateSize(sizeValue);
}

SAttributeList CESDescriptor::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "ES_ID";
  attribute.value = std::to_string(m_ES_id);
  attributesList.push_back(attribute);

  attribute.key = "Stream Dependence Flag";
  attribute.value = std::to_string(m_streamDependenceFlag);
  attributesList.push_back(attribute);

  attribute.key = "URL Flag";
  attribute.value = std::to_string(m_URLflag);
  attributesList.push_back(attribute);

  attribute.key = "OCRstream Flag";
  attribute.value = std::to_string(m_OCRstreamFlag);
  attributesList.push_back(attribute);

  attribute.key = "Stream Priority";
  attribute.value = std::to_string(m_streamPriority);
  attributesList.push_back(attribute);

  if (m_streamDependenceFlag) {
    attribute.key = "Depends On_ES_ID";
    attribute.value = std::to_string(m_dependsOn_ES_ID);
    attributesList.push_back(attribute);
  }

  if (m_URLflag) {
    attribute.key = "URL Length";
    attribute.value = std::to_string(m_URLlength);
    attributesList.push_back(attribute);

    attribute.key = "URL String";
    std::stringstream st;
    st << "[";
    for (size_t i = 0; i < m_URLstring.size(); ++i) {
      st << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
         << static_cast<int>(m_URLstring[i]);

      if (i < (m_URLstring.size() - 1)) {
        st << ", ";
      }
    }
    st << "]";

    attribute.value = st.str();
    attributesList.push_back(attribute);
  }

  if (m_OCRstreamFlag) {
    attribute.key = "OCR_ES_Id";
    attribute.value = std::to_string(m_OCR_ES_Id);
    attributesList.push_back(attribute);
  }

  attribute.key = "Decoder Config Descriptor";
  std::stringstream ss;
  auto descList = m_dcd.getAttributeList();

  ss << "{";
  for (size_t i = 0; i < descList.size(); ++i) {
    ss << descList[i].key << ": " << descList[i].value;

    if (i != descList.size() - 1) {
      ss << ", ";
    }
  }
  ss << "}";

  attribute.value = ss.str();
  attributesList.push_back(attribute);

  attribute.key = "Sl Config Descriptor";
  std::stringstream sl;
  auto slList = m_slConfigDescriptor.getAttributeList();

  sl << "{";
  for (size_t i = 0; i < slList.size(); ++i) {
    sl << slList[i].key << ": " << slList[i].value;

    if (i != slList.size() - 1) {
      sl << ", ";
    }
  }
  sl << "}";

  attribute.value = sl.str();
  attributesList.push_back(attribute);

  if (!m_remainingPayload.empty()) {
    attribute.key = "Remaining Payload";
    std::stringstream rp;
    for (auto& byte : m_remainingPayload) {
      rp << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
         << static_cast<int>(byte);
      rp << " ";
    }
    attribute.value = rp.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CESDescriptor::parse(ilo::ByteBuffer::const_iterator& begin,
                          const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::ESDescriptor, std::invalid_argument,
                  "CESDescriptor: tag is %d and it should be %d", static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::ESDescriptor));

  ILO_ASSERT_WITH(size() <= (static_cast<size_t>(end - begin)), std::logic_error,
                  "CESDescriptor: not enough data in buffer");

  m_ES_id = readUint16(begin, end);

  uint8_t tmp_byte = readUint8(begin, end);
  m_streamDependenceFlag = (tmp_byte & 0x80) >> 7;
  m_URLflag = (tmp_byte & 0x40) >> 6;
  m_OCRstreamFlag = (tmp_byte & 0x20) >> 5;
  m_streamPriority = (tmp_byte & 0x1F);

  if (m_streamDependenceFlag) {
    m_dependsOn_ES_ID = readUint16(begin, end);
  }

  if (m_URLflag) {
    m_URLlength = readUint8(begin, end);
    if (m_URLlength) {
      for (uint8_t i = 0; i < m_URLlength; i++) {
        m_URLstring.push_back(readUint8(begin, end));
      }
    }
  }

  if (m_OCRstreamFlag) {
    m_OCR_ES_Id = readUint16(begin, end);
  }

  // parse decoder config descriptor
  m_dcd = CDecoderConfigDescriptor(begin, end);

  // parse SL Config Descriptor
  m_slConfigDescriptor = CSLConfigDescriptor(begin, end);

  // parsing other payload that could be in the buffer e.g some other descriptors.
  if (begin != end) {
    m_remainingPayload = ilo::ByteBuffer(begin, end);
    begin = end;
  }
}

void CESDescriptor::writeDescriptor(ilo::ByteBuffer& buffer,
                                    ilo::ByteBuffer::iterator& position) const {
  ILO_ASSERT_WITH(tag() == EDescriptorTag::ESDescriptor, std::invalid_argument,
                  "CESDescriptor: tag is %d and it should be %d", static_cast<uint8_t>(tag()),
                  static_cast<uint8_t>(EDescriptorTag::ESDescriptor));

  ILO_ASSERT_WITH(static_cast<size_t>(buffer.end() - position) >= size(), std::logic_error,
                  "CESDescriptor: not enough space in buffer");

  writeUint16(buffer, position, m_ES_id);

  uint8_t tmpByte = 0;
  tmpByte = static_cast<uint8_t>(m_streamDependenceFlag << 7);
  tmpByte |= (m_URLflag << 6);
  tmpByte |= (m_OCRstreamFlag << 5);
  tmpByte |= m_streamPriority;

  writeUint8(buffer, position, tmpByte);

  if (m_streamDependenceFlag) {
    writeUint16(buffer, position, m_dependsOn_ES_ID);
  }

  if (m_URLflag) {
    writeUint8(buffer, position, m_URLlength);
    for (uint8_t i = 0; i < m_URLlength; i++) {
      writeUint8(buffer, position, m_URLstring[i]);
    }
  }

  if (m_OCRstreamFlag) {
    writeUint16(buffer, position, m_OCR_ES_Id);
  }

  m_dcd.write(buffer, position);
  m_slConfigDescriptor.write(buffer, position);

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
