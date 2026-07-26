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
 * Content: mp4a decoder config record
 */

// System includes
#include <algorithm>
#include <memory>

// External includes
#include "ilo/bytebuffertools.h"

// Internal includes
#include "mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h"
#include "descriptor/esdescriptor.h"
#include "common/logging.h"

using namespace ilo;

namespace mmt {
namespace isobmff {
namespace config {
CMp4aDecoderConfigRecord::CMp4aDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                                                   const ilo::ByteBuffer::const_iterator& end)
    : m_objectTypeIndication(0), m_streamType(0), m_upStream(0) {
  parse(begin, end);
}

CMp4aDecoderConfigRecord::CMp4aDecoderConfigRecord(const CMp4aDecoderConfigRecord::SConfig& config)
    : m_objectTypeIndication(0x40), m_streamType(0x05), m_upStream(0), m_config(config) {
  using namespace descriptor;

  CGenericDecoderSpecificInfo::SGenericDecoderSpecificInfoWriteConfig gdsiConfig;
  gdsiConfig.decoderSpecificInfo = m_config.asc;

  auto gdsi = std::make_shared<CGenericDecoderSpecificInfo>(gdsiConfig);

  CDecoderConfigDescriptor::SDecoderConfigDescriptorWriteConfig dcdConfig;
  dcdConfig.avgBitrate = m_config.avgBitrate;
  dcdConfig.bufferSizeDB = m_config.bufferSizeDB;
  dcdConfig.maxBitrate = m_config.maxBitrate;
  dcdConfig.objectTypeIndication = m_objectTypeIndication;
  dcdConfig.streamType = m_streamType;
  dcdConfig.reserved = 1;
  dcdConfig.upStream = m_upStream;
  dcdConfig.decoderSpecificInfo = gdsi;

  CDecoderConfigDescriptor dcd(dcdConfig);

  CSLConfigDescriptor slConfigDescriptor;

  CESDescriptor::SESDescriptorWriteConfig esdConfig;
  esdConfig.dcd = dcd;
  esdConfig.slConfigDescriptor = slConfigDescriptor;

  CESDescriptor esd(esdConfig);

  // The esd size is only the esd payload size. Calculate the tag/size
  uint8_t numBytes = static_cast<uint8_t>(esd.size() / MAX_SIZE_IN_ONE_BYTE);
  numBytes += (esd.size() % MAX_SIZE_IN_ONE_BYTE) ? (1) : (0);
  numBytes += 1;  // m_tag

  m_esdsByteBlob.resize(esd.size() + numBytes);
  ilo::ByteBuffer::iterator iter = m_esdsByteBlob.begin();

  esd.write(m_esdsByteBlob, iter);
}

void CMp4aDecoderConfigRecord::parse(ilo::ByteBuffer::const_iterator& begin,
                                     const ilo::ByteBuffer::const_iterator& end) {
  m_esdsByteBlob = ilo::ByteBuffer(begin, end);

  auto esDescriptor = descriptor::CESDescriptor(begin, end);
  auto decConfDesc = esDescriptor.decoderConfigDescriptor();
  auto decoderSpecificConfig = decConfDesc.decoderSpecificConfig();

  m_config.asc = ilo::ByteBuffer(decoderSpecificConfig.begin(), decoderSpecificConfig.end());
  m_objectTypeIndication = decConfDesc.objectTypeIndication();
  m_streamType = decConfDesc.streamType();
  m_upStream = decConfDesc.upStream();
  m_config.bufferSizeDB = decConfDesc.bufferSizeDB();
  m_config.maxBitrate = decConfDesc.maxBitRate();
  m_config.avgBitrate = decConfDesc.avgBitRate();
}

uint64_t CMp4aDecoderConfigRecord::size() const {
  return m_esdsByteBlob.size();
}

SAttributeList CMp4aDecoderConfigRecord::getAttributeList() const {
  SAttributeList attributesList;

  SAttribute attribute;

  attribute.key = "Object Type Indication";
  attribute.value = std::to_string(m_objectTypeIndication);
  attributesList.push_back(attribute);

  attribute.key = "Stream Type";
  attribute.value = std::to_string(m_streamType);
  attributesList.push_back(attribute);

  attribute.key = "Up Stream";
  attribute.value = std::to_string(m_upStream);
  attributesList.push_back(attribute);

  attribute.key = "Config";
  std::stringstream co;

  co << "Max Bitrate: " << std::to_string(m_config.maxBitrate)
     << ", Avg Bitrate: " << std::to_string(m_config.avgBitrate)
     << ", Buffer Size DB: " << std::to_string(m_config.bufferSizeDB)
     << ", Audio Specific Config: ";

  for (auto& byte : m_config.asc) {
    co << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase
       << static_cast<int>(byte);
    co << " ";
  }
  attribute.value = co.str();
  attribute.value = attribute.value.substr(0, attribute.value.size() - 1);
  attributesList.push_back(attribute);

  return attributesList;
}

uint64_t CMp4aDecoderConfigRecord::write(ilo::ByteBuffer& buffer,
                                         ilo::ByteBuffer::iterator& position) {
  ILO_ASSERT(position + m_esdsByteBlob.size() <= buffer.end(),
             "Buffer is too small to hold esds data");

  std::copy(m_esdsByteBlob.begin(), m_esdsByteBlob.end(), position);
  position += m_esdsByteBlob.size();

  return m_esdsByteBlob.size();
}
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
