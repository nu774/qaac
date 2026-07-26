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
 * Content: media processing unit box class
 */

// System headers
#include <stdexcept>
#include <algorithm>
#include <iterator>

// External headers
#include "ilo/string_utils.h"
#include "ilo/bytebuffertools.h"

// Internal headers
#include "mmpubox.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
namespace box {

static void assertAssetIdScheme(ilo::Fourcc assetIdScheme) {
  ILO_LOG_SCOPE("%s", ilo::toString(assetIdScheme).c_str());

  // To check: this might have to be updated because last version of ISO/IEC 23008-1 has different
  // asset_id_scheme 0x00000000 UUID (universally unique identifier) 0x00000001 URI (uniform
  // resource identifier)

  std::array<char, 3> uri{'U', 'R', 'I'};

  ILO_ASSERT_WITH(assetIdScheme == ilo::toFcc("UUID") ||
                      std::equal(uri.begin(), uri.end(), assetIdScheme.begin()) ||
                      std::equal(uri.begin(), uri.end(), assetIdScheme.begin() + 1),
                  std::invalid_argument, "MPU box invalid asset_id_scheme %s",
                  ilo::toString(assetIdScheme).c_str());
}

CMediaProcessingUnitBox::CMediaProcessingUnitBox(ilo::ByteBuffer::const_iterator& begin,
                                                 const ilo::ByteBuffer::const_iterator& end)
    : CFullBox(begin, end),
      m_isComplete(0),
      m_isAdcPresent(0),
      m_reserved(0),
      m_mpuSequenceNumber(0),
      m_assetIdentifier() {
  parseBox(begin, end);
}

CMediaProcessingUnitBox::CMediaProcessingUnitBox(const SMmpuBoxWriteConfig& config)
    : CFullBox(config),
      m_isComplete(false),
      m_isAdcPresent(false),
      m_reserved(0),
      m_mpuSequenceNumber(0),
      m_assetIdentifier() {
  assertAssetIdScheme(config.assetIdentifier.assetIdScheme);

  ILO_ASSERT_WITH(
      config.assetIdentifier.assetIdLength == config.assetIdentifier.assetIdValue.size(),
      std::invalid_argument, "AssetIdLength and number of AssetIdValues don't match");

  m_isComplete = config.isComplete;
  m_isAdcPresent = config.isAdcPresent;
  m_reserved = 0;
  m_mpuSequenceNumber = config.mpuSequenceNumber;
  m_assetIdentifier = config.assetIdentifier;

  updateSize(0);
}

void CMediaProcessingUnitBox::parseBox(ilo::ByteBuffer::const_iterator& begin,
                                       const ilo::ByteBuffer::const_iterator& end) {
  ILO_ASSERT_WITH(CFullBox::version() == 0, std::invalid_argument,
                  "Version %s of mmpu box is not defined/implemented",
                  (std::to_string(CFullBox::version()).c_str()));

  ILO_ASSERT_WITH(CFullBox::flags() == 0, std::invalid_argument,
                  "Flags must be zero for this version of the mmpu box");

  uint8_t tmp = ilo::readUint8(begin, end);

  m_isComplete = (tmp & 0x80) != 0;
  m_isAdcPresent = (tmp & 0x40) != 0;
  m_reserved = (uint8_t)(tmp & 0x3F);
  m_mpuSequenceNumber = ilo::readUint32(begin, end);
  m_assetIdentifier.assetIdScheme = ilo::readFourCCRaw(begin, end);

  assertAssetIdScheme(m_assetIdentifier.assetIdScheme);

  m_assetIdentifier.assetIdLength = ilo::readUint32(begin, end);

  for (uint32_t i = 0; i < m_assetIdentifier.assetIdLength; ++i) {
    m_assetIdentifier.assetIdValue.push_back(ilo::readUint8(begin, end));
  }
}

void CMediaProcessingUnitBox::updateSize(uint64_t sizeValue) {
  // size + complete_present_reserved + seqNumber + scheme + length + values
  CFullBox::updateSize(sizeValue + 1 + 4 + 4 + 4 + m_assetIdentifier.assetIdValue.size());
}

SAttributeList CMediaProcessingUnitBox::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;
  attribute.key = "Mpu Sequence Number";
  attribute.value = std::to_string(m_mpuSequenceNumber);
  attributesList.push_back(attribute);

  attribute.key = "Asset Identifier";
  std::stringstream ss;

  ss << "Asset Id Length: " << std::to_string(m_assetIdentifier.assetIdLength)
     << ", Asset Id Scheme: " << ilo::toString(m_assetIdentifier.assetIdScheme)
     << ", Asset Id Value{";

  uint8_t index = 0;
  for (auto asset : m_assetIdentifier.assetIdValue) {
    ss << (std::to_string(asset));
    if (index < m_assetIdentifier.assetIdValue.size() - 1) {
      ss << ", ";
    }
    index++;
  }

  ss << "}";

  attribute.value = ss.str();
  attributesList.push_back(attribute);

  return attributesList;
}

void CMediaProcessingUnitBox::writeBox(ilo::ByteBuffer& buffer,
                                       ilo::ByteBuffer::iterator& position) const {
  uint8_t tmp = 0;

  if (m_isComplete) {
    tmp += (uint8_t(1) << 7);
  }

  if (m_isAdcPresent) {
    tmp += (uint8_t(1) << 6);
  }

  tmp += uint8_t(m_reserved);

  ilo::writeUint8(buffer, position, tmp);
  ilo::writeUint32(buffer, position, m_mpuSequenceNumber);
  ilo::writeFourCC(buffer, position, m_assetIdentifier.assetIdScheme);
  ilo::writeUint32(buffer, position, m_assetIdentifier.assetIdLength);

  for (auto assetId : m_assetIdentifier.assetIdValue) {
    ilo::writeUint8(buffer, position, assetId);
  }
}

}  // namespace box
}  // namespace isobmff
}  // namespace mmt

#include "box/boxregistryentry.h"

using namespace mmt;
using namespace mmt::isobmff;
using namespace mmt::isobmff::box;

BOXREGISTRY_DECLARE(mmpu, CMediaProcessingUnitBox, CMediaProcessingUnitBox::SMmpuBoxWriteConfig,
                    CContainerType::noContainer);
