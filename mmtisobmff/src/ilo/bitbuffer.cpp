/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2005 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

// System includes
#include <algorithm>

// Internal includes
#include "ilo/bitbuffer.h"
#include "ilo_logging.h"

namespace ilo {
CBitBuffer::CBitBuffer(uint32_t initLengthInBytes)
    : m_useExtBuffer(false),
      m_internalBuffer(initLengthInBytes),
      m_buffer(m_internalBuffer.data()),
      m_extBufferSizeBytes(0u),
      m_writeIterBytes(0u),
      m_localWriteBits(0u),
      m_nofvalidBits(0u) {}

CBitBuffer::CBitBuffer(ilo::ByteBuffer& externalBuffer, uint32_t nofValidBits)
    : m_useExtBuffer(true),
      m_buffer(externalBuffer.data()),
      m_extBufferSizeBytes(externalBuffer.size()),
      m_writeIterBytes(0u),
      m_localWriteBits(0u) {
  m_nofvalidBits = nofValidBits;
}

CBitBuffer::CBitBuffer(uint8_t* buffer, size_t sizeBytes, uint32_t nofValidBits)
    : m_useExtBuffer(true),
      m_buffer(buffer),
      m_extBufferSizeBytes(sizeBytes),
      m_writeIterBytes(0u),
      m_localWriteBits(0u) {
  m_nofvalidBits = nofValidBits;
}

CBitBuffer::CBitBuffer(const CBitBuffer& copyBuffer)
    : m_useExtBuffer(copyBuffer.m_useExtBuffer),
      m_internalBuffer(copyBuffer.m_internalBuffer),
      m_buffer(copyBuffer.m_buffer),
      m_writeIterBytes(copyBuffer.m_writeIterBytes),
      m_localWriteBits(copyBuffer.m_localWriteBits),
      m_nofvalidBits(copyBuffer.m_nofvalidBits) {
  ILO_ASSERT(!copyBuffer.m_useExtBuffer,
             "BitBuffer copy constructor is not allowed for external buffers.");
}

CBitBuffer::~CBitBuffer() {}

void CBitBuffer::write(bool toWrite) {
  uint8_t value = toWrite;
  write(value, 1);
}

void CBitBuffer::append(const ilo::ByteBuffer& toAppend) {
  if (m_useExtBuffer) {
    ILO_ASSERT_WITH((m_extBufferSizeBytes * 8u) >= nofBits() + toAppend.size() * 8u,
                    AppendException,
                    "External Buffer size is not big enough to append the given byte buffer.");
  }

  uint32_t writePosBeforeBits = tell();
  if (writePosBeforeBits == nofBits()) {
    writePosBeforeBits += static_cast<uint8_t>(toAppend.size()) * 8u;
  }

  seek(0u, ilo::EPosType::end);

  for (const uint8_t& toWrite : toAppend) {
    write(toWrite, 8u);
  }

  seek(writePosBeforeBits, ilo::EPosType::begin);
}

void CBitBuffer::erase(uint32_t firstBit, uint32_t nnofBits) {
  uint32_t writePosBeforeBit = tell();

  uint32_t lastBit = firstBit + nnofBits;

  // basic error handling:
  ILO_ASSERT_WITH(lastBit <= nofBits(), EraseException, "The range to be erased is invalid.");

  CBitParser parser(m_buffer, nofBits());
  parser.seek(lastBit, ilo::EPosType::begin);

  uint32_t toReadBits = nofBits() - lastBit;

  // extract all bits after last item:
  CBitBuffer tmpBuffer;
  uint8_t reader;
  while (toReadBits > 0) {
    uint32_t localToReadBit = std::min(8u, toReadBits);
    reader = parser.read<uint8_t>(localToReadBit);
    tmpBuffer.write(reader, localToReadBit);

    toReadBits -= localToReadBit;
  }
  // resize the buffer to truncate the data behind position
  resize(firstBit);  // first shall not be included in remaining buffer

  // add the new data at end:
  seek(0, ilo::EPosType::end);

  // append the extracted old data:
  CBitParser tmpParser(tmpBuffer.bufferPtr(), tmpBuffer.nofBits());
  toReadBits = tmpBuffer.tell();

  while (toReadBits > 0) {
    uint32_t localToReadBit = std::min(8u, toReadBits);
    reader = tmpParser.read<uint8_t>(localToReadBit);
    write(reader, localToReadBit);

    toReadBits -= localToReadBit;
  }

  if (writePosBeforeBit < firstBit) {
    seek(writePosBeforeBit, ilo::EPosType::begin);
  } else if (writePosBeforeBit >= lastBit) {
    seek(writePosBeforeBit - nnofBits, ilo::EPosType::begin);
  } else {
    seek(firstBit, ilo::EPosType::begin);
  }
}

void CBitBuffer::resize(uint32_t newSizeInBits) {
  uint32_t writeIterPos = tell();
  if (newSizeInBits > nofBits()) {
    // seek to end with write pointer
    seek(0, ilo::EPosType::end);

    if (m_useExtBuffer) {
      ILO_ASSERT(m_extBufferSizeBytes * 8u >= newSizeInBits,
                 "New size exceeded external buffer size");
    } else {
      m_internalBuffer.resize((newSizeInBits + 7u) / 8u);
      m_buffer = m_internalBuffer.data();
    }

    // calculate number of bits to add
    uint32_t nnofBitsToAdd = newSizeInBits - nofBits();
    // add bits in bytewise manner
    while (nnofBitsToAdd > 0) {
      // we can add maximum 8 bits at a time
      uint32_t bitsForNow = std::min(8u, nnofBitsToAdd);
      // add zeros
      write(static_cast<uint8_t>(0), bitsForNow);

      // subtract added bits:
      nnofBitsToAdd -= bitsForNow;
    }
  } else if (newSizeInBits < nofBits()) {
    uint32_t newIter = newSizeInBits >> 3u;
    uint32_t newSizeInBytes = (newSizeInBits + 7u) >> 3u;

    if ((newSizeInBits % 8u) != 0) {
      uint32_t overhead = 8u - newSizeInBits % 8u;
      uint8_t mask = 0xFFu << overhead;

      m_buffer[newIter] &= mask;
      newIter++;
    }

    if (m_useExtBuffer) {
      for (size_t i = newIter; i < m_extBufferSizeBytes; i++) {
        m_buffer[i] = 0;
      }
    } else if (m_internalBuffer.size() != newSizeInBytes) {
      m_internalBuffer.resize(newSizeInBytes);
    }
  }

  m_nofvalidBits = newSizeInBits;
  seek(std::min(writeIterPos, m_nofvalidBits), EPosType::begin);
}

void CBitBuffer::seek(int32_t bitposition, ilo::EPosType fromPosition) const {
  int32_t bitOffset = 0;
  // calculate absolute position:
  switch (fromPosition) {
    case ilo::EPosType::begin:
      bitOffset = bitposition;
      break;
    case ilo::EPosType::cur:
      bitOffset = static_cast<int32_t>(m_localWriteBits) + bitposition +
                  static_cast<int32_t>(m_writeIterBytes << 3u);
      break;
    case ilo::EPosType::end:
      bitOffset = static_cast<int32_t>(nofBits()) + bitposition;
      break;
    default:
      ILO_ASSERT_WITH(false, SeekException, "Invalid seeking position found.");
      return;
      break;
  }

  // check absolute position:
  ILO_ASSERT_WITH(bitOffset >= 0, SeekException, "Seek to negative position.");
  auto absoluteBitPosition = static_cast<uint32_t>(bitOffset);
  ILO_ASSERT_WITH(absoluteBitPosition <= nofBits(), SeekException, "Seeking out of range.");

  // set the members:
  m_writeIterBytes = absoluteBitPosition >> 3u;
  m_localWriteBits = absoluteBitPosition & 0x07u;
}

void CBitBuffer::byteAlign() {
  // if we are not byte aligned:
  if (m_localWriteBits % 8 != 0) {
    uint8_t zeros = 0x00;
    uint32_t nnofBits = 8u - m_localWriteBits;
    write(zeros, nnofBits);
  }
}

uint32_t CBitBuffer::nofBytes() const {
  return (m_nofvalidBits + 7u) >> 3u;
}

uint8_t* CBitBuffer::bufferPtr() {
  if (!m_useExtBuffer) {
    return m_internalBuffer.data();
  }
  return m_buffer;
}

ilo::ByteBuffer CBitBuffer::bytebuffer() const {
  ILO_ASSERT(!m_useExtBuffer, "Conversion to bytebuffer only for internal buffer.");
  return m_internalBuffer;
}

uint32_t CBitBuffer::nofBits() const {
  return m_nofvalidBits;
}

// function to print out the whole bitbuffer bit-by-bit (for debugging)
std::ostream& operator<<(std::ostream& s, CBitBuffer bitbuffer) {
  CBitParser parser(bitbuffer.bufferPtr(), bitbuffer.nofBits());
  for (uint32_t i = 0; i < bitbuffer.nofBits(); i++) {
    s << parser.read<uint16_t>(1u);
  }

  return s;
}

uint32_t CBitBuffer::tell() const {
  return m_writeIterBytes * 8 + m_localWriteBits;
}

void CBitBuffer::reserve(uint32_t newCapacity) {
  // no reserve on external buffer
  ILO_ASSERT_WITH(!m_useExtBuffer, ReserveException, "Reserve only available for internal buffer.");
  m_internalBuffer.reserve(newCapacity);
}

void CBitBuffer::writeIntern(uint8_t toWrite, uint32_t nnofBits) {
  // basic error handling:
  ILO_ASSERT_WITH(nnofBits <= 8u, WriteException,
                  "Number of bits to read larger than the given data type (8Bit).");

  if (nnofBits == 0) {
    return;
  }

  uint32_t neededMemoryBits = tell() + nnofBits;

  // external buffer nothing shall be written beyond out of bounds
  if (m_useExtBuffer) {
    ILO_ASSERT_WITH(m_extBufferSizeBytes * 8u >= neededMemoryBits, WriteException,
                    "External buffer size is greater than needed memory to write in.");
  }

  // check capacity of buffer if we need more bytes than allocated
  if ((!m_useExtBuffer &&
       m_internalBuffer.size() * 8u < neededMemoryBits))  // we need to write out of buffers bounds?
  {
    // have to increase the size of the buffer:
    m_internalBuffer.resize((neededMemoryBits + 7u) / 8u);  // increase size by 1
    m_buffer = m_internalBuffer.data();
  }

  uint32_t shiftvalue = 16 - m_localWriteBits - nnofBits;

  // create mask
  uint16_t mask;
  mask = static_cast<uint16_t>(0xFFu >>
                               (8 - nnofBits));  // set the specified number of bits to one in mask
  mask = static_cast<uint16_t>(mask << shiftvalue);  // shift to start for writing bits
  mask = ~mask;

  uint16_t shiftBuffer = static_cast<uint16_t>(toWrite & (0xFFu >> (8 - nnofBits))) << shiftvalue;

  // write first value:
  m_buffer[m_writeIterBytes] = static_cast<uint8_t>(m_buffer[m_writeIterBytes] & (mask >> 8U));
  m_buffer[m_writeIterBytes] =
      static_cast<uint8_t>(m_buffer[m_writeIterBytes] | (shiftBuffer >> 8U));

  // set localWriteBits before they could be decreased:
  m_localWriteBits += nnofBits;

  // write second value if needed:
  if (~mask & 0xFFu) {
    m_buffer[m_writeIterBytes + 1] = m_buffer[m_writeIterBytes + 1] & (mask & 0xFFu);
    m_buffer[m_writeIterBytes + 1] = m_buffer[m_writeIterBytes + 1] | (shiftBuffer & 0xFFu);
  }

  // Should only happen if we write byte aligned
  m_writeIterBytes += m_localWriteBits / 8;
  m_localWriteBits %= 8;

  // adjust bit counters:
  m_nofvalidBits =
      std::max(tell(), m_nofvalidBits);  // get maximum of current write pos and nofvalidBits
}

void CBitBuffer::iloAssertWithWriteException(bool cond, std::string msg) {
  ILO_ASSERT_WITH(cond, WriteException, msg.c_str());
}

void CBitBuffer::iloAssertWithInsertException(bool cond, std::string msg) {
  ILO_ASSERT_WITH(cond, InsertException, msg.c_str());
}
}  // namespace ilo
