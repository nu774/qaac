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
 * Content: track extends box class
 */

// System headers
#include <stdexcept>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "trexbox.h"
#include "common/logging.h"
#include "mmtisobmff/helper/commonhelpertools.h"

namespace mmt {
namespace isobmff {
namespace box {

CTrackExtendsBox::CTrackExtendsBox(ilo::ByteBuffer::const_iterator& begin,
                                   const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_trackID(0),
      m_defaultSampleDescriptionIndex(0),
      m_defaultSampleDuration(0),
      m_defaultSampleSize(0),
      m_defaultSampleFlags(0) {
  parseBox(begin, end);
}

CTrackExtendsBox::CTrackExtendsBox(const STrexBoxWriteConfig& trexBoxData)
    : CFullBox(trexBoxData),
      m_trackID(trexBoxData.trackID),
      m_defaultSampleDescriptionIndex(trexBoxData.defaultSampleDescriptionIndex),
      m_defaultSampleDuration(trexBoxData.defaultSampleDuration),
      m_defaultSampleSize(trexBoxData.defaultSampleSize),
      m_defaultSampleFlags(trexBoxData.defaultSampleFlags) {
  updateSize(0);
}

void CTrackExtendsBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("trex"), std::invalid_argument,
                  "Expected box type trex, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::version() == 0, std::invalid_argument,
                  "Version %s of trex box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the trex box");

  m_trackID = ilo::readUint32(begin, end);
  m_defaultSampleDescriptionIndex = ilo::readUint32(begin, end);
  m_defaultSampleDuration = ilo::readUint32(begin, end);
  m_defaultSampleSize = ilo::readUint32(begin, end);
  m_defaultSampleFlags = ilo::readUint32(begin, end);
}

SAttributeList CTrackExtendsBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Track ID";
  attribute.value = std::to_string(m_trackID);
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Description Index";
  attribute.value = std::to_string(m_defaultSampleDescriptionIndex);
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Duration";
  attribute.value = std::to_string(m_defaultSampleDuration);
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Size";
  attribute.value = std::to_string(m_defaultSampleSize);
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Flags";
  SSampleFlags sampleFlags = tools::valueToSampleFlags(m_defaultSampleFlags);
  attribute.value =
      "isLeading=" + std::to_string(static_cast<uint8_t>(sampleFlags.isLeading)) +
      ", depOn=" + std::to_string(static_cast<uint8_t>(sampleFlags.dependsON)) +
      ", isDepOn=" + std::to_string(static_cast<uint8_t>(sampleFlags.isDependedOn)) +
      ", hasRedundancy=" + std::to_string(static_cast<uint8_t>(sampleFlags.hasRedundancy)) +
      ", padValue=" + std::to_string(sampleFlags.paddingValue) +
      ", isDiffSample=" + (sampleFlags.isNonSyncSample ? "true" : "false") +
      ", degradPrio=" + std::to_string(sampleFlags.degradationPriority);
  attributesList.push_back(attribute);

  return attributesList;
}

void CTrackExtendsBox::updateSize(uint64_t sizeValue) {
  // size + trackID + defaultSampleDescriptionIndex + defaultSampleDuration + defaultSampleSize +
  // defaultSampleFlags
  CFullBox::updateSize(sizeValue + 4 * 5);
}

void CTrackExtendsBox::writeBox(ilo::ByteBuffer& buffer,
                                ilo::ByteBuffer::iterator& position) const {
  ilo::writeUint32(buffer, position, m_trackID);
  ilo::writeUint32(buffer, position, m_defaultSampleDescriptionIndex);
  ilo::writeUint32(buffer, position, m_defaultSampleDuration);
  ilo::writeUint32(buffer, position, m_defaultSampleSize);
  ilo::writeUint32(buffer, position, m_defaultSampleFlags);
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(trex, CTrackExtendsBox, CTrackExtendsBox::STrexBoxWriteConfig,
                    CContainerType::noContainer);
