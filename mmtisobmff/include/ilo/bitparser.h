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

/*!
 * @file bitparser.h
 * @brief Class for reading bits from a byte buffer.
 */

#pragma once

// System includes
#include <iostream>

// Internal includes
#include "ilo/common_types.h"
#include "ilo/bittool_utils.h"

namespace ilo {
/*!
 * @brief Class for parsing a byte buffer bit-wise
 *
 * This class works on a given byte buffer and allows reading data in a bit-wise
 * fashion.
 *
 * @note All operations of this class are non-destructive. The buffer is never altered.
 *
 * <b>Example</b><br>
 * The example consists of two parts: One showing the actual bitparser
 * state, the other showing the appropriate code sections. The read
 * pointer is indicated with the green background. The fields affected by
 * the current operation are indicated with the yellow background.<br>
 * @code
 * // The bitparser object
 * CBitParser bitparser(buffer, nofValidBits);
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD style="background-color:
 * #2d7a31;">0</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>0</TD><TD>0</TD>
 *     <TD>0</TD><TD>1</TD><TD>1</TD><TD>0</TD><TD>1</TD><TD>1</TD>
 *     <TD>0</TD><TD>0</TD><TD>0</TD><TD>1</TD><TD>1</TD><TD>0</TD>
 *     <TD>1</TD><TD>1</TD></TR>
 * </TABLE>
 * @code
 * // Seek to index 5 of the bit buffer counting from the buffer start
 * bitparser.seek(5, ilo::EPosType::begin);
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD>0</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>0</TD><TD style="background-color:
 * #2d7a31;">0</TD> <TD>0</TD><TD>1</TD><TD>1</TD><TD>0</TD><TD>1</TD><TD>1</TD>
 *     <TD>0</TD><TD>0</TD><TD>0</TD><TD>1</TD><TD>1</TD><TD>0</TD>
 *     <TD>1</TD><TD>1</TD></TR>
 * </TABLE>
 * @code
 * // Read 8 bits as unsigned 8-bit integer
 * uint8_t data = bitparser.read<uint8_t>(8);
 * @endcode
 * <TABLE CELLSPACING="0" CELLPADDING="2" STYLE="margin-left: 20px;">
 * <TR><TD style="border: 0;">buffer</td>
 *     <TD>0</TD><TD>1</TD><TD>1</TD><TD>1</TD><TD>0</TD>
 *     <TD style="background-color: #9c9c1a;">0</TD>
 *     <TD style="background-color: #9c9c1a;">0</TD>
 *     <TD style="background-color: #9c9c1a;">1</TD>
 *     <TD style="background-color: #9c9c1a;">1</TD>
 *     <TD style="background-color: #9c9c1a;">0</TD>
 *     <TD style="background-color: #9c9c1a;">1</TD>
 *     <TD style="background-color: #9c9c1a;">1</TD>
 *     <TD style="background-color: #9c9c1a;">0</TD>
 *     <TD>0</TD><TD>0</TD><TD>1</TD><TD>1</TD><TD>0</TD>
 *     <TD>1</TD><TD>1</TD></TR>
 * </TABLE>
 *
 * @note Ensure the number of bits read is smaller or equal to the primitive data type used to store
 * the result.
 * @note Ensure to use primitives matching the expected signedness of the value that is read.
 *
 * \ingroup bittools
 */
class CBitParser {
 public:
  /*!
   * @brief Create parser from a byte buffer
   *
   * The byte buffer will not be copied or changed.
   *
   * @param externalBuffer The external buffer which will be parsed
   * @param nofValidBits The number of valid bits in the buffer. Used to specify byte-aligned
   * buffers with overhead at the end of the buffer. If 0, the whole buffer is considered to contain
   * valid data.
   */
  CBitParser(const ByteBuffer& externalBuffer, const uint32_t nofValidBits = 0);

  /*!
   * @brief Create parser from begin iterator and buffer length
   *
   * Data will not be copied or changed.
   *
   * @param begin The begin iterator at which the bit parser starts reading
   * @param size The length of the buffer bytes
   * @param nofValidBits The number of valid bits in the buffer. Used to specify byte-aligned
   * buffers with overhead at the end of the buffer. If 0, the whole buffer is considered to contain
   * valid data.
   */
  CBitParser(ByteBuffer::const_iterator& begin, const size_t size, const uint32_t nofValidBits = 0);

  /*!
   * @brief Create parser from begin and end iterator
   *
   * Data will not be copied or changed.
   *
   * @param begin The begin iterator at which the bit parser starts reading
   * @param end The end iterator at which the bit parser stops reading
   * @param nofValidBits The number of valid bits in the buffer. Used to specify byte-aligned
   * buffers with overhead at the end of the buffer. If 0, the whole buffer is considered to contain
   * valid data.
   */
  CBitParser(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end,
             const uint32_t nofValidBits = 0);

  /*!
   * @brief Create parser from a data pointer
   *
   * Data will not be copied or changed.
   *
   * @param buffer A pointer to the first byte of the buffer
   * @param nofValidBits The number of valid bits in the buffer. Required to indicate how much data
   * is available for reading. Can be smaller than real buffer to indicate padding at the end.
   *
   * @note nofValidBits shall never be 0 or bigger than the amount of bits available behind the data
   * pointer.
   */
  CBitParser(uint8_t* buffer, const uint32_t nofValidBits);

  /*!
   * @brief Frees all self allocated memory
   *
   * Will not alter any external buffers.
   */
  ~CBitParser();

  //! Disallow copy constructor
  CBitParser(const CBitParser& copyBuffer) = delete;

  //! Disallow assignment operator
  CBitParser& operator=(const CBitParser& copyBuffer) = delete;

  /*!
   * @brief Function to read data from the buffer
   *
   * This function needs to be called with a matching set of how many bits to read and in what
   * format.
   *
   * @param nofBits number of bits to read
   * @return Primitive of type T containing the read bits
   *
   * @note The chosen format type must be greater or equal to the number of bits read.
   * @note The chosen format must be of the correct signedness.
   *
   * @code
   * // Example 1: Read 12 bits as 16 bit unsigned integer
   * uint16_t data = parser.read<uint16_t>(12);
   *
   * // Example 2: Read 6 bits as 8 bit signed integer
   * int8_t data = parser.read<int8_t>(6);
   * @endcode
   */
  template <typename T, typename = uint8_t, typename = uint16_t, typename = uint32_t,
            typename = uint64_t, typename = int8_t, typename = int16_t, typename = int32_t,
            typename = int64_t>
  T read(uint32_t nnofBits) {
    iloAssertWithReadException(nnofBits <= nofBitsLeft(), "Not enough data left to parse.");

    if (nnofBits == 0) {
      return static_cast<T>(0);
    }

    bool isSigned = std::is_signed<T>::value;
    if (isSigned) {
      return signed_read<T>(nnofBits);
    } else {
      return unsigned_read<T>(nnofBits);
    }
  }

  /*!
   * @brief Function to seek to a specified bit position
   *
   * Will set the position for the read pointer. All subsequent calls to read functions are
   * affected.
   *
   * @param bitposition Number of bits to skip over relative to the selected position (see
   * fromPosition parameter).
   * @param fromPosition Position to start the seek operation from (beg, end, cur)
   *
   * @note When using ilo::EPosType::end or ilo::EPosType::cur, bitposition can also be a negative
   * value to indicate a backward seeking operation.
   */
  void seek(int32_t bitposition, ilo::EPosType fromPosition);

  /*!
   * @brief Function to get the reader's current position in bits
   *
   * @return The bit position of the read pointer from the beginning of the buffer.
   */
  uint32_t tell() const;

  /*!
   * @brief Function to get the total buffer size in bytes
   *
   * @return The total size of the buffer in bytes incl. byte alignment.
   *
   * @note The result of this function never changes, since it is based on the values given on the
   * constructor.
   */
  uint32_t nofBytes() const;

  //! Function to get access to the internal buffer pointer
  const uint8_t* internalBufferPtr();

  /*!
   * @brief Function to get the total buffer size in bits
   *
   * @return The total size of the buffer in bits without byte alignment.
   *
   * @note The result of this function never changes, since it is based on the values given on the
   * constructor.
   * @note The resulting value is not guaranteed to be byte aligned.
   */
  uint32_t nofBits() const;

  /*!
   * @brief Function to get the number of bits read
   *
   * Same as calling @ref tell
   */
  uint32_t nofReadBits() const;

  //! Function to get the number of bits left to read
  uint32_t nofBitsLeft() const;

  /*!
   * @brief Function to get end-of-file indication
   *
   * @return If true, the reader is at the end of the buffer.
   */
  bool eof() const;

 private:
  uint8_t readUint8(uint32_t nofBits);

  void iloAssertWithReadException(bool cond, std::string msg);
  void iloAssertWithSeekException(bool cond, std::string msg);

  template <typename T>
  T unsigned_read(uint32_t nnofBits) {
    bool isUint8 = std::is_same<T, uint8_t>::value;
    if (isUint8) {
      return readUint8(nnofBits);
    }

    // Basic error handling
    uint32_t maxNumBits = static_cast<uint32_t>(sizeof(T) * 8);
    iloAssertWithReadException(nnofBits <= maxNumBits,
                               "Number of bits does not fit into the given variable");

    T result = 0;
    // The bits which are in the first 8bit-field - all remaining can be added with 8 bit and
    // shift...
    uint32_t nonAlignedBits = nnofBits % 8;
    // Read the first non aligned bits:
    result = read<uint8_t>(nonAlignedBits);

    // Now we need a bit counter to decrease the number of bits to be written
    uint32_t bitCounter = nnofBits - nonAlignedBits;
    while (bitCounter >= 8) {
      bitCounter -= 8;
      result = (result << 8) | read<uint8_t>(8);
    }

    return result;
  }

  template <typename T>
  T signed_read(uint32_t nnofBits) {
    using unsigned_T = typename std::make_unsigned<T>::type;

    // Sanity check
    if (nnofBits == 0) {
      return 0;
    }

    // Temporal value to read into
    T tmpRead;
    // Mask for signed variable
    unsigned_T mask = static_cast<unsigned_T>(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFF)
                                              << static_cast<uint32_t>(nnofBits - 1));

    // Do actual read operation
    tmpRead = static_cast<T>(read<unsigned_T>(nnofBits));

    // Check for sign
    if ((tmpRead & mask) != 0) {
      tmpRead |= mask;
    }

    return tmpRead;
  }

 private:
  //! The buffer that stores the data
  const uint8_t* m_buffer;
  //! The read pointer
  uint32_t m_readIter;
  //! The number of read bits in current byte (max 7)
  uint32_t m_localReadBits;
  //! Number of valid bits. If m_nofvalidBits == 0, size() on m_buffer must be used instead. Always
  //! use nofvalidBits() instead of this variable.
  uint32_t m_nofValidBits;
};

std::ostream& operator<<(std::ostream& s, CBitParser bitparser);

}  // namespace ilo
