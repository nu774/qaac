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
 * Content: abstract box class
 */

// System headers
#include <limits>

// External headers
#include "ilo/bytebuffertools.h"
#include "ilo/string_utils.h"

// Internal headers
#include "box.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CBox::CBox(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end)
    : m_size(0), m_type(ilo::toFcc("0000")) {
  CBox::parse(begin, end);
}

CBox::CBox(const SBoxWriteConfig& boxData)
    : m_size(0), m_type(boxData.getType()), m_had64BitSizeInInput(boxData.force64BitSizeExt) {
  updateSize(0);
}

void CBox::updateSize(uint64_t value) {
  if (value <= std::numeric_limits<uint32_t>::max() && !m_had64BitSizeInInput) {
    m_size = value + 8;
  } else {
    m_size = value + 8 + 8;
  }
}

SAttributeList CBox::getAttributeList() const {
  ILO_ASSERT_WITH(false, std::runtime_error, "getAttributeList is not implemented for this box");
  return SAttributeList();
}

bool CBox::hasLargeSize() const {
  if (size() <= std::numeric_limits<uint32_t>::max()) {
    return false;
  } else {
    return true;
  }
}

void CBox::parse(ilo::ByteBuffer::const_iterator& begin,
                 const ilo::ByteBuffer::const_iterator& end) {
  m_size = ilo::readUint32(begin, end);
  m_type = ilo::readFourCC(begin, end);

  switch (m_size) {
    case (0x00U):
      m_size = static_cast<uint64_t>(end - begin + 8u);  // 8 bytes were already read above
      break;

    case (0x01U):
      m_had64BitSizeInInput = true;
      m_size = ilo::readUint64(begin, end);
      break;

    default:
      break;
  }

  return;
}

void CBox::write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  writeHeader(buffer, position);
  writeBox(buffer, position);
}

void CBox::writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  if (!hasLargeSize()) {
    ilo::writeUint32_64(buffer, position, m_size);
    ilo::writeFourCC(buffer, position, m_type);
  } else {
    ilo::writeUint32(buffer, position, 1U);
    ilo::writeFourCC(buffer, position, m_type);
    ilo::writeUint64(buffer, position, m_size);
  }
}

CFullBox::CFullBox(ilo::ByteBuffer::const_iterator& begin,
                   const ilo::ByteBuffer::const_iterator& end)
    : CBox(begin, end), m_version(0), m_flags(0) {
  CFullBox::parse(begin, end);
}

CFullBox::CFullBox(const SFullBoxWriteConfig& fullBoxData) : CBox(fullBoxData) {
  m_version = fullBoxData.version;
  m_flags = fullBoxData.flags;

  updateSize(0);
}

void CFullBox::updateSize(uint64_t sizeValue) {
  // size + version + flags
  CBox::updateSize(sizeValue + 1 + 3);
}

void CFullBox::parse(ilo::ByteBuffer::const_iterator& begin,
                     const ilo::ByteBuffer::const_iterator& end) {
  m_version = ilo::readUint8(begin, end);
  m_flags = ilo::readUint24(begin, end);
}

void CFullBox::writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  CBox::writeHeader(buffer, position);

  ilo::writeUint8(buffer, position, m_version);
  ilo::writeUint24(buffer, position, m_flags);
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt
