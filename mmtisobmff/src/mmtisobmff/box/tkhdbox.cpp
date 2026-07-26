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
 * Content: track header box class
 */

// System headers
#include <algorithm>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "tkhdbox.h"
#include "common/logging.h"
#include "mmtisobmff/helper/commonhelpertools.h"

namespace mmt {
namespace isobmff {
namespace box {

CTrackHeaderBox::CTrackHeaderBox(ilo::ByteBuffer::const_iterator& begin,
                                 const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_creationTime(0),
      m_modificationTime(0),
      m_trackID(0),
      m_duration(0),
      m_layer(0),
      m_alternateGroup(0),
      m_volume(0x0100),
      m_matrix({0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000}),
      m_width(0),
      m_height(0) {
  parseBox(begin, end);
}

CTrackHeaderBox::CTrackHeaderBox(const STkhdBoxWriteConfig& tkhdBoxData)
    : CFullBox(tkhdBoxData),
      m_creationTime(tkhdBoxData.creationTime),
      m_modificationTime(tkhdBoxData.modificationTime),
      m_trackID(tkhdBoxData.trackID),
      m_duration(tkhdBoxData.duration),
      m_layer(tkhdBoxData.layer),
      m_alternateGroup(tkhdBoxData.alternateGroup),
      m_volume(tkhdBoxData.volume),
      m_matrix(tkhdBoxData.matrix),
      m_width(tkhdBoxData.width),
      m_height(tkhdBoxData.height) {
  if (tkhdBoxData.creationTime > std::numeric_limits<uint32_t>::max() ||
      tkhdBoxData.modificationTime > std::numeric_limits<uint32_t>::max() ||
      tkhdBoxData.duration > std::numeric_limits<uint32_t>::max()) {
    CFullBox::updateVersion(1);
  }

  uint32_t tkhdFlags = 0;

  if (tkhdBoxData.trackIsEnabled) {
    tkhdFlags += 0x000001;
  }

  if (tkhdBoxData.trackInMovie) {
    tkhdFlags += 0x000002;
  }

  if (tkhdBoxData.trackInPreview) {
    tkhdFlags += 0x000004;
  }

  if (tkhdBoxData.trackSizeIsAspectRatio) {
    tkhdFlags += 0x000008;
  }

  CFullBox::updateFlags(tkhdFlags);

  sanityCheck();

  updateSize(0);
}

void CTrackHeaderBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                               const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("tkhd"), std::invalid_argument,
                  "Expected box type tkhd, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::version() == 0 || CFullBox::version() == 1, std::invalid_argument,
                  "Version %s of tkhd box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  if (CFullBox::version() == 0) {
    m_creationTime = ilo::readUint32(begin, end);
    m_modificationTime = ilo::readUint32(begin, end);
    m_trackID = ilo::readUint32(begin, end);

    ILO_ASSERT(ilo::readUint32(begin, end) == 0, "Reserved value must be zero for the tkhd box");

    m_duration = ilo::readUint32(begin, end);
  } else {
    m_creationTime = ilo::readUint64(begin, end);
    m_modificationTime = ilo::readUint64(begin, end);
    m_trackID = ilo::readUint32(begin, end);

    ILO_ASSERT(ilo::readUint32(begin, end) == 0, "Reserved value must be zero for the tkhd box");

    m_duration = ilo::readUint64(begin, end);
  }

  ILO_ASSERT(ilo::readUint64(begin, end) == 0, "Reserved value must be zero for the tkhd box");

  m_layer = ilo::readInt16(begin, end);
  m_alternateGroup = ilo::readInt16(begin, end);
  m_volume = ilo::readInt16(begin, end);

  if (ilo::readUint16(begin, end) != 0) {
    ILO_LOG_WARNING("Reserved uint16 value is not zero for the tkhd box");
  }

  std::vector<int32_t> tmp = ilo::readInt32Array(begin, end, 9);
  std::copy_n(tmp.begin(), 9, m_matrix.begin());

  // Width and height are 32 bits values interpreted as 16.16
  // We currently only support the first 16 bit
  uint32_t tmpWidth = ilo::readUint32(begin, end);
  uint32_t tmpHeight = ilo::readUint32(begin, end);

  ILO_ASSERT((tmpWidth & 0xFFFF) == 0, "Video width of %d in tkhd box is not an integer width",
             tmpWidth);
  ILO_ASSERT((tmpHeight & 0xFFFF) == 0, "Video height of %d in tkhd box is not an integer height",
             tmpHeight);

  m_width = static_cast<uint16_t>(tmpWidth >> 16);
  m_height = static_cast<uint16_t>(tmpHeight >> 16);

  sanityCheck();
}

void CTrackHeaderBox::updateSize(uint64_t sizeValue) {
  // size + creation + modification + id + res + dur + res + lay +
  // alter_grp + vol + res + matrix + width + height
  if (CFullBox::version() == 0) {
    CFullBox::updateSize(sizeValue + 4 + 4 + 4 + 4 + 4 + 8 + 2 + 2 + 2 + 2 + 36 + 4 + 4);
  } else {
    CFullBox::updateSize(sizeValue + 8 + 8 + 4 + 4 + 8 + 8 + 2 + 2 + 2 + 2 + 36 + 4 + 4);
  }
}

SAttributeList CTrackHeaderBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Creation Time";
  attribute.value = tools::UTCTimeToString(m_creationTime);
  attributesList.push_back(attribute);

  attribute.key = "Modification Time";
  attribute.value = tools::UTCTimeToString(m_modificationTime);
  attributesList.push_back(attribute);

  attribute.key = "Track ID";
  attribute.value = std::to_string(m_trackID);
  attributesList.push_back(attribute);

  attribute.key = "Duration";
  attribute.value = std::to_string(m_duration);
  attributesList.push_back(attribute);

  attribute.key = "Layer";
  attribute.value = std::to_string(m_layer);
  attributesList.push_back(attribute);

  attribute.key = "Alternate Group";
  attribute.value = std::to_string(m_alternateGroup);
  attributesList.push_back(attribute);

  attribute.key = "Volume";
  attribute.value = std::to_string(m_volume);
  attributesList.push_back(attribute);

  attribute.key = "Matrix";
  std::stringstream ss;
  for (auto element : m_matrix) {
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase
       << static_cast<int>(element);
    ss << ", ";
  }
  attribute.value = ss.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 2);
  attributesList.push_back(attribute);

  attribute.key = "Width";
  attribute.value = std::to_string(m_width);
  attributesList.push_back(attribute);

  attribute.key = "Height";
  attribute.value = std::to_string(m_height);
  attributesList.push_back(attribute);

  return attributesList;
}

void CTrackHeaderBox::writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  if (CFullBox::version() == 0) {
    ilo::writeUint32_64(buffer, position, m_creationTime);
    ilo::writeUint32_64(buffer, position, m_modificationTime);
    ilo::writeUint32(buffer, position, m_trackID);
    ilo::writeUint32(buffer, position, 0U);
    ilo::writeUint32_64(buffer, position, m_duration);
  } else {
    ilo::writeUint64(buffer, position, m_creationTime);
    ilo::writeUint64(buffer, position, m_modificationTime);
    ilo::writeUint32(buffer, position, m_trackID);
    ilo::writeUint32(buffer, position, 0U);
    ilo::writeUint64(buffer, position, m_duration);
  }

  ilo::writeUint64(buffer, position, 0U);
  ilo::writeInt16(buffer, position, m_layer);
  ilo::writeInt16(buffer, position, m_alternateGroup);
  ilo::writeInt16(buffer, position, m_volume);
  ilo::writeInt16(buffer, position, 0U);

  for (auto value : m_matrix) {
    ilo::writeInt32(buffer, position, value);
  }

  uint32_t tmpWidth = static_cast<uint32_t>(m_width) << 16;
  uint32_t tmpHeight = static_cast<uint32_t>(m_height) << 16;
  ilo::writeUint32(buffer, position, tmpWidth);
  ilo::writeUint32(buffer, position, tmpHeight);
}

void CTrackHeaderBox::sanityCheck() {
  if (m_creationTime > m_modificationTime) {
    ILO_LOG_WARNING(
        "Warning: Creation time in tkhd box of MP4 is smaller than modifcation time: %u, %u",
        m_creationTime, m_modificationTime);
  }

  ILO_ASSERT_WITH(CFullBox::flags() <= 0x00000F, std::invalid_argument,
                  "Found invalid/unknown flag of %s for tkhd box",
                  std::to_string(CFullBox::flags()).c_str());

  ILO_ASSERT_WITH((m_width || m_height) == (m_width && m_height), std::invalid_argument,
                  "Both width and height must be present in tkhd box for video tracks");
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(tkhd, CTrackHeaderBox, CTrackHeaderBox::STkhdBoxWriteConfig,
                    CContainerType::noContainer);
