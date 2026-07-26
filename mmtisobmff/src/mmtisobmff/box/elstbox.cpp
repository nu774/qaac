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
 * Content: edit list box class
 */

// System headers
#include <stdexcept>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "elstbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CEditListBox::CEditListBox(ilo::ByteBuffer::const_iterator& begin,
                           const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end) {
  parse(begin, end);
}

CEditListBox::CEditListBox(const SEditListBoxWriteConfig& elstBoxData)
    : CFullBox(elstBoxData), m_entries(elstBoxData.entries) {
  for (auto& entry : m_entries) {
    if (entry.mediaTime > std::numeric_limits<int32_t>::max() ||
        entry.mediaTime < std::numeric_limits<int32_t>::min() ||
        entry.segmentDuration > std::numeric_limits<uint32_t>::max()) {
      CFullBox::updateVersion(1);
      break;
    }
  }

  updateSize(0);
}

void CEditListBox::updateSize(uint64_t sizeValue) {
  size_t s = (version() == 1) ? 20 : 12;
  CFullBox::updateSize(sizeValue + 4 + m_entries.size() * s);
}

SAttributeList CEditListBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;

  attribute.key = "Entry Count";
  attribute.value = std::to_string(static_cast<uint32_t>(m_entries.size()));
  attributesList.push_back(attribute);

  if (!m_entries.empty()) {
    attribute.key = "Entries";
    std::stringstream ss;
    for (const auto& entry : m_entries) {
      ss << "Media Rate Fraction: " << std::to_string(entry.mediaRateFraction)
         << ", Media Rate Integer: " << std::to_string(entry.mediaRateInteger)
         << ", Media Time: " << std::to_string(entry.mediaTime)
         << ", Segment Duration: " << std::to_string(entry.segmentDuration) << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);

    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CEditListBox::writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint32(buffer, position, static_cast<uint32_t>(m_entries.size()));

  for (const auto& entry : m_entries) {
    if (version() == 1) {
      ilo::writeUint64(buffer, position, entry.segmentDuration);
      ilo::writeInt64(buffer, position, entry.mediaTime);
    } else {
      ilo::writeUint32(buffer, position, static_cast<uint32_t>(entry.segmentDuration));
      ilo::writeInt32(buffer, position, static_cast<int32_t>(entry.mediaTime));
    }

    ilo::writeInt16(buffer, position, entry.mediaRateInteger);
    ilo::writeInt16(buffer, position, entry.mediaRateFraction);
  }
}

void CEditListBox::parse(ilo::ByteBuffer::const_iterator& begin,
                         const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CFullBox::version() == 0 || CFullBox::version() == 1, std::invalid_argument,
                  "Version %s of elst box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("elst"), std::invalid_argument,
                  "Expected box type elst, but found: %s while parsing",
                  ilo::toString(CBox::type()).c_str());

  uint32_t ec = ilo::readUint32(begin, end);

  for (auto i = 0U; i < ec; ++i) {
    SElstEntry e;
    if (version() == 1) {
      e.segmentDuration = ilo::readUint64(begin, end);
      e.mediaTime = ilo::readInt64(begin, end);
    } else {
      e.segmentDuration = ilo::readUint32(begin, end);
      e.mediaTime = ilo::readInt32(begin, end);
    }

    e.mediaRateInteger = ilo::readInt16(begin, end);
    e.mediaRateFraction = ilo::readInt16(begin, end);

    m_entries.push_back(e);
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

BOXREGISTRY_DECLARE(elst, CEditListBox, CEditListBox::SEditListBoxWriteConfig,
                    CContainerType::noContainer);
