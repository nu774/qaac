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
 * Content: extension for the byte buffer tools needed for mmtisobmff
 */

#include "ilo/bytebuffertools.h"

// Internal includes
#include "bytebuffertools_extension.h"
#include "common/logging.h"

using namespace ilo;
using namespace mmt::isobmff;
using namespace mmt::isobmff::tools;

namespace mmt {
namespace isobmff {
namespace tools {
BoxSizeType getBoxSizeAndType(const ByteBuffer& buffer,
                              const ByteBuffer::const_iterator& boxBegin) {
  if (buffer.begin() > boxBegin || buffer.end() - boxBegin < 8) {
    throw std::out_of_range("Read position out of bounds");
  }

  uint64_t boxSize = 0;
  Fourcc boxType;
  uint32_t headerLengthInBytes = 8;

  ByteBuffer::const_iterator temp = boxBegin;

  boxSize = readUint32(buffer, temp);
  boxType = readFourCC(buffer, temp);

  if (boxSize == 1) {
    ILO_ASSERT(buffer.end() - temp >= 8, "64-bit size box in too small buffer");
    boxSize = readUint64(buffer, temp);
    headerLengthInBytes += 8;
  }

  return BoxSizeType(boxSize, boxType, headerLengthInBytes);
}

Fourcc getBoxType(const ByteBuffer& buffer, const ByteBuffer::const_iterator& boxBegin) {
  if (buffer.begin() > boxBegin || buffer.end() - boxBegin < 8) {
    throw std::out_of_range("Read position out of bounds");
  }

  ByteBuffer::const_iterator temp = boxBegin + 4;
  return readFourCC(buffer, temp);
}

BoxSizeType getBoxSizeAndType(const ilo::ByteBuffer::const_iterator& begin,
                              const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 8, std::out_of_range, "Read position out of bounds");

  uint64_t boxSize = 0;
  Fourcc boxType;
  uint32_t headerLengthInBytes = 8;

  ByteBuffer::const_iterator temp = begin;

  boxSize = readUint32(temp, end);
  boxType = readFourCC(temp, end);

  if (boxSize == 1) {
    ILO_ASSERT(end - temp >= 8, "64-bit size box in too small buffer");
    boxSize = readUint64(temp, end);
    headerLengthInBytes += 8;
  }

  return BoxSizeType(boxSize, boxType, headerLengthInBytes);
}

Fourcc getBoxType(const ilo::ByteBuffer::const_iterator& begin,
                  const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(end - begin >= 8, std::out_of_range, "Read position out of bounds");

  ilo::ByteBuffer::const_iterator temp = begin + 4;
  return readFourCC(temp, end);
}
}  // namespace tools
}  // namespace isobmff
}  // namespace mmt
