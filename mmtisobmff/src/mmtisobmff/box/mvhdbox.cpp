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
 * Content: movie header box class
 */

// System headers
#include <stdexcept>
#include <limits>
#include <algorithm>

// External headers
#include "ilo/string_utils.h"
#include "ilo/bytebuffertools.h"

// Internal headers
#include "mvhdbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CMovieHeaderBox::CMovieHeaderBox(ilo::ByteBuffer::const_iterator& begin,
                                 const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_creationTime(0),
      m_modificationTime(0),
      m_timescale(0),
      m_duration(0),
      m_rate(0x00010000),
      m_volume(0x0100),
      m_matrix({0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000}),
      m_nextTrackID(0) {
  parseBox(begin, end);
}

CMovieHeaderBox::CMovieHeaderBox(const SMvhdBoxWriteConfig& config)
    : CFullBox(config),
      m_creationTime(config.creationTime),
      m_modificationTime(config.modificationTime),
      m_timescale(config.timescale),
      m_duration(config.duration),
      m_rate(config.rate),
      m_volume(config.volume),
      m_matrix(config.matrix),
      m_nextTrackID(config.nextTrackID) {
  if (config.creationTime > std::numeric_limits<uint32_t>::max() ||
      config.modificationTime > std::numeric_limits<uint32_t>::max() ||
      config.timescale > std::numeric_limits<uint32_t>::max() ||
      config.duration > std::numeric_limits<uint32_t>::max()) {
    CFullBox::updateVersion(1);
  }

  sanityCheck();
  updateSize(0);
}

void CMovieHeaderBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                               const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("mvhd"), std::invalid_argument,
                  "Expected box type mvhd, but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the mvhd box");

  ILO_ASSERT_WITH(CFullBox::version() == 0 || CFullBox::version() == 1, std::invalid_argument,
                  "Version %s of mvhd box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  if (CFullBox::version() == 0) {
    m_creationTime = ilo::readUint32(begin, end);
    m_modificationTime = ilo::readUint32(begin, end);
    m_timescale = ilo::readUint32(begin, end);
    m_duration = ilo::readUint32(begin, end);
  } else {
    m_creationTime = ilo::readUint64(begin, end);
    m_modificationTime = ilo::readUint64(begin, end);
    m_timescale = ilo::readUint64(begin, end);
    m_duration = ilo::readUint64(begin, end);
  }

  m_rate = ilo::readInt32(begin, end);
  m_volume = ilo::readInt16(begin, end);

  if (ilo::readUint16(begin, end) != 0) {
    ILO_LOG_WARNING("Reserved 16bit field is not zero for the mvhd box");
  }

  ILO_ASSERT(ilo::readUint64(begin, end) == 0, "Reserved value must be zero for the mvhd box");

  std::vector<int32_t> tmp = ilo::readInt32Array(begin, end, 9);
  std::copy_n(tmp.begin(), 9, m_matrix.begin());

  for (uint32_t i = 0; i < 6; ++i) {
    if (ilo::readUint32(begin, end) != 0) {
      ILO_LOG_WARNING("Predefined value in mvhd box is not zero");
    }
  }

  m_nextTrackID = ilo::readUint32(begin, end);

  sanityCheck();
}

void CMovieHeaderBox::updateSize(uint64_t sizeValue) {
  // size + creation + modification + timescale + dur + rate + vol +
  // res + res + matrix + predef + nextTrack
  if (CFullBox::version() == 0) {
    CFullBox::updateSize(sizeValue + 4 + 4 + 4 + 4 + 4 + 2 + 2 + 8 + 36 + 24 + 4);
  } else {
    CFullBox::updateSize(sizeValue + 8 + 8 + 8 + 8 + 4 + 2 + 2 + 8 + 36 + 24 + 4);
  }
}

SAttributeList CMovieHeaderBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Creation Time";
  attribute.value = tools::UTCTimeToString(m_creationTime);
  attributesList.push_back(attribute);

  attribute.key = "Modification Time";
  attribute.value = tools::UTCTimeToString(m_modificationTime);
  attributesList.push_back(attribute);

  attribute.key = "Timescale";
  attribute.value = std::to_string(m_timescale);
  attributesList.push_back(attribute);

  attribute.key = "Duration";
  attribute.value = std::to_string(m_duration);
  attributesList.push_back(attribute);

  attribute.key = "Rate";
  attribute.value = std::to_string(m_rate);
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

  attribute.key = "Next Track ID";
  attribute.value = std::to_string(m_nextTrackID);
  attributesList.push_back(attribute);

  return attributesList;
}

void CMovieHeaderBox::writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  if (CFullBox::version() == 0) {
    ilo::writeUint32_64(buffer, position, m_creationTime);
    ilo::writeUint32_64(buffer, position, m_modificationTime);
    ilo::writeUint32_64(buffer, position, m_timescale);
    ilo::writeUint32_64(buffer, position, m_duration);
  } else {
    ilo::writeUint64(buffer, position, m_creationTime);
    ilo::writeUint64(buffer, position, m_modificationTime);
    ilo::writeUint64(buffer, position, m_timescale);
    ilo::writeUint64(buffer, position, m_duration);
  }

  ilo::writeInt32(buffer, position, m_rate);
  ilo::writeInt16(buffer, position, m_volume);
  ilo::writeUint16(buffer, position, 0U);
  ilo::writeUint64(buffer, position, 0U);

  for (auto value : m_matrix) {
    ilo::writeInt32(buffer, position, value);
  }

  for (uint32_t i = 0; i < 6; ++i) {
    ilo::writeUint32(buffer, position, 0U);
  }

  ilo::writeUint32(buffer, position, m_nextTrackID);
}

void CMovieHeaderBox::sanityCheck() {
  ILO_ASSERT(m_nextTrackID != 0, "Field nextTrackID in mvhd box cannot be 0");

  if (m_creationTime > m_modificationTime) {
    ILO_LOG_WARNING(
        "Warning: Creation time in mvhd box of MP4 is smaller than modifcation time: %u, %u",
        m_creationTime, m_modificationTime);
  }
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(mvhd, CMovieHeaderBox, CMovieHeaderBox::SMvhdBoxWriteConfig,
                    CContainerType::noContainer);
