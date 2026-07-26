/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2006 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

// Internal includes
#include "ilo/bitparser.h"
#include "ilo_logging.h"

namespace ilo {
CBitParser::CBitParser(const ilo::ByteBuffer& externalBuffer, const uint32_t nofValidBits)
    : m_buffer(externalBuffer.data()), m_readIter(0U), m_localReadBits(0U) {
  m_nofValidBits = nofValidBits;
  if (m_nofValidBits == 0) {
    m_nofValidBits = static_cast<uint32_t>(externalBuffer.size()) * 8;
  }
}

CBitParser::CBitParser(ByteBuffer::const_iterator& begin, const size_t size,
                       const uint32_t nofValidBits)
    : m_buffer(&begin[0]), m_readIter(0U), m_localReadBits(0U) {
  m_nofValidBits = nofValidBits;
  if (m_nofValidBits == 0) {
    m_nofValidBits = static_cast<uint32_t>(size) * 8;
  }
}

CBitParser::CBitParser(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end,
                       const uint32_t nofValidBits)
    : m_buffer(&begin[0]), m_readIter(0U), m_localReadBits(0U) {
  m_nofValidBits = nofValidBits;
  if (m_nofValidBits == 0) {
    m_nofValidBits = static_cast<uint32_t>(end - begin) * 8;
  }
}

CBitParser::CBitParser(uint8_t* buffer, const uint32_t nofValidBits)
    : m_buffer(buffer), m_readIter(0U), m_localReadBits(0U), m_nofValidBits(nofValidBits) {}

CBitParser::~CBitParser() {}

uint8_t CBitParser::readUint8(uint32_t nnofBits) {
  uint8_t result = 0;

  // basic error handling (nnofBits > 8)
  ILO_ASSERT_WITH(nnofBits <= 8 && nnofBits > 0, ReadException,
                  "Number of bits to write are not within the range of uint8_t.");

  // check bounds:
  ILO_ASSERT_WITH(m_nofValidBits >= (m_readIter << 3) + m_localReadBits + nnofBits, ReadException,
                  "EOF reached.");

  // check if we are inbetween one byte
  if (m_localReadBits + nnofBits <= 8) {
    // create mask
    uint8_t mask = 0xFFu;
    // move mask to last read pointer
    mask = mask >> m_localReadBits;
    // get the current buffer character:
    result = m_buffer[m_readIter] & mask;
    // move the essential bits to correct position:
    result = result >> (8 - (nnofBits + m_localReadBits));
  } else {
    uint16_t mask = 0xFFFFu;
    // move mask to last read bit ptr
    mask = mask >> m_localReadBits;
    // fill shift buffer:
    uint16_t shiftBuffer =
        static_cast<uint16_t>(m_buffer[m_readIter]) << 8 | m_buffer[m_readIter + 1];
    // mask out already read bits:
    shiftBuffer = shiftBuffer & mask;
    // move the essential bits to correct position:
    shiftBuffer = shiftBuffer >> (16 - (nnofBits + m_localReadBits));
    // assign to output
    result = static_cast<uint8_t>(shiftBuffer);
  }
  m_localReadBits += nnofBits;
  if (m_localReadBits > 8) {
    m_localReadBits -= 8;
    m_readIter++;
  }

  return result;
}

void CBitParser::seek(int32_t bitposition, ilo::EPosType fromPosition) {
  int32_t bitOffset = 0;
  // calculate absolute position:
  switch (fromPosition) {
    case ilo::EPosType::begin:
      bitOffset = bitposition;
      break;
    case ilo::EPosType::cur:
      bitOffset = static_cast<int32_t>(m_localReadBits) + bitposition +
                  static_cast<int32_t>(m_readIter << 3);
      break;
    case ilo::EPosType::end:
      bitOffset = static_cast<int32_t>(m_nofValidBits) + bitposition;
      break;
    default:
      ILO_ASSERT_WITH(false, SeekException, "Invalid seeking position found.");
      break;
  }
  // check absolute position:
  ILO_ASSERT_WITH(bitOffset >= 0, SeekException, "Seek to negative position.");
  auto absoluteBitPosition = static_cast<uint32_t>(bitOffset);
  ILO_ASSERT_WITH(absoluteBitPosition <= m_nofValidBits, SeekException, "Seeking out of range.");

  // set the members:
  m_readIter = absoluteBitPosition >> 3;
  m_localReadBits = absoluteBitPosition & 0x07;
}

uint32_t CBitParser::nofBytes() const {
  return (m_nofValidBits + 7) >> 3;
}

const uint8_t* CBitParser::internalBufferPtr() {
  return m_buffer;
}

// get the nuber of bits in the buffer (not byte aligned)
uint32_t CBitParser::nofBits() const {
  return m_nofValidBits;
}

// function to print out the whole bitbuffer bit-by-bit (for debugging)
std::ostream& operator<<(std::ostream& s, CBitParser bitparser) {
  uint32_t reader;
  uint32_t old_pos = bitparser.tell();
  bitparser.seek(0, ilo::EPosType::begin);
  while ((reader = bitparser.read<uint32_t>(1)) != 0) {
    s << reader;
  }
  bitparser.seek(old_pos, ilo::EPosType::begin);

  return s;
}

// function to get eof indication:
bool CBitParser::eof() const {
  return nofReadBits() >= m_nofValidBits;
}

// function to get position of reader
uint32_t CBitParser::tell() const {
  return nofReadBits();
}

// function to get number of read bits
uint32_t CBitParser::nofReadBits() const {
  return (m_readIter << 3) + m_localReadBits;
}

uint32_t CBitParser::nofBitsLeft() const {
  return nofBits() - nofReadBits();
}

void CBitParser::iloAssertWithReadException(bool cond, std::string msg) {
  ILO_ASSERT_WITH(cond, ReadException, msg.c_str());
}

void CBitParser::iloAssertWithSeekException(bool cond, std::string msg) {
  ILO_ASSERT_WITH(cond, SeekException, msg.c_str());
}

}  // namespace ilo
