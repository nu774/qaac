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
 * Content: data entry url box class
 */

// System headers
#include <stdexcept>
#include <limits>
#include <algorithm>

// External headers
#include "ilo/bytebuffertools.h"

// Internal headers
#include "urlbox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

CDataEntryUrlBox::CDataEntryUrlBox(ilo::ByteBuffer::const_iterator& begin,
                                   const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end), m_location() {
  parseBox(begin, end);
}

CDataEntryUrlBox::CDataEntryUrlBox(const SUrlBoxWriteConfig& config)
    : CFullBox(config), m_location(config.location) {
  if (config.location == "") {
    CFullBox::updateFlags(0x0000001);
  }

  flagSanityCheck();
  updateSize(0);
}

void CDataEntryUrlBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CBox::type() == ilo::toFcc("url "), std::invalid_argument,
                  "Expected box type url , but found: %s", ilo::toString(CBox::type()).c_str());

  ILO_ASSERT_WITH(begin <= end, std::out_of_range, "Iterator out of range while parsing url box");

  ILO_ASSERT_WITH(CFullBox::version() == 0, std::invalid_argument,
                  "Version %s of url box is not defined/implemented",
                  std::to_string(CFullBox::version()).c_str());

  flagSanityCheck();

  if (flags() == 0x000001) {
    m_location = "";
  } else {
    m_location = ilo::readString(begin, end, end - begin);
  }
}

void CDataEntryUrlBox::updateSize(uint64_t sizeValue) {
  uint64_t locationSize = 0;

  if (flags() == 0x000000) {
    locationSize = m_location.size() + 1;
  }

  CFullBox::updateSize(sizeValue + locationSize);
}

SAttributeList CDataEntryUrlBox::getAttributeList() const {
  SAttributeList attributesList;

  if (flags() == 0x000000) {
    SAttribute attribute;
    attribute.key = "Location";
    attribute.value = m_location;
    attributesList.push_back(attribute);
  }

  return attributesList;
}

void CDataEntryUrlBox::writeBox(ilo::ByteBuffer& buffer,
                                ilo::ByteBuffer::iterator& position) const {
  if (flags() == 0x000000) {
    ilo::writeString(buffer, position, m_location);
  }
}

void CDataEntryUrlBox::flagSanityCheck() {
  ILO_ASSERT(flags() == 0x000000 || flags() == 0x000001, "Flags %s in url box are not valid",
             std::to_string(CFullBox::flags()).c_str());
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE_FCC(url, "url ", CDataEntryUrlBox, CDataEntryUrlBox::SUrlBoxWriteConfig,
                        CContainerType::noContainer);
