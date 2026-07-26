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

// System includes
#include <stdexcept>
#include <string>
#include <limits>
#include <ctype.h>
#include <algorithm>

// Internal includes
#include "ilo/bytebuffertools.h"
#include "ilo/string_utils.h"
#include "ilo_logging.h"

namespace ilo {
uint64_t readUint64(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  uint32_t lowValue, highValue;

  highValue = readUint32(buffer, position);
  lowValue = readUint32(buffer, position);

  uint64_t retval = static_cast<uint64_t>(highValue) << 32 | static_cast<uint64_t>(lowValue);
  return retval;
}

int64_t readInt64(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  return static_cast<int64_t>(readUint64(buffer, position));
}

uint32_t readUint32(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 4) {
    throw std::out_of_range("Read position out of bounds");
  }
  uint32_t retval =
      (*position << 24) | (*(position + 1) << 16) | (*(position + 2) << 8) | (*(position + 3));

  position += 4;
  return retval;
}

int32_t readInt32(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  uint32_t temp = 0;
  int32_t retval = 0;

  temp = readUint32(buffer, position);
  retval = static_cast<int32_t>(temp);

  return retval;
}

uint32_t readUint24(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 3) {
    throw std::out_of_range("Read position out of bounds");
  }
  uint32_t retval = (*position << 16) | (*(position + 1) << 8) | (*(position + 2));

  position += 3;
  return retval;
}

uint16_t readUint16(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 2) {
    throw std::out_of_range("Read position out of bounds");
  }
  uint16_t retval = static_cast<uint16_t>((*position << 8) | (*(position + 1)));
  position += 2;
  return retval;
}

int16_t readInt16(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  uint16_t temp = 0;
  int16_t retval = 0;

  temp = readUint16(buffer, position);
  retval = static_cast<int16_t>(temp);

  return retval;
}

uint8_t readUint8(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 1) {
    throw std::out_of_range("Read position out of bounds");
  }
  uint8_t retval = *position++;
  return retval;
}

Fourcc readFourCCRaw(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 4) {
    throw std::out_of_range("Read position out of bounds");
  }
  Fourcc retval;
  std::copy(position, position + 4, retval.begin());
  position += 4;

  return retval;
}

Fourcc readFourCC(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  auto retval = readFourCCRaw(buffer, position);

  if (std::any_of(retval.begin(), retval.end(),
#if CHAR_MIN == 0
                  [&](char c) { return !isprint(c); }))
#else
      [&](char c) { return ((c >= 0 && !isprint(c)) || c < 0); }))
#endif
  {
    ILO_LOG_WARNING("Character in fourCC %s is not printable", toString(retval).c_str());
  }
  return retval;
}

IsoLang readIsoLang(const ByteBuffer& buffer, ByteBuffer::const_iterator& position) {
  if (buffer.begin() > position || buffer.end() - position < 2) {
    throw std::out_of_range("Read position out of bounds");
  }
  IsoLang retVal;

  uint16_t padAndLanguage = readUint16(buffer, position);

  if ((padAndLanguage >> 15) != 0) {
    ILO_LOG_WARNING(
        "Warning: While reading IsoLang, dirty padding was found. Padding will be ignored");
    padAndLanguage &= 0x7FFF;  // Clears the pad bit to 0
  }

  int8_t first = (static_cast<int8_t>((padAndLanguage & 0x7C00u) >> 10)) + 0x60;
  int8_t second = (static_cast<int8_t>((padAndLanguage & 0x03E0u) >> 5)) + 0x60;
  int8_t third = (static_cast<int8_t>((padAndLanguage & 0x001Fu))) + 0x60;

  std::vector<int8_t> tmp = {first, second, third};
  std::copy_n(tmp.begin(), 3, retVal.begin());

  if (std::any_of(retVal.begin(), retVal.end(), [](char c) { return !isprint(c); })) {
    throw std::runtime_error("Isolang parsing failed");
  }
  return retVal;
}

std::string readString(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                       uint64_t maxLength) {
  ILO_ASSERT_WITH(buffer.begin() <= position && buffer.end() - position > 0, std::out_of_range,
                  "Read position out of bounds");

  auto begin = position;
  while (*position != '\0' &&
         (static_cast<uint64_t>(position - begin) <= maxLength || maxLength == 0)) {
    if (++position == buffer.end()) {
      throw std::out_of_range("Null termination is missing");
    }
  }
  return std::string(begin, position++);
}

std::vector<uint32_t> readUint32Array(const ByteBuffer& buffer,
                                      ByteBuffer::const_iterator& position, uint32_t count) {
  if (buffer.begin() > position || buffer.end() - position < static_cast<int32_t>(count * 4)) {
    throw std::out_of_range("Read position out of bounds");
  }

  std::vector<uint32_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readUint32(buffer, position);
  }
  return resultVector;
}

std::vector<int32_t> readInt32Array(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                                    uint32_t count) {
  if (buffer.begin() > position || buffer.end() - position < static_cast<int32_t>(count * 4)) {
    throw std::out_of_range("Read position out of bounds");
  }

  std::vector<int32_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readInt32(buffer, position);
  }
  return resultVector;
}

std::vector<uint8_t> readUint8Array(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                                    uint32_t count) {
  if (buffer.begin() > position || buffer.end() - position < static_cast<int32_t>(count)) {
    throw std::out_of_range("Read position out of bounds");
  }

  std::vector<uint8_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readUint8(buffer, position);
  }
  return resultVector;
}

uint64_t readUint64(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  uint32_t lowValue, highValue;

  highValue = readUint32(begin, end);
  lowValue = readUint32(begin, end);

  uint64_t retval = static_cast<uint64_t>(highValue) << 32 | static_cast<uint64_t>(lowValue);
  return retval;
}

int64_t readInt64(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  return static_cast<int64_t>(readUint64(begin, end));
}

uint32_t readUint32(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Read position out of bounds");

  uint32_t retval = (*begin << 24) | (*(begin + 1) << 16) | (*(begin + 2) << 8) | (*(begin + 3));

  begin += 4;
  return retval;
}

int32_t readInt32(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  uint32_t temp = 0;
  int32_t retval = 0;

  temp = readUint32(begin, end);
  retval = static_cast<int32_t>(temp);

  return retval;
}

uint32_t readUint24(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 3, std::out_of_range, "Read position out of bounds");
  uint32_t retval = (*begin << 16) | (*(begin + 1) << 8) | (*(begin + 2));

  begin += 3;
  return retval;
}

uint16_t readUint16(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 2, std::out_of_range, "Read position out of bounds");

  uint16_t retval = static_cast<uint16_t>((*begin << 8) | (*(begin + 1)));
  begin += 2;
  return retval;
}

int16_t readInt16(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  uint16_t temp = 0;
  int16_t retval = 0;

  temp = readUint16(begin, end);
  retval = static_cast<int16_t>(temp);

  return retval;
}

uint8_t readUint8(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin, std::out_of_range, "Read position out of bounds");

  uint8_t retval = *begin++;
  return retval;
}

Fourcc readFourCCRaw(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Read position out of bounds");

  Fourcc retval;
  std::copy(begin, begin + 4, retval.begin());
  begin += 4;

  return retval;
}

Fourcc readFourCC(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  auto retval = readFourCCRaw(begin, end);

  if (std::any_of(retval.begin(), retval.end(),
#if CHAR_MIN == 0
                  [&](char c) { return !isprint(c); }))
#else
      [&](char c) { return ((c >= 0 && !isprint(c)) || c < 0); }))
#endif
  {
    ILO_LOG_WARNING("Character in fourCC %s is not printable", toString(retval).c_str());
  }
  return retval;
}

IsoLang readIsoLang(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 2, std::out_of_range, "Read position out of bounds");

  IsoLang retVal;

  uint16_t padAndLanguage = readUint16(begin, end);

  if ((padAndLanguage >> 15) != 0) {
    ILO_LOG_WARNING(
        "Warning: While reading IsoLang, dirty padding was found. Padding will be ignored");
    padAndLanguage &= 0x7FFF;  // Clears the pad bit to 0
  }

  int8_t first = (static_cast<int8_t>((padAndLanguage & 0x7C00u) >> 10)) + 0x60;
  int8_t second = (static_cast<int8_t>((padAndLanguage & 0x03E0u) >> 5)) + 0x60;
  int8_t third = (static_cast<int8_t>((padAndLanguage & 0x001Fu))) + 0x60;

  std::vector<int8_t> tmp = {first, second, third};
  std::copy_n(tmp.begin(), 3, retVal.begin());

  if (std::any_of(retVal.begin(), retVal.end(), [](char c) { return !isprint(c); })) {
    throw std::runtime_error("Isolang parsing failed");
  }
  return retVal;
}

std::string readString(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end,
                       uint64_t maxLength) {
  ILO_ASSERT_WITH(end - begin > 0, std::out_of_range, "Read position out of bounds");

  auto str_begin = begin;
  while (*begin != '\0' &&
         (static_cast<uint64_t>(begin - str_begin) <= maxLength || maxLength == 0)) {
    ILO_ASSERT_WITH(++begin != end, std::out_of_range, "Null termination is missing");
  }
  return std::string(str_begin, begin++);
}

std::string readStringNonStrict(ByteBuffer::const_iterator& begin,
                                const ByteBuffer::const_iterator& end, uint64_t maxLength) {
  ILO_ASSERT_WITH(end - begin > 0, std::out_of_range, "Read position out of bounds");

  auto str_begin = begin;
  while (*begin != '\0' &&
         (static_cast<uint64_t>(begin - str_begin) <= maxLength || maxLength == 0)) {
    if (++begin == end) {
      ILO_LOG_WARNING("Null termination is missing");
      return std::string(str_begin, begin);
    }
  }
  return std::string(str_begin, begin++);
}

std::vector<uint32_t> readUint32Array(ByteBuffer::const_iterator& begin,
                                      const ByteBuffer::const_iterator& end, uint32_t count) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(4 * count), std::out_of_range,
                  "Read position out of bounds");

  std::vector<uint32_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readUint32(begin, end);
  }
  return resultVector;
}

std::vector<int32_t> readInt32Array(ByteBuffer::const_iterator& begin,
                                    const ByteBuffer::const_iterator& end, uint32_t count) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(4 * count), std::out_of_range,
                  "Read position out of bounds");

  std::vector<int32_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readInt32(begin, end);
  }
  return resultVector;
}

std::vector<uint8_t> readUint8Array(ByteBuffer::const_iterator& begin,
                                    const ByteBuffer::const_iterator& end, uint32_t count) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(count), std::out_of_range,
                  "Read position out of bounds");

  std::vector<uint8_t> resultVector(count);

  for (uint32_t i = 0; i < count; ++i) {
    resultVector[i] = readUint8(begin, end);
  }
  return resultVector;
}

// Tools for writing

void writeUint64(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint64_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 8) {
    throw std::out_of_range("Write position out of bounds");
  }

  uint32_t high = static_cast<uint32_t>((valueToWrite & 0xFFFFFFFF00000000ULL) >> 32U);
  uint32_t low = static_cast<uint32_t>(valueToWrite & 0xFFFFFFFF);

  writeUint32(buffer, position, high);
  writeUint32(buffer, position, low);
}

void writeInt64(ByteBuffer& buffer, ByteBuffer::iterator& position, const int64_t valueToWrite) {
  writeUint64(buffer, position, static_cast<uint64_t>(valueToWrite));
}

void writeUint32(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint32_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 4) {
    throw std::out_of_range("Write position out of bounds");
  }
  *position++ = static_cast<uint8_t>(valueToWrite >> 24);
  *position++ = static_cast<uint8_t>(valueToWrite >> 16 & 0xFF);
  *position++ = static_cast<uint8_t>(valueToWrite >> 8 & 0xFF);
  *position++ = static_cast<uint8_t>(valueToWrite & 0xFF);
}

void writeUint32_64(ByteBuffer& buffer, ByteBuffer::iterator& position,
                    const uint64_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 4) {
    throw std::out_of_range("Write position out of bounds");
  }

  if (valueToWrite > std::numeric_limits<uint32_t>::max()) {
    throw std::out_of_range("Can't write a 32Bit value from a 64Bit value without truncating data");
  }

  uint32_t low = valueToWrite & 0xFFFFFFFFu;

  *position++ = static_cast<uint8_t>(low >> 24);
  *position++ = static_cast<uint8_t>(low >> 16 & 0xFFu);
  *position++ = static_cast<uint8_t>(low >> 8 & 0xFFu);
  *position++ = static_cast<uint8_t>(low & 0xFFu);
}

void writeInt32(ByteBuffer& buffer, ByteBuffer::iterator& position, const int32_t valueToWrite) {
  writeUint32(buffer, position, static_cast<uint32_t>(valueToWrite));
}

void writeUint24(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint32_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 3) {
    throw std::out_of_range("Write position out of bounds");
  }
  *position++ = static_cast<uint8_t>(valueToWrite >> 16);
  *position++ = static_cast<uint8_t>(valueToWrite >> 8 & 0xFF);
  *position++ = static_cast<uint8_t>(valueToWrite & 0xFF);
}

void writeUint16(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint16_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 2) {
    throw std::out_of_range("Write position out of bounds");
  }
  *position++ = static_cast<uint8_t>(valueToWrite >> 8);
  *position++ = static_cast<uint8_t>(valueToWrite);
}

void writeInt16(ByteBuffer& buffer, ByteBuffer::iterator& position, const int16_t valueToWrite) {
  writeUint16(buffer, position, static_cast<uint16_t>(valueToWrite));
}

void writeUint8(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint8_t valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 1) {
    throw std::out_of_range("Write position out of bounds");
  }
  *position = valueToWrite;
  position++;
}

void writeFourCC(ByteBuffer& buffer, ByteBuffer::iterator& position, const Fourcc valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 4) {
    throw std::out_of_range("Write position out of bounds");
  }
  std::copy(valueToWrite.begin(), valueToWrite.begin() + 4, position);
  position += 4;
}

void writeIsoLang(ByteBuffer& buffer, ByteBuffer::iterator& position, const IsoLang valueToWrite) {
  if (buffer.begin() > position || buffer.end() - position < 2) {
    throw std::out_of_range("Write position out of bounds");
  }

  if (std::any_of(valueToWrite.begin(), valueToWrite.end(), [](char c) { return !isprint(c); })) {
    throw std::runtime_error("Isolang writing failed");
  }

  uint16_t tmp = 0;
  tmp = (uint16_t)(valueToWrite.at(0) - 0x60) << 10 | tmp;
  tmp = (uint16_t)(valueToWrite.at(1) - 0x60) << 5 | tmp;
  tmp = (uint16_t)(valueToWrite.at(2) - 0x60) | tmp;

  writeUint16(buffer, position, tmp);
}

void writeString(ByteBuffer& buffer, ByteBuffer::iterator& position,
                 const std::string valueToWrite) {
  if (buffer.begin() > position || buffer.end() < position ||
      static_cast<size_t>(buffer.end() - position) <= valueToWrite.size()) {
    throw std::out_of_range("Write position out of bounds");
  }
  std::copy(valueToWrite.cbegin(), valueToWrite.cend(), position);
  position += valueToWrite.size();
  *position++ = 0;
}

void writeUint32Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                      const std::vector<uint32_t> arrayToWrite) {
  if (buffer.begin() > position || buffer.end() < position ||
      static_cast<size_t>(buffer.end() - position) < 4 * arrayToWrite.size()) {
    throw std::out_of_range("Write position out of bounds");
  }
  for (auto valueToWrite : arrayToWrite) {
    writeUint32(buffer, position, valueToWrite);
  }
}

void writeInt32Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                     const std::vector<int32_t> arrayToWrite) {
  if (buffer.begin() > position || buffer.end() < position ||
      static_cast<size_t>(buffer.end() - position) < 4 * arrayToWrite.size()) {
    throw std::out_of_range("Write position out of bounds");
  }
  for (auto valueToWrite : arrayToWrite) {
    writeInt32(buffer, position, valueToWrite);
  }
}

void writeUint8Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                     const std::vector<uint8_t> arrayToWrite) {
  if (buffer.begin() > position || buffer.end() < position ||
      static_cast<size_t>(buffer.end() - position) < arrayToWrite.size()) {
    throw std::out_of_range("Write position out of bounds");
  }
  for (auto valueToWrite : arrayToWrite) {
    writeUint8(buffer, position, valueToWrite);
  }
}

void writeUint64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint64_t valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 8, std::out_of_range, "Write position out of bounds");

  uint32_t high = static_cast<uint32_t>((valueToWrite & 0xFFFFFFFF00000000ULL) >> 32U);
  uint32_t low = static_cast<uint32_t>(valueToWrite & 0xFFFFFFFF);

  writeUint32(begin, end, high);
  writeUint32(begin, end, low);
}

void writeInt64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int64_t valueToWrite) {
  writeUint64(begin, end, static_cast<uint64_t>(valueToWrite));
}

void writeUint32(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint32_t valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Write position out of bounds");

  *begin++ = static_cast<uint8_t>(valueToWrite >> 24);
  *begin++ = static_cast<uint8_t>(valueToWrite >> 16 & 0xFF);
  *begin++ = static_cast<uint8_t>(valueToWrite >> 8 & 0xFF);
  *begin++ = static_cast<uint8_t>(valueToWrite & 0xFF);
}

void writeUint32_64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                    const uint64_t valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Write position out of bounds");

  if (valueToWrite > std::numeric_limits<uint32_t>::max()) {
    throw std::out_of_range("Can't write a 32Bit value from a 64Bit value without truncating data");
  }

  uint32_t low = valueToWrite & 0xFFFFFFFFu;

  *begin++ = static_cast<uint8_t>(low >> 24);
  *begin++ = static_cast<uint8_t>(low >> 16 & 0xFFu);
  *begin++ = static_cast<uint8_t>(low >> 8 & 0xFFu);
  *begin++ = static_cast<uint8_t>(low & 0xFFu);
}

void writeInt32(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int32_t valueToWrite) {
  writeUint32(begin, end, static_cast<uint32_t>(valueToWrite));
}

void writeFloat(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end, float valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Write position out of bounds");
  uint8_t* array = reinterpret_cast<uint8_t*>(&valueToWrite);
  *begin++ = array[3];
  *begin++ = array[2];
  *begin++ = array[1];
  *begin++ = array[0];
}

void writeFloatLE(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                  float valueToWrite) {
#ifdef _MSC_VER
  errno_t err;
  ILO_ASSERT(
      (err = memcpy_s(&begin[0], end - begin, reinterpret_cast<uint8_t*>(&valueToWrite), 4)) == 0,
      "Writing float value failed: %i", err);
#else
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Write position out of bounds");
  memcpy(&begin[0], &valueToWrite, 4);
#endif
  begin += 4;
}

void writeUint24(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint32_t valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 3, std::out_of_range, "Write position out of bounds");

  *begin++ = static_cast<uint8_t>(valueToWrite >> 16);
  *begin++ = static_cast<uint8_t>(valueToWrite >> 8 & 0xFF);
  *begin++ = static_cast<uint8_t>(valueToWrite & 0xFF);
}

void writeUint16(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint16_t valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 2, std::out_of_range, "Write position out of bounds");

  *begin++ = static_cast<uint8_t>(valueToWrite >> 8);
  *begin++ = static_cast<uint8_t>(valueToWrite);
}

void writeInt16(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int16_t valueToWrite) {
  writeUint16(begin, end, static_cast<uint16_t>(valueToWrite));
}

void writeUint8(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const uint8_t valueToWrite) {
  ILO_ASSERT_WITH(end > begin, std::out_of_range, "Write position out of bounds");

  *begin = valueToWrite;
  begin++;
}

void writeFourCC(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const Fourcc valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 4, std::out_of_range, "Write position out of bounds");

  std::copy(valueToWrite.begin(), valueToWrite.begin() + 4, begin);
  begin += 4;
}

void writeIsoLang(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                  const IsoLang valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= 2, std::out_of_range, "Write position out of bounds");

  if (std::any_of(valueToWrite.begin(), valueToWrite.end(), [](char c) { return !isprint(c); })) {
    throw std::runtime_error("Isolang writing failed");
  }

  uint16_t tmp = 0;
  tmp = (uint16_t)(valueToWrite.at(0) - 0x60) << 10 | tmp;
  tmp = (uint16_t)(valueToWrite.at(1) - 0x60) << 5 | tmp;
  tmp = (uint16_t)(valueToWrite.at(2) - 0x60) | tmp;

  writeUint16(begin, end, tmp);
}

void writeString(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const std::string valueToWrite) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(valueToWrite.size() + 1), std::out_of_range,
                  "Write position out of bounds");

  std::copy(valueToWrite.cbegin(), valueToWrite.cend(), begin);
  begin += valueToWrite.size();
  *begin++ = 0;
}

void writeUint32Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                      const std::vector<uint32_t> arrayToWrite) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(4 * arrayToWrite.size()), std::out_of_range,
                  "Write position out of bounds");

  for (auto valueToWrite : arrayToWrite) {
    writeUint32(begin, end, valueToWrite);
  }
}

void writeInt32Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<int32_t> arrayToWrite) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(4 * arrayToWrite.size()), std::out_of_range,
                  "Write position out of bounds");

  for (auto valueToWrite : arrayToWrite) {
    writeInt32(begin, end, valueToWrite);
  }
}

void writeFloatArray(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<float> arrayToWrite) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(4 * arrayToWrite.size()), std::out_of_range,
                  "Write position out of bounds");

  for (auto valueToWrite : arrayToWrite) {
    writeFloat(begin, end, valueToWrite);
  }
}

void writeUint8Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<uint8_t> arrayToWrite) {
  ILO_ASSERT_WITH(end - begin >= static_cast<int32_t>(arrayToWrite.size()), std::out_of_range,
                  "Write position out of bounds");

  for (auto valueToWrite : arrayToWrite) {
    writeUint8(begin, end, valueToWrite);
  }
}
}  // namespace ilo
