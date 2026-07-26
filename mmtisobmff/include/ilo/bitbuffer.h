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

/*!
 * @file bitbuffer.h
 * @brief Class for writing bits to a byte buffer.
 */

#pragma once

// System includes
#include <vector>
#include <iostream>

// Internal includes
#include "ilo/bitparser.h"
#include "ilo/common_types.h"
#include "ilo/bittool_utils.h"

namespace ilo {
/*!
 * @brief Class for writing a buffer bit-wise
 *
 * This class can operate on an existing external buffer or on an internally managed one and allows
 * features like bit-wise writing, inserting at specific positions and erasing.
 *
 * <b>Example</b><br>
 * The following examples consist of two parts: One showing the actual bitbuffer
 * content, the other showing the appropriate code sections. The write
 * pointer is indicated with the green background. The fields affected by
 * the current operation are indicated with the yellow background.
 *
 * @code
 * // Create a bitbuffer from an external buffer pre-filled with all 1s.
 * CBitBuffer bitbuffer(buffer.begin(), buffer.size());
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD style="background-color:
 * #2d7a31;">1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD></TR>
 * </TABLE>
 * @code
 * // Seek to index 5 of the bit buffer counting from the buffer start
 * bitbuffer.seek(5, ilo::EPosType::begin);
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD style="background-color:
 * #2d7a31;">1</TD> <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD>
 *     <TD>1</TD><TD>1</TD></TR>
 * </TABLE><br>
 * @code
 * // Write the value 0 as 32 bit unsigned integer value
 * bitbuffer.write(std::uint32_t{0U}, 32U);
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD style="background-color:
 * #9c9c1a;">0</TD> <TD style="background-color: #9c9c1a;">0</TD><TD>1</TD></TR>
 * </TABLE><br>
 *
 * \ingroup bittools
 */
class CBitBuffer {
 public:
  /*!
   * @brief Create writer with an internally managed buffer
   *
   * The buffer will be created/resized etc. on demand. Useful if the final size is not known
   * before.
   *
   * @param initLengthInBytes The initial length of the buffer in bytes (capacity for writing)
   *
   * @note Growing the buffer might be slow so the initLengthInBytes should be chosen wisely for
   * big data chunks.
   */
  explicit CBitBuffer(uint32_t initLengthInBytes = 0);

  /*!
   * @brief Create writer from an externally managed buffer
   *
   * Cannot be resized by CBitBuffer. Useful for large memory used by e.g. a sample grabber
   *
   * @param externalBuffer The external buffer which will be modified
   * @param nofValidBits The number of valid bits in the buffer from the beginning of the buffer.
   * Used to specify byte-aligned buffers with overhead at the end of the buffer. If 0, the whole
   * buffer is considered to contain valid data.
   */
  CBitBuffer(ilo::ByteBuffer& externalBuffer, uint32_t nofValidBits = 0);

  /*!
   * @brief Create writer from a data pointer
   *
   * Cannot be resized by CBitBuffer. Useful for large memory used by e.g. a sample grabber
   *
   * @param buffer The external buffer which will be modified
   * @param sizeBytes The length of the external buffer in bytes
   * @param nofValidBits The number of valid bits in the buffer from the beginning of the buffer.
   * Used to specify byte-aligned buffers with overhead at the end of the buffer. If 0, the whole
   * buffer is considered to contain valid data.
   */
  CBitBuffer(uint8_t* buffer, size_t sizeBytes, uint32_t nofValidBits = 0);

  /*!
   * @brief Copy constructor (not allowed for external buffer)
   *
   * Needed for all assignments and call by value. The copy constructor is often called implicitly
   * and may degrade performance. Try to avoid call by value, if not necessary.
   */
  CBitBuffer(const CBitBuffer& copyBuffer);

  /*!
   * @brief Frees all self allocated memory
   *
   * This does not alter the external buffer.
   */
  ~CBitBuffer();

  /*!
   * @brief Function to write a boolean value into the bit buffer
   *
   * Writes one bit from toWrite into current write position. The write position for following calls
   * will be increased by 1. Existing bits will be overwritten
   *
   * @param toWrite Value to write into the buffer
   */
  void write(bool toWrite);

  /*!
   * @brief Function to write into the bit buffer
   *
   * Writes least significant nofBits from toWrite into current write position. The write position
   * for following calls will be increased by nnofBits. Existing bits will be overwritten
   *
   * @param toWrite Value to write into the buffer
   * @param nnofBits Number of bits to write
   */
  template <typename T, typename = uint8_t, typename = uint16_t, typename = uint32_t,
            typename = uint64_t>
  void write(T toWrite, uint32_t nnofBits) {
    if (m_useExtBuffer) {
      iloAssertWithWriteException(
          m_extBufferSizeBytes * 8u >= tell() + nnofBits,
          "Number of bits to write is exceeding the available size of the buffer.");
    }

    size_t size = sizeof(T) * 8u;

    iloAssertWithWriteException(
        size >= nnofBits,
        "Number of bits to write is larger than the size of the value which is written.");

    // the bits, which are in the first 8bit-field - all remaining can be added with 8 bit and
    // shift...
    uint32_t nonAlignedBits = nnofBits & 0x07u;
    uint8_t writer =
        nnofBits == size ? 0u : static_cast<uint8_t>(toWrite >> (nnofBits - nonAlignedBits));

    // write the first non aligned bits:
    writeIntern(writer, nonAlignedBits);

    // now we need a bit counter to decrease the number of bits to be written
    uint32_t bitCounter = nnofBits - nonAlignedBits;
    while (bitCounter >= 8u) {
      bitCounter -= 8u;
      writer = static_cast<uint8_t>(0xFFu & (toWrite >> bitCounter));
      writeIntern(writer, 8u);
    }
  }

  /*!
   * @brief Append another byte buffer at the current write position.
   *
   * Function to append another byte buffer to the existing buffer at the current write position.
   * Overwrites the data following the current write position.
   *
   * @param toAppend Byte buffer to append
   */
  void append(const ilo::ByteBuffer& toAppend);

  /*!
   * @brief Function to insert a certain number of bits at a specific location
   *
   * @param toInsert Data to be written
   * @param before Integer position of the bit right behind the insertion point
   * @param nnofBits Number of bits to insert
   */
  template <typename T, typename = uint8_t, typename = uint16_t, typename = uint32_t,
            typename = uint64_t>
  void insert(T toInsert, uint32_t before, uint32_t nnofBits) {
    // basic error handling
    iloAssertWithInsertException(before <= nofBits(), "Insert position is out of range.");

    if (m_useExtBuffer) {
      iloAssertWithInsertException(m_extBufferSizeBytes * 8u >= nofBits() + nnofBits,
                                   "External buffer too small to insert.");
    }

    uint32_t writePosBefore = tell();
    uint32_t toReadBits = nofBits() - before;

    CBitParser parser(&m_buffer[before >> 3u], toReadBits + before % 8u);
    parser.seek(before % 8u, EPosType::begin);

    // extract all bits after specified position:
    CBitBuffer tmpBuffer;
    uint8_t reader;

    while (toReadBits > 0) {
      uint32_t chunkSizeBits = std::min(8u, toReadBits);
      reader = parser.read<uint8_t>(chunkSizeBits);

      tmpBuffer.write<uint8_t>(reader, chunkSizeBits);

      toReadBits -= chunkSizeBits;
    }

    // resize the buffer to truncate the data behind position
    resize(before);

    // add the new data at end:
    seek(0, ilo::EPosType::end);

    // write the bits to insert
    write<T>(toInsert, nnofBits);

    CBitParser tmpParser(tmpBuffer.bufferPtr(), tmpBuffer.nofBits());

    // append the extracted old data:
    uint32_t writtenBits = tmpBuffer.nofBits();
    while (writtenBits > 0) {
      uint32_t chunkSizeBits = std::min(8u, writtenBits);
      reader = tmpParser.read<uint8_t>(chunkSizeBits);

      write(reader, chunkSizeBits);

      writtenBits -= chunkSizeBits;
    }

    if (writePosBefore < before) {
      seek(writePosBefore, ilo::EPosType::begin);
    } else if (writePosBefore >= before) {
      seek(writePosBefore + nnofBits, ilo::EPosType::begin);
    }
  }

  /*!
   * @brief Function to erase a certain bit interval
   *
   * Bits are erased including position "first", excluding position "last".
   * This scheme corresponds with STL iterator based functions, noted as [first, last).
   *
   * @param firstBit Position of first bit to be erased
   * @param nnofBits Number of bits to be erased
   */
  void erase(uint32_t firstBit, uint32_t nnofBits);

  /*!
   * @brief Function to resize the bitbuffer to the specified length in bits
   *
   * Similar to byte buffers resize function, this function might reallocate the memory buffer.
   *
   * @param newSizeInBits New size for the bitbuffer, everything behind the newSize bit will be
   * truncated. If the new size is larger than previous one, the appended bits will be zero
   */
  void resize(uint32_t newSizeInBits);

  /*!
   * @brief Function to seek to a specified bit position
   *
   * Will set the position for the write pointer. All subsequent calls to write functions are
   * affected.
   *
   * @param bitposition Number of bits to seek over
   * @param fromPosition Position where to start the seek operation from (beg, end, cur)
   *
   * @note When using ilo::EPosType::end or ilo::EPosType::cur, bitposition can also be a negative
   * value to indicate a backward seeking operation.
   */
  void seek(int32_t bitposition, ilo::EPosType fromPosition) const;

  /*!
   * @brief Function to get the writer's bitposition
   *
   * @return The current writers bit position from the beginning of the buffer.
   */
  uint32_t tell() const;

  /*!
   * @brief Function to reserve some data in the internal buffer without setting the buffers size -
   * only its capacity
   *
   * If newCapacity is bigger than the current max capacity, the capacity of the buffer will be
   * increased. This will not alter the current fill state of the buffer.
   */
  void reserve(uint32_t newCapacity);

  /*!
   * @brief Function to align write pointer to byte border
   *
   * Aligns the bitbuffer to the next full byte by filling zeros into buffer.
   * Does nothing if buffer is already byte aligned
   */
  void byteAlign();

  /*!
   * @brief Function to get the size of the bitbuffer in bytes
   *
   * @return Size in bytes of bitbuffer
   */
  uint32_t nofBytes() const;

  /*!
   * @brief Function to get access to internal buffer
   *
   * @return Pointer to internal byte buffer
   */
  uint8_t* bufferPtr();

  /*!
   * @brief Function to get a copy of the internal buffer
   *
   * @return Copy of internal byte buffer
   */
  ilo::ByteBuffer bytebuffer() const;

  /*!
   * @brief Function to get number of bits in buffer (may not be byte aligned)
   *
   * Returning all bits (even unaligned ones) contained in the bit buffer.
   * Including initially added bits and bits inserted at the end. The nofBytes
   * value is not necessarily the same as nofBits()/8 in case of unaligned buffers.
   */
  uint32_t nofBits() const;

 private:
  void writeIntern(uint8_t toWrite, uint32_t nnofBits);

  void iloAssertWithWriteException(bool cond, std::string msg);
  void iloAssertWithInsertException(bool cond, std::string msg);

  //! Indicates whether we use an external buffer or not
  bool m_useExtBuffer;
  //! The internal buffer if no external one was given
  ilo::ByteBuffer m_internalBuffer;
  //! The buffer which stores the data
  uint8_t* m_buffer;
  //! The size of the external buffer
  size_t m_extBufferSizeBytes;
  //! The write pointer
  mutable uint32_t m_writeIterBytes;
  //! The number of written bits in current byte (max 7)
  mutable uint32_t m_localWriteBits;
  //! Number of valid bits
  uint32_t m_nofvalidBits;

};  // CBitBuffer

std::ostream& operator<<(std::ostream& s, ilo::CBitBuffer bitbuffer);

}  // namespace ilo
