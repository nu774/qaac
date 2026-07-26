/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2018 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * Content: Initial Object descriptor class
 */

#pragma once

// external include
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"

#include "basedescriptor.h"
#include "esidincdescriptor.h"

namespace mmt {
namespace isobmff {
namespace descriptor {
// Initial Object Descriptor after ISO 14496-1
class CIODescriptor : public CBaseDescriptor {
 public:
  struct SIODescriptorWriteConfig : CBaseDescriptor::SBaseDescriptorWriteConfig {
    uint16_t objectDescriptorId = 1;
    uint8_t URLflag = 0;
    uint8_t includeInlineProfileLevelFlag = 0;
    uint8_t URLlength = 0;
    std::vector<uint8_t> URLstring;
    uint8_t ODProfileLevelIndication =
        0xFF;  // 0xFF means "No OD capability required" as described in ISO/IEC 14496-1
    uint8_t sceneProfileLevelIndication =
        0xFF;  // 0xFF means "no scene graph capability required" as described in ISO/IEC 14496-11
    uint8_t audioProfileLevelIndication =
        0xFF;  // 0xFF means "no audio capability required" as described in ISO/IEC 14496-3
    uint8_t visualProfileLevelIndication =
        0xFF;  // 0xFF means "no visual capability required" as described in ISO/IEC 14496-2
    uint8_t graphicsProfileLevelIndication =
        0xFF;  // 0xFF means "no graphics capability required" as described in ISO/IEC 14496-11
    std::vector<CESIdIncDescriptor> esIdIncDescriptors;

    SIODescriptorWriteConfig()
        : SBaseDescriptorWriteConfig(EDescriptorTag::MP4InitialObjectDescriptor) {}
  };

  // default constructor
  CIODescriptor(){};
  // constructor to init member variables through parsing
  CIODescriptor(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);
  // constructor to init member variables by setting
  explicit CIODescriptor(const SIODescriptorWriteConfig& config);

  SAttributeList getAttributeList() const override;

 public:
  uint16_t objectDescriptorId() { return m_objectDescriptorId; }
  uint8_t URLflag() { return m_URLflag; }
  uint8_t includeInlineProfileLevelFlag() { return m_includeInlineProfileLevelFlag; }
  uint8_t URLlength() { return m_URLlength; }
  std::vector<uint8_t> URLstring() { return m_URLstring; }
  uint8_t ODProfileLevelIndication() { return m_ODProfileLevelIndication; }
  uint8_t sceneProfileLevelIndication() { return m_sceneProfileLevelIndication; }
  uint8_t audioProfileLevelIndication() { return m_audioProfileLevelIndication; }
  uint8_t visualProfileLevelIndication() { return m_visualProfileLevelIndication; }
  uint8_t graphicsProfileLevelIndication() { return m_graphicsProfileLevelIndication; }
  std::vector<CESIdIncDescriptor> esIdIncDescriptors() { return m_esIdIncDescriptors; }
  ilo::ByteBuffer remainingPayload() { return m_remainingPayload; }

 protected:
  void updateSize(uint32_t size) final;

  // function to write the descriptor-content
  void writeDescriptor(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const override;

 private:
  // function to parse the descriptor
  void parse(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  uint16_t m_objectDescriptorId = 0;
  uint8_t m_URLflag = 0;
  uint8_t m_includeInlineProfileLevelFlag = 0;
  uint8_t m_URLlength = 0;
  std::vector<uint8_t> m_URLstring;
  uint8_t m_ODProfileLevelIndication = 0;
  uint8_t m_sceneProfileLevelIndication = 0;
  uint8_t m_audioProfileLevelIndication = 0;
  uint8_t m_visualProfileLevelIndication = 0;
  uint8_t m_graphicsProfileLevelIndication = 0;
  std::vector<CESIdIncDescriptor> m_esIdIncDescriptors;
  ilo::ByteBuffer m_remainingPayload;
};
}  // namespace descriptor
}  // namespace isobmff
}  // namespace mmt
