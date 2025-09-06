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
#include "timedtextdescription.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CTimedTextDescription::CTimedTextDescription(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end)
    : CSampleEntry(begin, end),
      m_displayFlags(0),
      m_horizontalJustification(0),
      m_verticalJustification(0),
      m_backgroundColor{},
      m_defaultTextBox{},
      m_startChar(0),
      m_endChar(0),
      m_fontId(0),
      m_faceStyleFlags(0),
      m_fontSize(0),
      m_textColor{} {
  parseBox(begin, end);
}

CTimedTextDescription::CTimedTextDescription(const STextWriteConfig& config)
    : box::CSampleEntry(config),
      m_displayFlags(config.displayFlags),
      m_horizontalJustification(config.horizontalJustification),
      m_verticalJustification(config.verticalJustification),
      m_defaultTextBox(config.defaultTextBox),
      m_startChar(config.startChar),
      m_endChar(config.endChar),
      m_fontId(config.fontId),
      m_faceStyleFlags(config.faceStyleFlags),
      m_fontSize(config.fontSize),
      m_textColor(config.textColor) {
  updateSize(0);
}

void CTimedTextDescription::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                  const ilo::ByteBuffer::const_iterator& end) {
  m_displayFlags = ilo::readUint32(begin, end);
  m_horizontalJustification = ilo::readUint8(begin, end);
  m_verticalJustification = ilo::readUint8(begin, end);
  for (size_t i = 0; i < m_backgroundColor.size(); ++i) {
    m_backgroundColor[i] = ilo::readUint8(begin, end);
  }
  for (size_t i = 0; i < m_defaultTextBox.size(); ++i) {
    m_defaultTextBox[i] = ilo::readUint16(begin, end);
  }
  m_startChar = ilo::readUint16(begin, end);
  m_endChar = ilo::readUint16(begin, end);
  m_fontId = ilo::readUint16(begin, end);
  m_faceStyleFlags = ilo::readUint8(begin, end);
  m_fontSize = ilo::readUint8(begin, end);
  for (size_t i = 0; i < m_textColor.size(); ++i) {
    m_textColor[i] = ilo::readUint8(begin, end);
  }
}

void CTimedTextDescription::updateSize(uint64_t sizeValue) {
  CSampleEntry::updateSize(sizeValue + 4 + 4 + 6 + 8 + 8 + 2 + 2 + 1 + 2 + 6);
}

void CTimedTextDescription::writeHeader(ilo::ByteBuffer& buffer,
                                         ilo::ByteBuffer::iterator& position) const {
  box::CSampleEntry::writeHeader(buffer, position);

  ilo::writeUint32(buffer, position, m_displayFlags);
  ilo::writeUint8(buffer, position, m_horizontalJustification);
  ilo::writeUint8(buffer, position, m_verticalJustification);
  for (size_t i = 0; i < m_backgroundColor.size(); ++i) {
    ilo::writeUint8(buffer, position, m_backgroundColor[i]);
  }
  for (size_t i = 0; i < m_defaultTextBox.size(); ++i) {
    ilo::writeUint16(buffer, position, m_defaultTextBox[i]);
  }
  ilo::writeUint16(buffer, position, m_startChar);
  ilo::writeUint16(buffer, position, m_endChar);
  ilo::writeUint16(buffer, position, m_fontId);
  ilo::writeUint8(buffer, position, m_faceStyleFlags);
  ilo::writeUint8(buffer, position, m_fontSize);
  for (size_t i = 0; i < m_textColor.size(); ++i) {
    ilo::writeUint16(buffer, position, m_textColor[i]);
  }
}

void CTimedTextDescription::writeBox(ilo::ByteBuffer& /*buffer*/,
                                      ilo::ByteBuffer::iterator& /*position*/) const {}

SAttributeList CTimedTextDescription::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "DisplayFlags";
  attribute.value = std::to_string(m_displayFlags);
  attributesList.push_back(attribute);

  attribute.key = "HorizontalJustification";
  attribute.value = std::to_string(m_horizontalJustification);
  attributesList.push_back(attribute);

  attribute.key = "VerticalJustification";
  attribute.value = std::to_string(m_verticalJustification);
  attributesList.push_back(attribute);

  attribute.key = "BackgroundColor";
  std::stringstream ss;
  for (auto element : m_backgroundColor) {
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase
       << static_cast<int>(element);
    ss << ", ";
  }
  attribute.value = ss.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
  attributesList.push_back(attribute);

  attribute.key = "DefaultTextBox";
  std::stringstream ss2;
  for (auto element : m_backgroundColor) {
    ss2 << static_cast<int>(element);
    ss2 << ", ";
  }
  attribute.value = ss2.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
  attributesList.push_back(attribute);

  attribute.key = "StartChar";
  attribute.value = std::to_string(m_startChar);
  attributesList.push_back(attribute);

  attribute.key = "EndChar";
  attribute.value = std::to_string(m_endChar);
  attributesList.push_back(attribute);

  attribute.key = "FontID";
  attribute.value = std::to_string(m_fontId);
  attributesList.push_back(attribute);

  attribute.key = "FaceStyleFlags";
  attribute.value = std::to_string(m_faceStyleFlags);
  attributesList.push_back(attribute);

  attribute.key = "FontSize";
  attribute.value = std::to_string(m_fontSize);
  attributesList.push_back(attribute);

  attribute.key = "TextColor";
  std::stringstream ss3;
  for (auto element : m_textColor) {
    ss3 << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase
       << static_cast<int>(element);
    ss3 << ", ";
  }
  attribute.value = ss3.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
  attributesList.push_back(attribute);

  attribute.key = "Data Reference Index";
  attribute.value = std::to_string(dataReferenceIndex());
  attributesList.push_back(attribute);

  return attributesList;
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_FUNCTIONS(CTimedTextDescription, CTimedTextDescription::STextWriteConfig);
BOXREGISTRY_REGISTER_FOURCC_FCC(tx3g, "tx3g", CContainerType::isContainer);
