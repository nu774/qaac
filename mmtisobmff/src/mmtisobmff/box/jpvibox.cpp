/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2019 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * Content: jegep xs video information box class
 */

// System headers
#include <stdexcept>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "jpvibox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CJPEGXSVideoInformationBox::CJPEGXSVideoInformationBox(ilo::ByteBuffer::const_iterator& begin,
                                                       const ilo::ByteBuffer::const_iterator& end)
    : CBox(begin, end) {
  parse(begin, end);
}

CJPEGXSVideoInformationBox::CJPEGXSVideoInformationBox(
    const CJPEGXSVideoInformationBox::SJPEGXSVideoInformationBoxWriteConfig& cjxsmBoxData)
    : CBox(cjxsmBoxData) {
  m_brat = cjxsmBoxData.brat;
  m_frat = cjxsmBoxData.frat;
  m_schar = cjxsmBoxData.schar;
  m_tcod = cjxsmBoxData.tcod;
  updateSize(0);
  verify();
}

void CJPEGXSVideoInformationBox::parse(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("jpvi"), std::invalid_argument,
                  "Expected box type jpvi, but found: %s while parsing",
                  ilo::toString(CBox::type()).c_str());

  m_brat = ilo::readUint32(begin, end);
  if (m_brat == 0) {
    ILO_LOG_WARNING("brat in jpvi is 0 (Maximal video bitrate)");
  }
  m_frat = ilo::readUint32(begin, end);
  m_schar = ilo::readUint16(begin, end);
  m_tcod = ilo::readUint32(begin, end);

  uint32_t hours = (m_tcod & 0xff000000) >> 24;
  uint32_t minutes = (m_tcod & 0x00ff0000) >> 16;
  uint32_t seconds = (m_tcod & 0x0000ff00) >> 8;
  uint32_t frames = (m_tcod & 0x000000ff);
  if (frames == 0 || frames > 60) {
    ILO_LOG_WARNING("frames (from tcod in jpvi) is not in valid range, it is: %d", frames);
  }
  if (seconds > 59) {
    ILO_LOG_WARNING("seconds (from tcod in jpvi) is not in valid range, it is: %d", seconds);
  }
  if (minutes > 59) {
    ILO_LOG_WARNING("minutes (from tcod in jpvi) is not in valid range, it is: %d", minutes);
  }
  if (hours > 23) {
    ILO_LOG_WARNING("hours (from tcod in jpvi) is not in valid range, it is: %d", hours);
  }
}

void CJPEGXSVideoInformationBox::writeBox(ilo::ByteBuffer& buffer,
                                          ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint32(buffer, position, m_brat);
  ilo::writeUint32(buffer, position, m_frat);
  ilo::writeUint16(buffer, position, m_schar);
  ilo::writeUint32(buffer, position, m_tcod);
}

void CJPEGXSVideoInformationBox::updateSize(uint64_t sizeValue) {
  // size + data
  ILO_ASSERT_WITH(sizeValue == 0, std::invalid_argument,
                  "There are boxes inside of the jpvi box, so updateSize is not allowed to be "
                  "called with different than 0");
  CBox::updateSize(sizeValue + 4 + 4 + 2 + 4);  // brat(32) + frat(32) + schar(16) + tcod(32)
}

SAttributeList CJPEGXSVideoInformationBox::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Maximal video bitrate";
  attribute.value = std::to_string(m_brat);
  attributesList.push_back(attribute);

  uint32_t interlaceMode = (m_frat & 0xc0000000) >> 30;
  uint32_t framerateDenominator = (m_frat & 0x3f000000) >> 24;
  uint32_t framerateReserved = (m_frat & 0x00ff0000) >> 16;
  uint32_t framerateNumerator = (m_frat & 0x0000ffff);

  attribute.key = "Interlace_mode";
  attribute.value = std::to_string(interlaceMode);
  switch (interlaceMode) {
    case 0:
      attribute.value += " [Progressive frame (frame contains one full-height picture)]";
      break;
    case 1:
      attribute.value += " [Interlaced frame (picture is first video field)]";
      break;
    case 2:
      attribute.value += " [Interlaced frame (picture is second video field)]";
      break;
    default:
      attribute.value += " [Reserved]";
      break;
  }

  attributesList.push_back(attribute);

  uint16_t validFlag = (m_schar & 0x8000) >> 15;
  uint16_t sampleReserved = (m_schar & 0x7f00) >> 8;
  uint16_t sampleBitdepth = (m_schar & 0x00f0) >> 4;
  uint16_t samplingStructure = (m_schar & 0x000f);

  attribute.key = "Framerate Denominator";
  attribute.value = std::to_string(framerateDenominator);
  switch (framerateDenominator) {
    case 1:
      attribute.value += " [denominator value is 1.000]";
      break;
    case 2:
      attribute.value += " [denominator value is 1.001]";
      break;
    default:
      attribute.value += " [Reserved]";
      break;
  }
  attributesList.push_back(attribute);

  attribute.key = "Framerate Reserved";
  attribute.value = std::to_string(framerateReserved);
  attributesList.push_back(attribute);

  attribute.key = "Framerate Numerator";
  attribute.value = std::to_string(framerateNumerator);
  attributesList.push_back(attribute);

  attribute.key = "Valid Flag";
  attribute.value = std::to_string(validFlag);
  attributesList.push_back(attribute);

  attribute.key = "Sample Reserved";
  attribute.value = std::to_string(sampleReserved);
  attributesList.push_back(attribute);

  attribute.key = "Sample Bitdepth";
  attribute.value = std::to_string(sampleBitdepth);
  attributesList.push_back(attribute);

  attribute.key = "Sampling Structure";
  attribute.value = std::to_string(samplingStructure);
  switch (samplingStructure) {
    case 0:
      attribute.value += " [4:2:2 (YCbCr)]";
      break;
    case 1:
      attribute.value += " [4:4:4 (YCbCr)]";
      break;
    case 2:
      attribute.value += " [4:4:4 (RGB)]";
      break;
    case 4:
      attribute.value += " [4:2:2:4 (YCbCrAux)]";
      break;
    case 5:
      attribute.value += " [4:4:4:4 (YCbCrAux)]";
      break;
    case 6:
      attribute.value += " [4:4:4:4 (RGBAux)]";
      break;
    default:
      attribute.value += " [Reserved]";
      break;
  }
  attributesList.push_back(attribute);

  uint32_t hours = (m_tcod & 0xff000000) >> 24;
  uint32_t minutes = (m_tcod & 0x00ff0000) >> 16;
  uint32_t seconds = (m_tcod & 0x0000ff00) >> 8;
  uint32_t frames = (m_tcod & 0x000000ff);

  attribute.key = "Time";
  attribute.value = "";
  if (std::to_string(hours).length() < 2) {
    attribute.value += "0";
  }
  attribute.value += std::to_string(hours);
  if (std::to_string(minutes).length() < 2) {
    attribute.value += "0";
  }
  attribute.value += std::to_string(minutes);
  if (std::to_string(seconds).length() < 2) {
    attribute.value += "0";
  }
  attribute.value += std::to_string(seconds);
  if (std::to_string(frames).length() < 2) {
    attribute.value += "0";
  }
  attribute.value += std::to_string(frames) + " [HHMMSSFF]";
  attributesList.push_back(attribute);

  return attributesList;
}

uint32_t CJPEGXSVideoInformationBox::brat() const {
  return m_brat;
}

uint32_t CJPEGXSVideoInformationBox::frat() const {
  return m_frat;
}

uint16_t CJPEGXSVideoInformationBox::schar() const {
  return m_schar;
}

uint32_t CJPEGXSVideoInformationBox::tcod() const {
  return m_tcod;
}

void CJPEGXSVideoInformationBox::verify() const {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("jpvi"), std::invalid_argument,
                  "Expected box type jpvi, but found: %s", ilo::toString(CBox::type()).c_str());
  ILO_ASSERT_WITH(m_brat > 0, std::invalid_argument,
                  "Maximum bitrate of video stream is expected to be bigger than 0",
                  ilo::toString(CBox::type()).c_str());

  uint32_t hours = (m_tcod & 0xff000000) >> 24;
  uint32_t minutes = (m_tcod & 0x00ff0000) >> 16;
  uint32_t seconds = (m_tcod & 0x0000ff00) >> 8;
  uint32_t frames = (m_tcod & 0x000000ff);

  ILO_ASSERT_WITH(frames >= 1 && frames <= 60, std::invalid_argument,
                  "Number for frames has to be in range from 1 to 60",
                  ilo::toString(CBox::type()).c_str());
  ILO_ASSERT_WITH(seconds <= 59, std::invalid_argument,
                  "Maximum number for seconds in time code is 59",
                  ilo::toString(CBox::type()).c_str());
  ILO_ASSERT_WITH(minutes <= 59, std::invalid_argument,
                  "Maximum number for minutes in time code is 59",
                  ilo::toString(CBox::type()).c_str());
  ILO_ASSERT_WITH(hours <= 23, std::invalid_argument, "Maximum number for hours in time code is 23",
                  ilo::toString(CBox::type()).c_str());
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(jpvi, CJPEGXSVideoInformationBox,
                    CJPEGXSVideoInformationBox::SJPEGXSVideoInformationBoxWriteConfig,
                    CContainerType::noContainer);
