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

/*!
 * @file bytebuffertools.h
 * @brief Tools for reading/writing value types from/to a byte buffer
 */

#pragma once

// System includes
#include <vector>
#include <string>

// Internal includes
#include "ilo/version.h"
#include "common_types.h"

namespace ilo {
/*! \defgroup ByteBufferReadHelper Functions to read data from a buffer
 *  @{
 *  There are two sets of reader helper. The first one operates on the buffer and a position
 * iterator. The second set works on begin and end iterators. After reading, the position (or begin)
 * iterator is advanced. If the functions failed to read the value, an exception is thrown.
 *
 *  @note Unless explicitly stated otherwise, these functions are reading big-endian format.
 */

uint64_t readUint64(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
int64_t readInt64(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
uint32_t readUint32(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
int32_t readInt32(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
uint32_t readUint24(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
uint16_t readUint16(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
int16_t readInt16(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
uint8_t readUint8(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
Fourcc readFourCC(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
Fourcc readFourCCRaw(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
IsoLang readIsoLang(const ByteBuffer& buffer, ByteBuffer::const_iterator& position);
std::string readString(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                       uint64_t maxLength);

std::vector<uint32_t> readUint32Array(const ByteBuffer& buffer,
                                      ByteBuffer::const_iterator& position, uint32_t count);
std::vector<int32_t> readInt32Array(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                                    uint32_t count);
std::vector<uint8_t> readUint8Array(const ByteBuffer& buffer, ByteBuffer::const_iterator& position,
                                    uint32_t count);

uint64_t readUint64(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
int64_t readInt64(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
uint32_t readUint32(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
int32_t readInt32(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
uint32_t readUint24(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
uint16_t readUint16(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
int16_t readInt16(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
uint8_t readUint8(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
Fourcc readFourCC(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
Fourcc readFourCCRaw(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
IsoLang readIsoLang(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end);
std::string readString(ByteBuffer::const_iterator& begin, const ByteBuffer::const_iterator& end,
                       uint64_t maxLength);
std::string readStringNonStrict(ByteBuffer::const_iterator& begin,
                                const ByteBuffer::const_iterator& end, uint64_t maxLength);

std::vector<uint32_t> readUint32Array(ByteBuffer::const_iterator& begin,
                                      const ByteBuffer::const_iterator& end, uint32_t count);
std::vector<int32_t> readInt32Array(ByteBuffer::const_iterator& begin,
                                    const ByteBuffer::const_iterator& end, uint32_t count);
std::vector<uint8_t> readUint8Array(ByteBuffer::const_iterator& begin,
                                    const ByteBuffer::const_iterator& end, uint32_t count);

/**@}*/

/*! \defgroup ByteBufferWriteHelper Functions to write data to a buffer
 *  @{
 *  There are two sets of write helper. The first one operates on the buffer and a position
 * iterator. The second set works on begin and end iterators. After writing, the position (or begin)
 * iterator is advanced. If the functions failed to write the value, an exception is thrown.
 *
 *  @note Unless explicitly stated otherwise, these functions are writing big-endian format.
 */

void writeUint64(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint64_t valueToWrite);
void writeInt64(ByteBuffer& buffer, ByteBuffer::iterator& position, const int64_t valueToWrite);
void writeUint32(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint32_t valueToWrite);
void writeUint32_64(ByteBuffer& buffer, ByteBuffer::iterator& position,
                    const uint64_t valueToWrite);
void writeInt32(ByteBuffer& buffer, ByteBuffer::iterator& position, const int32_t valueToWrite);
void writeUint24(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint32_t valueToWrite);
void writeUint16(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint16_t valueToWrite);
void writeInt16(ByteBuffer& buffer, ByteBuffer::iterator& position, const int16_t valueToWrite);
void writeUint8(ByteBuffer& buffer, ByteBuffer::iterator& position, const uint8_t valueToWrite);
void writeFourCC(ByteBuffer& buffer, ByteBuffer::iterator& position, const Fourcc valueToWrite);
void writeIsoLang(ByteBuffer& buffer, ByteBuffer::iterator& position, const IsoLang valueToWrite);
void writeString(ByteBuffer& buffer, ByteBuffer::iterator& position,
                 const std::string valueToWrite);

void writeUint32Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                      const std::vector<uint32_t> arrayToWrite);
void writeInt32Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                     const std::vector<int32_t> arrayToWrite);
void writeUint8Array(ByteBuffer& buffer, ByteBuffer::iterator& position,
                     const std::vector<uint8_t> arrayToWrite);

void writeUint64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint64_t valueToWrite);
void writeInt64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int64_t valueToWrite);
void writeUint32(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint32_t valueToWrite);
void writeUint32_64(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                    const uint64_t valueToWrite);
void writeInt32(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int32_t valueToWrite);
void writeFloat(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const float valueToWrite);
void writeFloatLE(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end, float valueToWrite);
void writeUint24(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint32_t valueToWrite);
void writeUint16(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const uint16_t valueToWrite);
void writeInt16(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const int16_t valueToWrite);
void writeUint8(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                const uint8_t valueToWrite);
void writeFourCC(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const Fourcc valueToWrite);
void writeIsoLang(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                  const IsoLang valueToWrite);
void writeString(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                 const std::string valueToWrite);

void writeUint32Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                      const std::vector<uint32_t> arrayToWrite);
void writeInt32Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<int32_t> arrayToWrite);
void writeFloatArray(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<float> arrayToWrite);
void writeUint8Array(ByteBuffer::iterator& begin, const ByteBuffer::iterator& end,
                     const std::vector<uint8_t> arrayToWrite);

/**@}*/
}  // namespace ilo
