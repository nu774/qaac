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
 * Content: Compact Sample Size Box class
 */

// System headers
#include <limits>
#include <iostream>
#include <math.h>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "stz2box.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CCompactSampleSizeBox::CCompactSampleSizeBox(ilo::ByteBuffer::const_iterator& begin,
                                             const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end), m_sampleCount(0), m_entrySizes() {
  parseBox(begin, end);
}

CCompactSampleSizeBox::CCompactSampleSizeBox(const SStz2BoxWriteConfig& stz2BoxData)
    : CFullBox(stz2BoxData),
      m_sampleCount(static_cast<uint32_t>(stz2BoxData.entrySizes.size())),
      m_entrySizes(stz2BoxData.entrySizes) {
  switch (stz2BoxData.fieldSize) {
    case EFieldSize::fieldSize4:
      m_fieldSize = 4;
      break;

    case EFieldSize::fieldSize8:
      m_fieldSize = 8;
      break;

    case EFieldSize::fieldSize16:
      m_fieldSize = 16;
      break;
  }

  updateSize(0);
}

void CCompactSampleSizeBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("stz2"), std::invalid_argument,
                  "Expected box type stz2, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::version() == 0, std::invalid_argument,
                  "Version %s of stz2 box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the stz2 box");

  begin += 3;  // 3 reserved bytes
  m_fieldSize = ilo::readUint8(begin, end);
  m_sampleCount = ilo::readUint32(begin, end);

  ILO_ASSERT_WITH(
      m_fieldSize == 4 || m_fieldSize == 8 || m_fieldSize == 16, std::invalid_argument,
      "field size can't take the value %u. 4, 8 and 16 are the only possible field size values",
      m_fieldSize);

  auto requiredBytes =
      static_cast<float>(m_sampleCount * static_cast<uint64_t>(m_fieldSize)) / 8.0f;
  ILO_ASSERT_WITH(static_cast<int64_t>(end - begin) >= static_cast<int64_t>(ceil(requiredBytes)),
                  std::out_of_range, "Malformed stz2 box");
  m_entrySizes.resize(m_sampleCount);

  uint16_t temp = 0;
  bool iteratorCorrection = m_fieldSize == 4 ? 1 : 0;
  uint16_t fieldSize4Mask = 240;  // 0000 0000 1111 0000
  uint16_t tmpEntry = 0;
  int8_t numberOfBytes = static_cast<int8_t>(ceil(m_fieldSize / 8.0f));

  for (uint32_t i = 0; i < m_sampleCount; i++) {
    for (int8_t j = (numberOfBytes - 1); j >= 0; --j) {
      temp = static_cast<uint16_t>(ilo::readUint8(begin, end) << (j * 8));
      m_entrySizes[i] += temp;
    }

    if (m_fieldSize == 4) {
      begin -= iteratorCorrection;
      iteratorCorrection = !iteratorCorrection;

      tmpEntry = m_entrySizes[i] & fieldSize4Mask;
      m_entrySizes[i] =
          tmpEntry > 15 ? tmpEntry >> 4
                        : tmpEntry;  // tmpEntry > 15 decides if it is first 4 bits or second 4 bits

      fieldSize4Mask =
          255 - fieldSize4Mask;  // this will change the 4 bits of the mask back and forth
    }
  }
}

SAttributeList CCompactSampleSizeBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Field Size";
  attribute.value = std::to_string(m_fieldSize);
  attributesList.push_back(attribute);

  attribute.key = "Sample Count";
  attribute.value = std::to_string(m_sampleCount);
  attributesList.push_back(attribute);

  if (!m_entrySizes.empty()) {
    attribute.key = "Sample Sizes";
    std::stringstream ss;
    for (auto sSize : m_entrySizes) {
      ss << std::to_string(sSize) << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }
  return attributesList;
}

void CCompactSampleSizeBox::updateSize(uint64_t sizeValue) {
  CFullBox::updateSize(sizeValue + 4 + 4 +
                       static_cast<uint32_t>(ceil(m_entrySizes.size() * m_fieldSize / 8.0f)));
}

void CCompactSampleSizeBox::writeBox(ilo::ByteBuffer& buffer,
                                     ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint24(buffer, position, 0);
  ilo::writeUint8(buffer, position, m_fieldSize);
  ilo::writeUint32(buffer, position, m_sampleCount);

  uint8_t byteToWrite = 0;
  for (size_t i = 0; i < m_entrySizes.size(); ++i) {
    ILO_ASSERT_WITH(m_entrySizes[i] <= static_cast<uint16_t>(pow(2, m_fieldSize) - 1),
                    std::invalid_argument,
                    "entry %u can't be represented with the specified field size (%u bytes).",
                    m_entrySizes[i], m_fieldSize);

    if (m_fieldSize == 4) {
      bool paddingNeeded = i == (m_entrySizes.size() - 1);

      if (paddingNeeded) {
        byteToWrite = static_cast<uint8_t>(m_entrySizes[i] << 4);
      } else {
        ILO_ASSERT(m_entrySizes[i + 1] <= 15,
                   "entry %s can't be represented with the specified field size (4 bytes).",
                   m_entrySizes[i + 1]);
        byteToWrite = static_cast<uint8_t>((m_entrySizes[i] << 4) + m_entrySizes[i + 1]);
      }
      ilo::writeUint8(buffer, position, byteToWrite);
      ++i;
    } else {
      bool condition = !(m_fieldSize == 16);

      do {
        byteToWrite = (m_entrySizes[i] >> (!condition * 8) & 255);
        ilo::writeUint8(buffer, position, (byteToWrite));
        condition = !condition;
      } while (condition);
    }
  }
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(stz2, CCompactSampleSizeBox, CCompactSampleSizeBox::SStz2BoxWriteConfig,
                    CContainerType::noContainer);
