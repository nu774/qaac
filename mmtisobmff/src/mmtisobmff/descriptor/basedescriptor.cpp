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
 * Content: base descriptor class
 */

// System includes
#include <limits>
#include <deque>

// External includes
#include "ilo/bytebuffertools.h"

// Internal includes
#include "basedescriptor.h"
#include "common/logging.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace descriptor {
CBaseDescriptor::CBaseDescriptor(ilo::ByteBuffer::const_iterator& begin,
                                 const ilo::ByteBuffer::const_iterator& end)
    : m_sizeOfInstance(0), m_tag(EDescriptorTag::Forbidden) {
  CBaseDescriptor::parse(begin, end);
}

CBaseDescriptor::CBaseDescriptor(const SBaseDescriptorWriteConfig& baseDescriptorData)
    : m_sizeOfInstance(0), m_tag(baseDescriptorData.getType()) {
  updateSize(0);
}

void CBaseDescriptor::updateSize(uint32_t instanceSize) {
  m_sizeOfInstance = instanceSize;
}

void CBaseDescriptor::parse(ilo::ByteBuffer::const_iterator& begin,
                            const ilo::ByteBuffer::const_iterator& end) {
  m_tag = static_cast<EDescriptorTag>(readUint8(begin, end));

  m_sizeOfInstance = 0;
  uint8_t tmpByte = readUint8(begin, end);
  uint8_t nextByte = (tmpByte & 0x80) >> 7;
  m_sizeOfInstance = tmpByte & 0x7F;

  while (nextByte) {
    tmpByte = readUint8(begin, end);
    nextByte = (tmpByte & 0x80) >> 7;
    uint8_t sizeByte = tmpByte & 0x7F;

    m_sizeOfInstance = m_sizeOfInstance << 7 | sizeByte;
  }

  ILO_ASSERT_WITH(begin + size() <= end, std::out_of_range, "descriptor size exceeds input");
}

void CBaseDescriptor::write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const {
  writeBaseDescriptor(buffer, position);
  writeDescriptor(buffer, position);
}

EDescriptorTag CBaseDescriptor::peekTag(ilo::ByteBuffer::const_iterator& begin,
                                        const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT(end - begin >= 1, "There must be at least a tag in the buffer");

  EDescriptorTag descTag = static_cast<EDescriptorTag>(readUint8(begin, end));
  begin -= 1;

  return descTag;
}

SAttributeList CBaseDescriptor::getAttributeList() const {
  ILO_ASSERT_WITH(false, std::runtime_error,
                  "getAttributeList is not implemented for this descriptor");
  return SAttributeList();
}

void CBaseDescriptor::writeBaseDescriptor(ilo::ByteBuffer& buffer,
                                          ilo::ByteBuffer::iterator& position) const {
  writeUint8(buffer, position, static_cast<uint8_t>(m_tag));

  uint8_t writeByte = 0;
  uint64_t instance = m_sizeOfInstance;

  uint8_t numBytes = static_cast<uint8_t>(m_sizeOfInstance / MAX_SIZE_IN_ONE_BYTE);
  numBytes += (m_sizeOfInstance % MAX_SIZE_IN_ONE_BYTE) ? (1) : (0);

  std::deque<uint8_t> bytesToWrite;

  for (uint8_t i = 0; i < numBytes - 1; i++) {
    writeByte = instance & 0x7f;
    instance >>= 7;
    bytesToWrite.push_front(writeByte);
  }
  writeByte = instance & 0x7f;
  if (!bytesToWrite.empty()) {
    writeByte |= 0x80;
  }
  writeUint8(buffer, position, writeByte);

  for (uint8_t i = 0; i < bytesToWrite.size(); ++i) {
    if (i < bytesToWrite.size() - 1) {
      bytesToWrite[i] |= 0x80;
    }
    writeUint8(buffer, position, bytesToWrite[i]);
  }
}
}  // namespace descriptor
}  // namespace isobmff
}  // namespace mmt
