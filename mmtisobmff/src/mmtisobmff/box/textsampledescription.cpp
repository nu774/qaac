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
#include "textsampledescription.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CTextSampleDescription::CTextSampleDescription(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end)
    : CSampleEntry(begin, end) {
  parseBox(begin, end);
}

CTextSampleDescription::CTextSampleDescription(const STextWriteConfig& config)
    : box::CSampleEntry(config),
      m_displayFlags(config.displayFlags),
      m_textJustification(config.textJustification),
      m_backgroundColor(config.backgroundColor),
      m_defaultTextBox(config.defaultTextBox),
      m_reserved1(0),
      m_fontNumber(config.fontNumber),
      m_fontFace(config.fontFace),
      m_reserved2(0),
      m_reserved3(0),
      m_foregroundColor(config.foregroundColor) {
  updateSize(0);
}

void CTextSampleDescription::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                  const ilo::ByteBuffer::const_iterator& end) {
  m_displayFlags = ilo::readUint32(begin, end);
  m_textJustification = ilo::readUint32(begin, end);
  for (size_t i = 0; i < m_backgroundColor.size(); ++i) {
    m_backgroundColor[i] = ilo::readUint16(begin, end);
  }
  for (size_t i = 0; i < m_defaultTextBox.size(); ++i) {
    m_defaultTextBox[i] = ilo::readUint16(begin, end);
  }
  m_reserved1 = ilo::readUint64(begin, end);
  m_fontNumber = ilo::readUint16(begin, end);
  m_fontFace = ilo::readUint16(begin, end);
  m_reserved2 = ilo::readUint8(begin, end);
  m_reserved3 = ilo::readUint16(begin, end);
  for (size_t i = 0; i < m_foregroundColor.size(); ++i) {
    m_foregroundColor[i] = ilo::readUint16(begin, end);
  }
  sanityCheck();
}

void CTextSampleDescription::sanityCheck() const {
}

void CTextSampleDescription::updateSize(uint64_t sizeValue) {
  CSampleEntry::updateSize(sizeValue + 4 + 4 + 6 + 8 + 8 + 2 + 2 + 1 + 2 + 6);
}

void CTextSampleDescription::writeHeader(ilo::ByteBuffer& buffer,
                                         ilo::ByteBuffer::iterator& position) const {
  box::CSampleEntry::writeHeader(buffer, position);

  ilo::writeUint32(buffer, position, m_displayFlags);
  ilo::writeUint32(buffer, position, m_textJustification);
  for (size_t i = 0; i < m_backgroundColor.size(); ++i) {
    ilo::writeUint16(buffer, position, m_backgroundColor[i]);
  }
  for (size_t i = 0; i < m_defaultTextBox.size(); ++i) {
    ilo::writeUint16(buffer, position, m_defaultTextBox[i]);
  }
  ilo::writeUint64(buffer, position, m_reserved1);
  ilo::writeUint16(buffer, position, m_fontNumber);
  ilo::writeUint16(buffer, position, m_fontFace);
  ilo::writeUint8(buffer, position, m_reserved2);
  ilo::writeUint16(buffer, position, m_reserved3);
  for (size_t i = 0; i < m_foregroundColor.size(); ++i) {
    ilo::writeUint16(buffer, position, m_foregroundColor[i]);
  }
}

void CTextSampleDescription::writeBox(ilo::ByteBuffer& /*buffer*/,
                                      ilo::ByteBuffer::iterator& /*position*/) const {}

SAttributeList CTextSampleDescription::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "DisplayFlags";
  attribute.value = std::to_string(m_displayFlags);
  attributesList.push_back(attribute);

  attribute.key = "TextJustification";
  attribute.value = std::to_string(m_textJustification);
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

  attribute.key = "FontNumber";
  attribute.value = std::to_string(m_fontNumber);
  attributesList.push_back(attribute);

  attribute.key = "FontFace";
  attribute.value = std::to_string(m_fontFace);
  attributesList.push_back(attribute);

  attribute.key = "ForegroundColor";
  std::stringstream ss3;
  for (auto element : m_foregroundColor) {
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

BOXREGISTRY_FUNCTIONS(CTextSampleDescription, CTextSampleDescription::STextWriteConfig);
BOXREGISTRY_REGISTER_FOURCC_FCC(text, "text", CContainerType::isContainer);
