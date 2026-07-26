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
#include "sbgpbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {
CSampleToGroupBox::CSampleToGroupBox(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end), m_groupingTypeParameter(0) {
  parseBox(begin, end);
}

CSampleToGroupBox::CSampleToGroupBox(const SSbgpBoxWriteConfig& config) : CFullBox(config) {
  m_groupingType = config.groupingType;
  m_groupingTypeParameter = config.groupingTypeParameter;
  m_sampleGroupEntries = config.sampleGroupEntries;

  updateSize(0);
}

void CSampleToGroupBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                 const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("sbgp"), std::invalid_argument,
                  "Expected box type sbgp, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the sbgp box");

  m_groupingType = ilo::readFourCC(begin, end);
  if (CFullBox::version() == 1) {
    m_groupingTypeParameter = ilo::readUint32(begin, end);
  }

  uint32_t entryCount = ilo::readUint32(begin, end);
  uint32_t sampleCount = 0;
  uint32_t groupDescriptionIndex = 0;
  // sanity check, every entry has 8 byte
  ILO_ASSERT_WITH((static_cast<int64_t>(entryCount) * 8) <= static_cast<int64_t>(end - begin),
                  std::out_of_range, "Sample to group entry count is bigger than remaining buffer");
  m_sampleGroupEntries.resize(entryCount);

  for (auto& entry : m_sampleGroupEntries) {
    sampleCount = ilo::readUint32(begin, end);
    groupDescriptionIndex = ilo::readUint32(begin, end);
    entry = SSampleGroupEntry{sampleCount, groupDescriptionIndex};
  }
}

void CSampleToGroupBox::updateSize(uint64_t sizeValue) {
  if (CFullBox::version() == 1) {
    // size + size(grouping_type) + size(grouping_type_parameter) + size(entry_count) +
    // entryCount*(size(sample_count) + size(group_description_index))
    CFullBox::updateSize(sizeValue + 4 + 4 + 4 + m_sampleGroupEntries.size() * (4 + 4));
  } else {
    // size + size(grouping_type) + size(entry_count) + entryCount*(size(sample_count) +
    // size(group_description_index))
    CFullBox::updateSize(sizeValue + 4 + 4 + m_sampleGroupEntries.size() * (4 + 4));
  }
}

SAttributeList CSampleToGroupBox::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Grouping Type";
  attribute.value = ilo::toString(m_groupingType);
  attributesList.push_back(attribute);

  if (version() == 1) {
    attribute.key = "Grouping Type Parameter";
    attribute.value = std::to_string(m_groupingTypeParameter);
    attributesList.push_back(attribute);
  }

  attribute.key = "Entry Count";
  attribute.value = std::to_string(static_cast<uint32_t>(m_sampleGroupEntries.size()));
  attributesList.push_back(attribute);

  if (!m_sampleGroupEntries.empty()) {
    attribute.key = "Sample Group Entries";
    std::stringstream ss;
    for (auto entry : m_sampleGroupEntries) {
      ss << "Group Description Index: " << (std::to_string(entry.groupDescriptionIndex))
         << ", Sample Count: " << (std::to_string(entry.sampleCount)) << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CSampleToGroupBox::writeBox(ilo::ByteBuffer& buffer,
                                 ilo::ByteBuffer::iterator& position) const {
  ilo::writeFourCC(buffer, position, m_groupingType);

  if (CFullBox::version() == 1) {
    ilo::writeUint32(buffer, position, m_groupingTypeParameter);
  }

  ilo::writeUint32(buffer, position, static_cast<uint32_t>(m_sampleGroupEntries.size()));

  for (const auto& entry : m_sampleGroupEntries) {
    ilo::writeUint32(buffer, position, entry.sampleCount);
    ilo::writeUint32(buffer, position, entry.groupDescriptionIndex);
  }
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(sbgp, CSampleToGroupBox, CSampleToGroupBox::SSbgpBoxWriteConfig,
                    CContainerType::noContainer);
