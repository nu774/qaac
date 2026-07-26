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
 * Content: loudness box class
 */

// System headers
#include <algorithm>

// External headers
#include "ilo/bytebuffertools.h"
#include "ilo/string_utils.h"

// Internal headers
#include "loudnessbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CLoudnessBaseBox::CLoudnessBaseBox(ilo::ByteBuffer::const_iterator& begin,
                                   const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end) {
  parseBox(begin, end);
}

CLoudnessBaseBox::CLoudnessBaseBox(const SLoudnessWriteConfig& loudnessWriteConfig)
    : CFullBox(loudnessWriteConfig), m_loudnessBaseSets(loudnessWriteConfig.loudnessBaseSets) {
  ILO_ASSERT_WITH(loudnessWriteConfig.type == ilo::toFcc("tlou") ||
                      loudnessWriteConfig.type == ilo::toFcc("alou"),
                  std::invalid_argument, "Expected box type tlou or alou, but found: %s",
                  ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(
      loudnessWriteConfig.loudnessBaseSets.size() <= 63U, std::invalid_argument,
      "Nr of loudness base sets exceeds the limits of 6bit");  // 63U == 00111111 (6bit uint)

  if (CFullBox::version() == 0) {
    ILO_ASSERT_WITH(loudnessWriteConfig.loudnessBaseSets.size() == 1, std::invalid_argument,
                    "Loudnes Base Count must be 1 for box version 0");
  }

  for (const auto& lbs : loudnessWriteConfig.loudnessBaseSets) {
    ILO_ASSERT_WITH(lbs.measurementSets.size() <= std::numeric_limits<uint32_t>::max(),
                    std::invalid_argument, "Nr of measurement sets exceeds the limits of 8bit");

    if (CFullBox::version() == 0) {
      ILO_ASSERT_WITH(lbs.eqSetId == 0, std::invalid_argument,
                      "EQ Set ID cannot be set for box version 0");
    }
  }
  updateSize(0);
}

void CLoudnessBaseBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("tlou") || CBox::type() == ilo::toFcc("alou"),
                  std::invalid_argument, "Expected box type tlou or alou, but found: %s",
                  ilo::toString(CBox::type()).c_str());

  uint8_t loudnessBaseCount = 1;

  if (CFullBox::version() >= 1) {
    auto tmp = ilo::readUint8(begin, end);
    loudnessBaseCount = tmp & 0x3F;
  }

  for (uint8_t i = 1; i <= loudnessBaseCount; ++i) {
    SLoudnessBaseSet lbSet;
    if (CFullBox::version() >= 1) {
      auto tmp = ilo::readUint8(begin, end);
      lbSet.eqSetId = tmp & 0x3F;
    }

    auto tmp1 = ilo::readUint16(begin, end);
    lbSet.downmixId = static_cast<uint8_t>((tmp1 & 0x1FC0u) >> 6U);
    lbSet.drcSetId = static_cast<uint8_t>(tmp1 & 0x7Fu);

    auto tmp2 = ilo::readInt32(begin, end);
    lbSet.bsSamplePeakLevel = static_cast<int16_t>(tmp2 >> 20);
    lbSet.bsTruePeakLevel = static_cast<int16_t>(((tmp2 & 0xFFFFF) << 12) >> 20);
    lbSet.measurementSystemForTp = (static_cast<uint8_t>(tmp2 & 0xF0)) >> 4;
    lbSet.reliabilityForTp = static_cast<uint8_t>(tmp2 & 0x0F);

    auto measurementCount = ilo::readUint8(begin, end);
    for (auto j = 1; j <= measurementCount; ++j) {
      SMeasurementSet set;
      set.methodDefinition = ilo::readUint8(begin, end);
      set.methodValue = ilo::readUint8(begin, end);
      auto tmp = ilo::readUint8(begin, end);
      set.measurementSystem = (tmp & 0xF0) >> 4;
      set.reliability = tmp & 0xF;
      lbSet.measurementSets.push_back(set);
    }
    m_loudnessBaseSets.push_back(lbSet);
  }
}

void CLoudnessBaseBox::updateSize(uint64_t sizeValue) {
  uint64_t payloadSize = 0;

  if (CFullBox::version() >= 1) {
    payloadSize += 1;  // reserved + LoudnessBaseCount
    payloadSize += static_cast<uint64_t>(m_loudnessBaseSets.size() * 1);  // reserved + EqSetId
  }

  for (const auto& lbSet : m_loudnessBaseSets) {
    payloadSize += 7;
    payloadSize += lbSet.measurementSets.size() * 3;
  }

  CFullBox::updateSize(sizeValue + payloadSize);
}

SAttributeList CLoudnessBaseBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Loudness Base Sets";
  std::stringstream ss;
  for (const auto& loudness : m_loudnessBaseSets) {
    ss << "{Bs Sample Peak Level: " << (std::to_string(loudness.bsSamplePeakLevel)) << ", "
       << "Bs True Peak Level: " << (std::to_string(loudness.bsTruePeakLevel)) << ", "
       << "Downmix Id: " << (std::to_string(loudness.downmixId)) << ", "
       << "Drc Set Id: " << (std::to_string(loudness.drcSetId)) << ", "
       << "Eq Set Id: " << (std::to_string(loudness.eqSetId)) << ", "
       << "Measurement System For Tp: " << (std::to_string(loudness.measurementSystemForTp)) << ", "
       << "Reliability For Tp: " << (std::to_string(loudness.reliabilityForTp)) << ", "
       << "Measurement Sets: {";

    size_t index = 0;
    for (const auto& measurement : loudness.measurementSets) {
      ss << "Measurement System: " << (std::to_string(measurement.measurementSystem)) << ", "
         << "Method Definition: " << (std::to_string(measurement.methodDefinition)) << ", "
         << "Method Value: " << (std::to_string(measurement.methodValue)) << ", "
         << "Reliability: " << (std::to_string(measurement.reliability));

      index++;

      if (index < loudness.measurementSets.size()) {
        ss << "; ";
      } else {
        ss << "}";
      }
    }
    ss << "}";
  }

  attribute.value = ss.str();
  attributesList.push_back(attribute);

  return attributesList;
}

void CLoudnessBaseBox::writeBox(ilo::ByteBuffer& buffer,
                                ilo::ByteBuffer::iterator& position) const {
  if (CFullBox::version() >= 1) {
    ilo::writeUint8(buffer, position, static_cast<uint8_t>(m_loudnessBaseSets.size()));
  }

  for (const auto& lbs : m_loudnessBaseSets) {
    if (CFullBox::version() >= 1) {
      ilo::writeUint8(buffer, position, lbs.eqSetId);
    }

    uint32_t tmp = lbs.downmixId;
    tmp = (tmp << 6) | lbs.drcSetId;
    ilo::writeUint16(buffer, position, static_cast<uint16_t>(tmp));

    int32_t tmp2 = lbs.bsSamplePeakLevel;
    tmp2 = (tmp2 << 12) | lbs.bsTruePeakLevel;
    tmp2 = (tmp2 << 4) | lbs.measurementSystemForTp;
    tmp2 = (tmp2 << 4) | lbs.reliabilityForTp;
    ilo::writeInt32(buffer, position, tmp2);

    ilo::writeUint8(buffer, position, static_cast<uint8_t>(lbs.measurementSets.size()));

    for (const auto& ms : lbs.measurementSets) {
      ilo::writeUint8(buffer, position, ms.methodDefinition);
      ilo::writeUint8(buffer, position, ms.methodValue);

      uint32_t tmp3 = ms.measurementSystem;
      tmp3 = (tmp3 << 4) | ms.reliability;
      ilo::writeUint8(buffer, position, static_cast<uint8_t>(tmp3));
    }
  }
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"
#include <memory>

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_FUNCTIONS(CLoudnessBaseBox, CLoudnessBaseBox::SLoudnessWriteConfig);
BOXREGISTRY_REGISTER_FOURCC(tlou, CContainerType::noContainer);
BOXREGISTRY_REGISTER_FOURCC(alou, CContainerType::noContainer);
