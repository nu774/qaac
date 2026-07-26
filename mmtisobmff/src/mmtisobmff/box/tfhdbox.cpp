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
 * Content: track fragment header box class
 */

// System headers
#include <limits>

// External headers
#include "ilo/bytebuffertools.h"
#include "ilo/string_utils.h"

// Internal headers
#include "tfhdbox.h"

namespace mmt {
namespace isobmff {
namespace box {

CTrackFragmentHeaderBox::CTrackFragmentHeaderBox(ilo::ByteBuffer::const_iterator& begin,
                                                 const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_trackId(0),
      m_baseDataOffset(0),
      m_sampleDescriptionIndex(0),
      m_defaultSampleDuration(0),
      m_defaultSampleSize(0),
      m_defaultSampleFlags(0),
      m_baseDataOffsetPresent(false),
      m_sampleDescriptionIndexPresent(false),
      m_defaultSampleDurationPresent(false),
      m_defaultSampleSizePresent(false),
      m_defaultSampleFlagsPresent(false),
      m_durationIsEmpty(false),
      m_defaultBaseIsMoof(false) {
  validate();
  parseBox(begin, end);
}

CTrackFragmentHeaderBox::CTrackFragmentHeaderBox(const STfhdBoxWriteConfig& tfhdBoxData)
    : CFullBox(tfhdBoxData),
      m_trackId(tfhdBoxData.trackId),
      m_baseDataOffset(tfhdBoxData.baseDataOffset),
      m_sampleDescriptionIndex(tfhdBoxData.sampleDescriptionIndex),
      m_defaultSampleDuration(tfhdBoxData.defaultSampleDuration),
      m_defaultSampleSize(tfhdBoxData.defaultSampleSize),
      m_defaultSampleFlags(tfhdBoxData.defaultSampleFlags),
      m_baseDataOffsetPresent(tfhdBoxData.baseDataOffsetPresent),
      m_sampleDescriptionIndexPresent(tfhdBoxData.sampleDescriptionIndexPresent),
      m_defaultSampleDurationPresent(tfhdBoxData.defaultSampleDurationPresent),
      m_defaultSampleSizePresent(tfhdBoxData.defaultSampleSizePresent),
      m_defaultSampleFlagsPresent(tfhdBoxData.defaultSampleFlagsPresent),
      m_durationIsEmpty(tfhdBoxData.durationIsEmpty),
      m_defaultBaseIsMoof(tfhdBoxData.defaultBaseIsMoof) {
  uint32_t tfhdFlags = 0;

  if (m_baseDataOffsetPresent) {
    tfhdFlags += 0x000001;
  }

  if (m_sampleDescriptionIndexPresent) {
    tfhdFlags += 0x000002;
  }

  if (m_defaultSampleDurationPresent) {
    tfhdFlags += 0x000008;
  }

  if (m_defaultSampleSizePresent) {
    tfhdFlags += 0x000010;
  }

  if (m_defaultSampleFlagsPresent) {
    tfhdFlags += 0x000020;
  }

  if (m_durationIsEmpty) {
    tfhdFlags += 0x010000;
  }

  if (m_defaultBaseIsMoof) {
    tfhdFlags += 0x020000;
  }

  CFullBox::updateFlags(tfhdFlags);
  updateSize(0);

  validate();
}

void CTrackFragmentHeaderBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("tfhd"), std::invalid_argument,
                  "Expected box type tfhd, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::version() == 0, std::invalid_argument,
                  "Version %s of tfhd box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  uint32_t fls = flags();
  m_baseDataOffsetPresent = (fls & 0x000001) != 0;
  m_sampleDescriptionIndexPresent = (fls & 0x000002) != 0;
  m_defaultSampleDurationPresent = (fls & 0x000008) != 0;
  m_defaultSampleSizePresent = (fls & 0x000010) != 0;
  m_defaultSampleFlagsPresent = (fls & 0x000020) != 0;
  m_durationIsEmpty = (fls & 0x010000) != 0;
  m_defaultBaseIsMoof = (fls & 0x020000) != 0;

  m_trackId = ilo::readUint32(begin, end);

  if (m_baseDataOffsetPresent) {
    m_baseDataOffset = ilo::readUint64(begin, end);
  }

  if (m_sampleDescriptionIndexPresent) {
    m_sampleDescriptionIndex = ilo::readUint32(begin, end);
  }

  if (m_defaultSampleDurationPresent) {
    m_defaultSampleDuration = ilo::readUint32(begin, end);
  }

  if (m_defaultSampleSizePresent) {
    m_defaultSampleSize = ilo::readUint32(begin, end);
  }

  if (m_defaultSampleFlagsPresent) {
    m_defaultSampleFlags = ilo::readUint32(begin, end);
  }

  if (m_durationIsEmpty) {
    m_defaultSampleDuration = 0;
  }
}

void CTrackFragmentHeaderBox::updateSize(uint64_t sizeValue) {
  // track id
  sizeValue += 4;

  // flags
  if (m_baseDataOffsetPresent) {
    sizeValue += 8;
  }

  if (m_sampleDescriptionIndexPresent) {
    sizeValue += 4;
  }

  if (m_defaultSampleDurationPresent) {
    sizeValue += 4;
  }

  if (m_defaultSampleSizePresent) {
    sizeValue += 4;
  }

  if (m_defaultSampleFlagsPresent) {
    sizeValue += 4;
  }

  CFullBox::updateSize(sizeValue);
}

SAttributeList CTrackFragmentHeaderBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Track ID";
  attribute.value = std::to_string(m_trackId);
  attributesList.push_back(attribute);

  attribute.key = "Base Data Offset";
  if (m_baseDataOffsetPresent) {
    attribute.value = std::to_string(m_baseDataOffset);
  } else {
    attribute.value = "-1";
  }
  attributesList.push_back(attribute);

  attribute.key = "Sample Description Index";
  if (m_sampleDescriptionIndex) {
    attribute.value = std::to_string(m_sampleDescriptionIndex);
  } else {
    attribute.value = "-1";
  }
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Duration";
  if (m_defaultSampleDuration) {
    attribute.value = std::to_string(m_defaultSampleDuration);
  } else {
    attribute.value = "-1";
  }
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Size";
  if (m_defaultSampleSize) {
    attribute.value = std::to_string(m_defaultSampleSize);
  } else {
    attribute.value = "-1";
  }
  attributesList.push_back(attribute);

  attribute.key = "Default Sample Flags";
  if (m_defaultSampleFlags) {
    attribute.value = std::to_string(m_defaultSampleFlags);
  } else {
    attribute.value = "-1";
  }
  attributesList.push_back(attribute);

  return attributesList;
}

void CTrackFragmentHeaderBox::validate() {
  ILO_ASSERT_WITH(flags() == 0x000000 || (flags() & 0x03003B) != 0, std::invalid_argument,
                  "Invalid flags found in tfhd box");
}

void CTrackFragmentHeaderBox::writeBox(ilo::ByteBuffer& buffer,
                                       ilo::ByteBuffer::iterator& position) const {
  ilo::writeInt32(buffer, position, m_trackId);

  if (m_baseDataOffsetPresent) {
    ilo::writeUint64(buffer, position, m_baseDataOffset);
  }

  if (m_sampleDescriptionIndexPresent) {
    ilo::writeInt32(buffer, position, m_sampleDescriptionIndex);
  }

  if (m_defaultSampleDurationPresent) {
    ilo::writeInt32(buffer, position, m_defaultSampleDuration);
  }

  if (m_defaultSampleSizePresent) {
    ilo::writeInt32(buffer, position, m_defaultSampleSize);
  }

  if (m_defaultSampleFlagsPresent) {
    ilo::writeInt32(buffer, position, m_defaultSampleFlags);
  }
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(tfhd, CTrackFragmentHeaderBox, CTrackFragmentHeaderBox::STfhdBoxWriteConfig,
                    CContainerType::noContainer);
