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
 * Content: color information box class
 */

// System headers
#include <stdexcept>
#include <algorithm>

// External headers
#include "ilo/bytebuffertools.h"
#include "ilo/string_utils.h"

// Internal headers
#include "colrbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CColourInformationBox::CColourInformationBox(ilo::ByteBuffer::const_iterator& begin,
                                             const ilo::ByteBuffer::const_iterator& end)
    : CBox(begin, end) {
  parse(begin, end);
}

CColourInformationBox::CColourInformationBox(
    const CColourInformationBox::SColourInformationBoxWriteConfig& colrBoxData)
    : CBox(colrBoxData) {
  std::vector<ilo::Fourcc> validColourTypes = {ilo::toFcc("nclx"), ilo::toFcc("rICC"),
                                               ilo::toFcc("prof")};
  ILO_ASSERT_WITH(std::find(validColourTypes.begin(), validColourTypes.end(),
                            colrBoxData.colourType) != validColourTypes.end(),
                  std::invalid_argument, "Unknown colour type.");

  m_colourType = colrBoxData.colourType;
  if (m_colourType == ilo::toFcc("nclx")) {
    ILO_ASSERT_WITH(colrBoxData.iccProfile.size() == 0, std::invalid_argument,
                    "For the 'nclx' mode no icc profile can be set.");
    m_colourPrimaries = colrBoxData.colourPrimaries;
    m_transferCharacteristics = colrBoxData.transferCharacteristics;
    m_matrixCoefficients = colrBoxData.matrixCoefficients;
    m_fullRangeFlag = colrBoxData.fullRangeFlag;
  } else {
    ILO_ASSERT_WITH(colrBoxData.iccProfile.size() > 0, std::invalid_argument,
                    "For the 'nclx' mode a icc profile must be set.");
    m_iccProfile = colrBoxData.iccProfile;
  }

  updateSize(0);
}

void CColourInformationBox::parse(ilo::ByteBuffer::const_iterator& begin,
                                  const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("colr"), std::invalid_argument,
                  "Expected box type colr, but found: %s", ilo::toString(CBox::type()).c_str());
  m_colourType = ilo::readFourCC(begin, end);
  ;

  if (m_colourType == ilo::toFcc("nclx")) {
    m_colourPrimaries = ilo::readUint16(begin, end);
    m_transferCharacteristics = ilo::readUint16(begin, end);
    m_matrixCoefficients = ilo::readUint16(begin, end);
    uint8_t tmp = ilo::readUint8(begin, end);
    m_fullRangeFlag = (tmp & 0x80) > 0;
  } else if (m_colourType == ilo::toFcc("rICC") || m_colourType == ilo::toFcc("prof")) {
    size_t iccProfileSize =
        static_cast<size_t>(size()) - 8 - 4;  // -8: Box header(size + fcc) -4: colour_type
    if (had64BitSizeInInput()) {
      iccProfileSize -= 8;  // for 64 bit size in Box header
    }

    ILO_ASSERT(static_cast<uint64_t>(end - begin) >= iccProfileSize,
               "Not enough data to read ICC profile.");
    m_iccProfile = ilo::ByteBuffer(begin, begin + iccProfileSize);
    begin += iccProfileSize;
  } else {
    ILO_ASSERT_WITH(false, std::invalid_argument, "Unknown colour type found.");
  }
}

void CColourInformationBox::writeBox(ilo::ByteBuffer& buffer,
                                     ilo::ByteBuffer::iterator& position) const {
  ilo::writeFourCC(buffer, position, m_colourType);
  if (m_colourType == ilo::toFcc("nclx")) {
    ilo::writeUint16(buffer, position, m_colourPrimaries);
    ilo::writeUint16(buffer, position, m_transferCharacteristics);
    ilo::writeUint16(buffer, position, m_matrixCoefficients);
    ilo::writeUint8(buffer, position, (m_fullRangeFlag ? 1 : 0) << 7);
  } else {
    ilo::writeUint8Array(buffer, position, m_iccProfile);
  }
}

void CColourInformationBox::updateSize(uint64_t sizeValue) {
  if (m_colourType == ilo::toFcc("nclx")) {
    CBox::updateSize(sizeValue + 11);
  } else {
    CBox::updateSize(sizeValue + 4 + m_iccProfile.size());
  }
}

SAttributeList CColourInformationBox::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Colour Type";
  attribute.value = ilo::toString(m_colourType);
  attributesList.push_back(attribute);

  if (m_colourType == ilo::toFcc("nclx")) {
    attribute.key = "Colour Primaries";
    attribute.value = std::to_string(m_colourPrimaries);
    attributesList.push_back(attribute);

    attribute.key = "Transfer Characteristics";
    attribute.value = std::to_string(m_transferCharacteristics);
    attributesList.push_back(attribute);

    attribute.key = "Matrix Coefficients";
    attribute.value = std::to_string(m_matrixCoefficients);
    attributesList.push_back(attribute);

    attribute.key = "Full Range Flag";
    attribute.value = m_fullRangeFlag ? "True" : "False";
    attributesList.push_back(attribute);
  } else {
    attribute.key = "ICC Profile length";
    attribute.value = std::to_string(m_iccProfile.size());
    attributesList.push_back(attribute);
  }

  return attributesList;
}

ilo::Fourcc CColourInformationBox::colourType() {
  return m_colourType;
}

bool CColourInformationBox::hasColourPrimaries() {
  return m_colourType == ilo::toFcc("nclx");
}

uint16_t CColourInformationBox::colourPrimaries() {
  ILO_ASSERT(hasColourPrimaries(), "Colour primaries is not set.");
  return m_colourPrimaries;
}

bool CColourInformationBox::hasTransferCharacteristics() {
  return m_colourType == ilo::toFcc("nclx");
}

uint16_t CColourInformationBox::transferCharacteristics() {
  ILO_ASSERT(hasTransferCharacteristics(), "Transfer Characteristics is not set.");
  return m_transferCharacteristics;
}

bool CColourInformationBox::hasMatrixCoefficients() {
  return m_colourType == ilo::toFcc("nclx");
}

uint16_t CColourInformationBox::matrixCoefficients() {
  ILO_ASSERT(hasMatrixCoefficients(), "Matrix Coefficients is not set.");
  return m_matrixCoefficients;
}

bool CColourInformationBox::hasFullRangeFlag() {
  return m_colourType == ilo::toFcc("nclx");
}

bool CColourInformationBox::fullRangeFlag() {
  ILO_ASSERT(hasFullRangeFlag(), "Full Range Flag is not defined.");
  return m_fullRangeFlag;
}

bool CColourInformationBox::hasIccProfile() {
  return m_colourType == ilo::toFcc("rICC") || m_colourType == ilo::toFcc("prof");
}
ilo::ByteBuffer CColourInformationBox::iccProfile() {
  ILO_ASSERT(hasIccProfile(), "ICC Profile is not set.");
  return m_iccProfile;
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(colr, CColourInformationBox,
                    CColourInformationBox::SColourInformationBoxWriteConfig,
                    CContainerType::noContainer);
