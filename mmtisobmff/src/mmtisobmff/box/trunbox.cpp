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
 * Content: track run box class
 */

// System headers
#include <stdexcept>
#include <string>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "trunbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CTrunEntry::CTrunEntry()
    : m_duration(0),
      m_size(0),
      m_flags(0),
      m_ctsOffset(0),
      m_durationPresent(false),
      m_sizePresent(false),
      m_flagsPresent(false),
      m_ctsOffsetPresent(false) {}

CTrunEntry::~CTrunEntry() {}

void CTrunEntry::setSampleDuration(uint32_t sDuration) {
  m_duration = sDuration;
  m_durationPresent = true;
}

void CTrunEntry::setSampleSize(uint32_t sSize) {
  m_size = sSize;
  m_sizePresent = true;
}

void CTrunEntry::setSampleFlags(uint32_t sFlags) {
  m_flags = sFlags;
  m_flagsPresent = true;
}

void CTrunEntry::setSampleCtsOffset(int64_t sCtsOffset) {
  m_ctsOffset = sCtsOffset;
  m_ctsOffsetPresent = true;
}

uint32_t CTrunEntry::sampleDuration() const {
  ILO_ASSERT(m_durationPresent, "Sample duration field not present because flag is not set");
  return m_duration;
}

uint32_t CTrunEntry::sampleSize() const {
  ILO_ASSERT(m_sizePresent, "Sample size field not present because flag is not set");
  return m_size;
}

uint32_t CTrunEntry::sampleFlags() const {
  ILO_ASSERT(m_flagsPresent, "Sample flags field not present because flag is not set");
  return m_flags;
}

int64_t CTrunEntry::sampleCtsOffset() const {
  ILO_ASSERT(m_ctsOffsetPresent, "Sample cts offset field not present because flag is not set");
  return m_ctsOffset;
}

CTrackRunBox::CTrackRunBox(ilo::ByteBuffer::const_iterator& begin,
                           const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_sampleCount(0),
      m_dataOffset(0),
      m_firstSampleFlags(0),
      m_trunEntries() {
  parseBox(begin, end);
}

CTrackRunBox::CTrackRunBox(const STrunBoxWriteConfig& config)
    : box::CFullBox(config),
      m_sampleCount(config.sampleCount),
      m_dataOffset(config.dataoffset),
      m_firstSampleFlags(config.firstSampleFlags),
      m_trunEntries(config.trunEntries) {
  ILO_ASSERT_WITH(m_trunEntries.size() == m_sampleCount, std::invalid_argument,
                  "Number of trun entries in trun box does not match the sample count number");

  uint32_t trunFlags = 0;

  if (config.dataOffsetPresent) {
    trunFlags += 0x000001;
  }
  if (config.firstSampleFlagsPresent) {
    trunFlags += 0x000004;
  }
  if (config.sampleDurationPresent) {
    trunFlags += 0x000100;
  }
  if (config.sampleSizePresent) {
    trunFlags += 0x000200;
  }
  if (config.sampleFlagsPresent) {
    trunFlags += 0x000400;
  }
  if (config.sampleCtsOffsetPresent) {
    trunFlags += 0x000800;
  }

  CFullBox::updateFlags(trunFlags);

  flagSanityCheck();

  uint8_t boxVersion = 0;

  if (sampleCtsOffsetPresent()) {
    for (const auto& trunEntry : m_trunEntries) {
      if (trunEntry.sampleCtsOffset() < 0) {
        boxVersion = 1;
        break;
      }
    }
  }

  CFullBox::updateVersion(boxVersion);

  updateSize(0);
}

void CTrackRunBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                            const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("trun"), std::invalid_argument,
                  "Expected box type trun, but found: %s", ilo::toString(CBox::type()).c_str());

  flagSanityCheck();

  m_sampleCount = ilo::readUint32(begin, end);

  if (dataOffsetPresent()) {
    m_dataOffset = ilo::readInt32(begin, end);
  }

  if (firstSampleFlagsPresent()) {
    m_firstSampleFlags = ilo::readUint32(begin, end);
  }

  for (uint32_t i = 0; i < m_sampleCount; ++i) {
    CTrunEntry trunEntry;

    if (sampleDurationPresent()) {
      trunEntry.setSampleDuration(ilo::readUint32(begin, end));
    }

    if (sampleSizePresent()) {
      trunEntry.setSampleSize(ilo::readUint32(begin, end));
    }

    if (sampleFlagsPresent()) {
      trunEntry.setSampleFlags(ilo::readUint32(begin, end));
    }

    if (sampleCtsOffsetPresent()) {
      int64_t sampleCtsOffset = 0;

      if (CFullBox::version() == 0) {
        sampleCtsOffset = static_cast<int64_t>(ilo::readUint32(begin, end));
      } else {
        sampleCtsOffset = static_cast<int64_t>(ilo::readInt32(begin, end));
      }

      trunEntry.setSampleCtsOffset(sampleCtsOffset);
    }

    m_trunEntries.push_back(trunEntry);
  }
}

void CTrackRunBox::updateSize(uint64_t sizeValue) {
  uint64_t staticSize = 4;

  if (dataOffsetPresent()) {
    staticSize += 4;
  }

  if (firstSampleFlagsPresent()) {
    staticSize += 4;
  }

  for (uint32_t i = 0; i < m_sampleCount; ++i) {
    if (sampleDurationPresent()) {
      staticSize += 4;
    }

    if (sampleSizePresent()) {
      staticSize += 4;
    }

    if (sampleFlagsPresent()) {
      staticSize += 4;
    }

    if (sampleCtsOffsetPresent()) {
      staticSize += 4;
    }
  }

  CFullBox::updateSize(sizeValue + staticSize);
}

SAttributeList CTrackRunBox::getAttributeList() const {
  SAttributeList attributesList;
  SAttribute attribute;

  attribute.key = "Data Offset Present";
  attribute.value = dataOffsetPresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "Data Offset";
  attribute.value = std::to_string(m_dataOffset);
  attributesList.push_back(attribute);

  attribute.key = "First Sample Flags Present";
  attribute.value = firstSampleFlagsPresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "First Sample Flags";
  attribute.value = std::to_string(m_firstSampleFlags);
  attributesList.push_back(attribute);

  attribute.key = "Sample Duration Present";
  attribute.value = sampleDurationPresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "Sample Size Present";
  attribute.value = sampleSizePresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "Sample Flags Present";
  attribute.value = sampleFlagsPresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "Sample Cts Offset Present";
  attribute.value = sampleCtsOffsetPresent() ? "true" : "false";
  attributesList.push_back(attribute);

  attribute.key = "Sample Count";
  attribute.value = std::to_string(m_sampleCount);
  attributesList.push_back(attribute);

  if (m_sampleCount > 0) {
    attribute.key = "Trun Entries";
    std::stringstream ss;
    for (auto entry : m_trunEntries) {
      ss << "Duration: " << (sampleDurationPresent() ? std::to_string(entry.sampleDuration()) : "0")
         << ", Size: " << (sampleSizePresent() ? std::to_string(entry.sampleSize()) : "0")
         << ", Flags: " << (sampleFlagsPresent() ? std::to_string(entry.sampleFlags()) : "0")
         << ", Cts Offset: "
         << (sampleCtsOffsetPresent() ? std::to_string(entry.sampleCtsOffset()) : "0") << ";";
    }

    attribute.value = ss.str();
    attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CTrackRunBox::writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint32(buffer, position, m_sampleCount);

  if (dataOffsetPresent()) {
    ilo::writeInt32(buffer, position, m_dataOffset);
  }

  if (firstSampleFlagsPresent()) {
    ilo::writeUint32(buffer, position, m_firstSampleFlags);
  }

  for (const auto& trunEntry : m_trunEntries) {
    if (sampleDurationPresent()) {
      ilo::writeUint32(buffer, position, trunEntry.sampleDuration());
    }

    if (sampleSizePresent()) {
      ilo::writeUint32(buffer, position, trunEntry.sampleSize());
    }

    if (sampleFlagsPresent()) {
      ilo::writeUint32(buffer, position, trunEntry.sampleFlags());
    }

    if (CFullBox::version() == 0) {
      if (sampleCtsOffsetPresent()) {
        ilo::writeUint32_64(buffer, position, static_cast<uint64_t>(trunEntry.sampleCtsOffset()));
      }
    } else {
      if (sampleCtsOffsetPresent()) {
        ilo::writeInt32(buffer, position, static_cast<int32_t>(trunEntry.sampleCtsOffset()));
      }
    }
  }
}

uint32_t CTrackRunBox::dataOffset() const {
  ILO_ASSERT(dataOffsetPresent(), "Data offset field not present because flag is not set");
  return m_dataOffset;
}

uint32_t CTrackRunBox::firstSampleFlags() const {
  ILO_ASSERT(firstSampleFlagsPresent(),
             "First sample flags field not present because flag is not set");
  return m_firstSampleFlags;
}

void CTrackRunBox::flagSanityCheck() {
  ILO_ASSERT_WITH(!(firstSampleFlagsPresent() && sampleFlagsPresent()), std::invalid_argument,
                  "Flags %s in trun box field are not valid",
                  std::to_string(CFullBox::flags()).c_str());
}
}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(trun, CTrackRunBox, CTrackRunBox::STrunBoxWriteConfig,
                    CContainerType::noContainer);
