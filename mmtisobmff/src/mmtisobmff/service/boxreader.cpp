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
 * Content: box reading class(es)
 */

// System includes
#include <ios>
#include <string>
#include <cmath>

// External includes
#include "ilo/string_utils.h"
#include "ilo/bytebuffertools.h"

// Internal includes
#include "boxreader.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {

pos_type inputBytesReadable(std::unique_ptr<IIsobmffInput>& input) {
  auto oldPosition = input->tell();
  input->seek(0, SeekingOrigin::end);
  auto stillAvailable = input->tell() - oldPosition;
  input->seek(oldPosition);
  return stillAvailable;
}

BoxSizeType CBoxReader::readBoxInto(ilo::ByteBuffer& buffer) {
  BoxSizeType boxSizeType = readBoxHeaderFields(buffer);
  return readBoxRemainder(buffer, boxSizeType);
}

bool CBoxReader::isEos() {
  return input->isEOI();
}

BoxSizeType CBoxReader::readBoxHeaderFields(ilo::ByteBuffer& buffer) const {
  BoxSizeType boxSizeType;
  buffer.resize(BASIC_HEADER_SIZER);

  ILO_ASSERT(
      input->read(buffer.begin(), buffer.end()) == BASIC_HEADER_SIZER,
      "Failed to obtain box size and type. Buffer is too small to contain basic box header.");

  ilo::ByteBuffer::const_iterator iter = buffer.begin();
  boxSizeType.size = ilo::readUint32(buffer, iter);
  boxSizeType.type = ilo::readFourCC(buffer, iter);

  if (boxSizeType.size == 0) {
    auto currentPos = input->tell();
    input->seek(0, SeekingOrigin::end);
    auto endPos = input->tell();
    input->seek(currentPos);
    boxSizeType.size = (size_t)(endPos - currentPos + buffer.size());
  } else if (boxSizeType.size == 1) {
    buffer.resize(buffer.size() + EXTRA_EXTENSION_HEADER);
    iter = buffer.begin() + BASIC_HEADER_SIZER;
    ILO_ASSERT(
        input->read(buffer.begin() + BASIC_HEADER_SIZER, buffer.end()) == EXTRA_EXTENSION_HEADER,
        "Header signals 64bit box extension, but buffer is too small to contain extension size "
        "field");
    boxSizeType.size = ilo::readUint64(buffer, iter);
  }
  boxSizeType.headerLengthInBytes = static_cast<uint32_t>(buffer.size());

  ILO_LOG_SCOPE("type: %s, size: % " PRIu64 "bytes", ilo::toString(boxSizeType.type).c_str(),
                boxSizeType.size);

  return boxSizeType;
}

BoxSizeType CBoxReader::readBoxRemainder(ilo::ByteBuffer& buffer,
                                         const BoxSizeType& boxSizeType) const {
  ILO_ASSERT(boxSizeType.size >= boxSizeType.headerLengthInBytes,
             "Invalid stream. Reported box header is bigger than total box size");

  uint64_t toRead = boxSizeType.size - boxSizeType.headerLengthInBytes;

  if (toRead == 0) {
    return boxSizeType;
  }

  auto availableByteCount = inputBytesReadable(input);
  if (toRead > availableByteCount) {
    ILO_LOG_WARNING("box truncated in input: type %s, size %udd (available: %udd)",
                    ilo::toString(boxSizeType.type).c_str(), boxSizeType.size, availableByteCount);
    toRead = availableByteCount;
  }

  if (boxSizeType.type == ilo::toFcc("mdat") && m_skipMdatPayload) {
    input->seek(toRead, SeekingOrigin::cur);
    return boxSizeType;
  }

  buffer.resize(buffer.size() + static_cast<size_t>(toRead));

  size_t remainingRead =
      input->read(buffer.begin() + boxSizeType.headerLengthInBytes, buffer.end());
  ILO_ASSERT(static_cast<uint64_t>(remainingRead) == toRead,
             "Failed to read box payload. Not enough data to read.");

  return boxSizeType;
}
}  // namespace isobmff
}  // namespace mmt
