/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2017 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * Content: sample group description box class
 */

// System headers
#include <stdexcept>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "sgpdbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {
CSampleGroupDescriptionBox::CSampleGroupDescriptionBox(ilo::ByteBuffer::const_iterator& begin,
                                                       const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end), m_defaultLength(0), m_defaultSampleDescriptionIndex(0) {
  parseBox(begin, end);
}

CSampleGroupDescriptionBox::CSampleGroupDescriptionBox(const SSgpdBoxWriteConfig& config)
    : CFullBox(config) {
  m_groupingType = config.groupingType;
  m_defaultLength = config.defaultLength;
  m_defaultSampleDescriptionIndex = config.defaultSampleDescriptionIndex;
  m_sampleGroupDescriptionEntries = config.sampleGroupDescriptionEntries;
  updateSize(0);
}

void CSampleGroupDescriptionBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                          const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("sgpd"), std::invalid_argument,
                  "Expected box type sgpd, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the sgpd box");

  m_groupingType = ilo::readFourCC(begin, end);

  ILO_ASSERT_WITH(m_groupingType == ilo::toFcc("roll") || m_groupingType == ilo::toFcc("prol") ||
                      m_groupingType == ilo::toFcc("sap "),
                  std::invalid_argument, "Grouping type: %s is not supported",
                  ilo::toString(m_groupingType).c_str());

  if (CFullBox::version() == 1) {
    m_defaultLength = ilo::readUint32(begin, end);
  } else if (CFullBox::version() >= 2) {
    m_defaultSampleDescriptionIndex = ilo::readUint32(begin, end);
  }

  uint32_t entryCount = ilo::readUint32(begin, end);
  // sanity check, assume every entry has at least a single byte
  ILO_ASSERT_WITH(static_cast<int64_t>(entryCount) <= static_cast<int64_t>(end - begin),
                  std::out_of_range,
                  "Sample group description entry count is bigger than remaining buffer");
  m_sampleGroupDescriptionEntries.resize(entryCount);

  for (auto& sampleGroupDescriptionEntry : m_sampleGroupDescriptionEntries) {
    if (CFullBox::version() == 1 && m_defaultLength == 0) {
      sampleGroupDescriptionEntry.descriptionLength = ilo::readUint32(begin, end);
    }

    if (m_groupingType == ilo::toFcc("roll")) {
      sampleGroupDescriptionEntry.sampleGroupEntry =
          std::make_shared<CAudioRollRecoveryEntry>(begin, end);
    } else if (m_groupingType == ilo::toFcc("prol")) {
      sampleGroupDescriptionEntry.sampleGroupEntry =
          std::make_shared<CAudioPreRollEntry>(begin, end);
    } else if (m_groupingType == ilo::toFcc("sap ")) {
      sampleGroupDescriptionEntry.sampleGroupEntry = std::make_shared<CSAPEntry>(begin, end);
    }
  }
}

void CSampleGroupDescriptionBox::updateSize(uint64_t sizeValue) {
  uint32_t sampleEntriesSize = 0;
  for (const auto& entry : m_sampleGroupDescriptionEntries) {
    sampleEntriesSize += entry.sampleGroupEntry->entrySize();
  }

  switch (CFullBox::version()) {
    case 0:
      // size + size(grouping_type) + size(entry_count) + entryCount*size(sampleGroupEntry)
      CFullBox::updateSize(sizeValue + 4 + 4 + sampleEntriesSize);
      break;
    case 1:
      if (m_defaultLength == 0) {
        // size + size(grouping_type) + size(default_length) + size(entry_count) +
        // entryCount*(size(description_length)+size(sampleGroupEntry)
        CFullBox::updateSize(sizeValue + 4 + 4 + 4 + m_sampleGroupDescriptionEntries.size() * 4 +
                             sampleEntriesSize);
      } else {
        // size + size(grouping_type) + size(default_length) + size(entry_count) +
        // entryCount*size(sampleGroupEntry)
        CFullBox::updateSize(sizeValue + 4 + 4 + 4 + sampleEntriesSize);
      }
      break;
    default:
      // size + size(grouping_type) + size(default_sample_description_index) + size(entry_count) +
      // entryCount*size(sampleGroupEntry)
      CFullBox::updateSize(sizeValue + 4 + 4 + 4 + sampleEntriesSize);
      break;
  }
}

SAttributeList CSampleGroupDescriptionBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;

  attribute.key = "Grouping Type";
  attribute.value = ilo::toString(m_groupingType);
  attributesList.push_back(attribute);

  if (version() == 1) {
    attribute.key = "Default Length";
    attribute.value = std::to_string(m_defaultLength);
  } else {
    attribute.key = "Default Sample Description Index";
    attribute.value = std::to_string(m_defaultSampleDescriptionIndex);
  }
  attributesList.push_back(attribute);

  attribute.key = "Entry Count";
  attribute.value = std::to_string(static_cast<uint32_t>(m_sampleGroupDescriptionEntries.size()));
  attributesList.push_back(attribute);

  if (!m_sampleGroupDescriptionEntries.empty()) {
    attribute.key = "Sample Group Description Entries";
    std::stringstream ss;
    for (auto& entry : m_sampleGroupDescriptionEntries) {
      auto sampleGroupAttributeList = entry.sampleGroupEntry->getAttributeList();
      ILO_ASSERT(sampleGroupAttributeList.size() <= 1,
                 "Sample Group Entries should only have 1 attribute");
      if (version() == 1 && m_defaultLength == 0) {
        ss << "Description Length: " << std::to_string(entry.descriptionLength) << ", ";
      }
      ss << sampleGroupAttributeList[0].key << ": " << sampleGroupAttributeList[0].value << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CSampleGroupDescriptionBox::writeBox(ilo::ByteBuffer& buffer,
                                          ilo::ByteBuffer::iterator& position) const {
  ilo::writeFourCC(buffer, position, m_groupingType);
  if (CFullBox::version() == 1) {
    ilo::writeUint32(buffer, position, m_defaultLength);
  }
  if (CFullBox::version() >= 2) {
    ilo::writeUint32(buffer, position, m_defaultSampleDescriptionIndex);
  }
  ilo::writeUint32(buffer, position, static_cast<uint32_t>(m_sampleGroupDescriptionEntries.size()));

  for (const auto& entry : m_sampleGroupDescriptionEntries) {
    if (CFullBox::version() == 1 && m_defaultLength == 0) {
      ilo::writeUint32(buffer, position, entry.descriptionLength);
    }
    entry.sampleGroupEntry->writeEntry(buffer, position);
  }
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(sgpd, CSampleGroupDescriptionBox,
                    CSampleGroupDescriptionBox::SSgpdBoxWriteConfig, CContainerType::noContainer);
