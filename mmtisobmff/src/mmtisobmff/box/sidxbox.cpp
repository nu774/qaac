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
 * Content: segment index box class
 */

// System headers
#include <stdexcept>
#include <limits>
#include <algorithm>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "sidxbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CSegmentIndexBox::CSegmentIndexBox(ilo::ByteBuffer::const_iterator& begin,
                                   const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_referenceId(0),
      m_timescale(0),
      m_earliestPresentationTime(0),
      m_firstOffset(0),
      m_references() {
  parseBox(begin, end);
}

CSegmentIndexBox::CSegmentIndexBox(const SSidxBoxWriteConfig& config)
    : CFullBox(config),
      m_referenceId(config.referenceId),
      m_timescale(config.timescale),
      m_earliestPresentationTime(config.earliestPresentationTime),
      m_firstOffset(config.firstOffset),
      m_references(config.references) {
  if (config.firstOffset > std::numeric_limits<uint32_t>::max() ||
      config.earliestPresentationTime > std::numeric_limits<uint32_t>::max()) {
    CFullBox::updateVersion(1);
  }

  sanityCheck();
  updateSize(0);
}

void CSegmentIndexBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("sidx"), std::invalid_argument,
                  "Expected box type sidx, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the sidx box");

  m_referenceId = ilo::readUint32(begin, end);
  m_timescale = ilo::readUint32(begin, end);

  ILO_ASSERT_WITH(CFullBox::version() == 0 || CFullBox::version() == 1, std::invalid_argument,
                  "Version %s of sidx box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  if (CFullBox::version() == 0) {
    m_earliestPresentationTime = ilo::readUint32(begin, end);
    m_firstOffset = ilo::readUint32(begin, end);
  } else {
    m_earliestPresentationTime = ilo::readUint64(begin, end);
    m_firstOffset = ilo::readUint64(begin, end);
  }

  ILO_ASSERT(ilo::readUint16(begin, end) == 0, "Reserved value in sidx box entry must be zero");

  uint16_t refCount = ilo::readUint16(begin, end);
  for (uint32_t i = 0; i < refCount; ++i) {
    SSidxReference ref;

    uint32_t refTypeSize = ilo::readUint32(begin, end);
    ref.referenceType = (refTypeSize & 0x80000000) != 0;
    ref.referenceSize = (refTypeSize & 0x0FFFFFFF);
    ref.subsegmentDuration = ilo::readUint32(begin, end);
    uint32_t sapTypeDeltaTime = ilo::readUint32(begin, end);
    ref.startsWithSap = (sapTypeDeltaTime & 0x80000000) != 0;
    ref.sapType = (sapTypeDeltaTime & 0x70000000) >> 28;
    ref.sapDeltaTime = (sapTypeDeltaTime & 0x0FFFFFFF);

    m_references.push_back(ref);
  }

  sanityCheck();
}

void CSegmentIndexBox::updateSize(uint64_t sizeValue) {
  // size + refId + timscale + m_earliestPresentationTime +
  // m_firstOffset + reserved + refCount + refTypeSize + subsegDur + sapTypeDeltaTime
  if (CFullBox::version() == 0) {
    CFullBox::updateSize(sizeValue + 4 + 4 + 4 + 4 + 2 + 2 + m_references.size() * 12);
  } else {
    CFullBox::updateSize(sizeValue + 4 + 4 + 8 + 8 + 2 + 2 + m_references.size() * 12);
  }
}

SAttributeList CSegmentIndexBox::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Reference Id";
  attribute.value = std::to_string(m_referenceId);
  attributesList.push_back(attribute);

  attribute.key = "Timescale";
  attribute.value = std::to_string(m_timescale);
  attributesList.push_back(attribute);

  attribute.key = "Earliest Presentation Time";
  attribute.value = std::to_string(m_earliestPresentationTime);
  attributesList.push_back(attribute);

  attribute.key = "First Offset";
  attribute.value = std::to_string(m_firstOffset);
  attributesList.push_back(attribute);

  attribute.key = "Reference Count";
  attribute.value = std::to_string(static_cast<uint32_t>(m_references.size()));
  attributesList.push_back(attribute);

  if (!m_references.empty()) {
    attribute.key = "References";
    std::stringstream ss;
    for (const auto& reference : m_references) {
      ss << "Reference Type: " << (std::to_string(reference.referenceType))
         << ", Reference Size: " << (std::to_string(reference.referenceSize))
         << ", Subsegment Duration: " << (std::to_string(reference.subsegmentDuration))
         << ", Starts With Sap: " << (std::to_string(reference.startsWithSap))
         << ", Sap Type: " << (std::to_string(reference.sapType))
         << ", Sap Delta Time: " << (std::to_string(reference.sapDeltaTime)) << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CSegmentIndexBox::writeBox(ilo::ByteBuffer& buffer,
                                ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint32(buffer, position, m_referenceId);
  ilo::writeUint32(buffer, position, m_timescale);

  if (CFullBox::version() == 0) {
    ilo::writeUint32_64(buffer, position, m_earliestPresentationTime);
    ilo::writeUint32_64(buffer, position, m_firstOffset);
  } else {
    ilo::writeUint64(buffer, position, m_earliestPresentationTime);
    ilo::writeUint64(buffer, position, m_firstOffset);
  }

  ilo::writeUint16(buffer, position, 0x00);

  ILO_ASSERT(std::numeric_limits<uint16_t>::max() >= m_references.size(),
             "Number of references in sidx box exceed max allowed entries");

  ilo::writeUint16(buffer, position, static_cast<uint16_t>(m_references.size()));

  for (const SSidxReference& ref : m_references) {
    uint32_t refTypeSize = 0;
    if (ref.referenceType) {
      refTypeSize |= 0x80000000;
    }

    refTypeSize |= ref.referenceSize;

    ilo::writeUint32(buffer, position, refTypeSize);
    ilo::writeUint32(buffer, position, ref.subsegmentDuration);

    uint32_t sapTypeDeltaTime = 0;
    if (ref.startsWithSap) {
      sapTypeDeltaTime |= 0x80000000;
    }

    sapTypeDeltaTime |= ((uint32_t)ref.sapType << 28);
    sapTypeDeltaTime |= ref.sapDeltaTime;

    ilo::writeUint32(buffer, position, sapTypeDeltaTime);
  }
}

void CSegmentIndexBox::sanityCheck() {}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(sidx, CSegmentIndexBox, CSegmentIndexBox::SSidxBoxWriteConfig,
                    CContainerType::noContainer);
