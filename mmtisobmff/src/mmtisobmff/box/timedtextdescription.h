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
 * Content: abstract sample entry, audio sample entry and visual sample entry classes
 */

#pragma once

// System headers
#include <string>
#include <memory>
#include <vector>

// External headers
#include "ilo/common_types.h"

// Internal headers
#include "mmtisobmff/types.h"
#include "box.h"
#include "sampleentry.h"

namespace mmt {
namespace isobmff {
namespace box {

//! class implementing a text sample description
class CTimedTextDescription : public CSampleEntry {
 public:
  struct STextWriteConfig : SSampleEntryWriteConfig {
    uint32_t displayFlags = 0;
    int8_t horizontalJustification = 0;
    int8_t verticalJustification = 0;
    std::array<uint8_t, 4> backgroundColor;
    std::array<uint16_t, 4> defaultTextBox;
    uint16_t startChar;
    uint16_t endChar;
    uint16_t fontId;
    uint8_t faceStyleFlags;
    uint8_t fontSize;
    std::array<uint8_t, 4> textColor;

    STextWriteConfig(ilo::Fourcc format)
        : SSampleEntryWriteConfig(format),
          backgroundColor({0, 0, 0, 0}),
          defaultTextBox({0, 0, 0, 0}),
          textColor({0, 0, 0}) {}
  };

  //! constructor to init member variables through parsing
  CTimedTextDescription(ilo::ByteBuffer::const_iterator& begin,
                     const ilo::ByteBuffer::const_iterator& end);

  //! constructor to init member variables by setting
  explicit CTimedTextDescription(const STextWriteConfig& textWriteConfig);

  virtual uint32_t displayFlags() const { return m_displayFlags; }

  virtual int8_t horizontalJustification() const { return m_horizontalJustification; }

  virtual int8_t verticalJustification() const { return m_horizontalJustification; }

  virtual std::array<uint8_t, 4> backgroundColor() const { return m_backgroundColor; }

  virtual std::array<uint16_t, 4> defaultTextBox() const { return m_defaultTextBox; }

  virtual uint16_t startChar() const { return m_startChar; }

  virtual uint16_t endChar() const { return m_endChar; }

  virtual uint16_t fontId() const { return m_fontId; }

  virtual uint8_t faceStyleFlags() const { return m_faceStyleFlags; }

  virtual uint8_t fontStyleSize() const { return m_fontSize; }

  virtual std::array<uint8_t, 4> textColor() const { return m_textColor; }

  SAttributeList getAttributeList() const override;

 protected:
  void updateSize(uint64_t size) override;

  void writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const override;

  void writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const override;

 private:
  void parseBox(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

 private:
  uint32_t m_displayFlags;
  int8_t m_horizontalJustification;
  int8_t m_verticalJustification;
  std::array<uint8_t, 4> m_backgroundColor;
  std::array<uint16_t, 4> m_defaultTextBox;
  uint16_t m_startChar;
  uint16_t m_endChar;
  uint16_t m_fontId;
  uint8_t m_faceStyleFlags;
  uint8_t m_fontSize;
  std::array<uint8_t, 4> m_textColor;
};

}  // namespace box
}  // namespace isobmff
}  // namespace mmt
