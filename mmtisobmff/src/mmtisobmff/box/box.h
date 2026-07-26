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
 * Content: abstract box class
 */

#pragma once

// System headers
#include "ilo/common_types.h"

// Internal headers
#include "mmtisobmff/types.h"
#include "ibox.h"

namespace mmt {
namespace isobmff {
namespace box {

//! Box after ISO 14496-12
class CBox : public IBox {
 public:
  //! constructor to init member variables through parsing
  CBox(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  //! constructor to init member variables by setting
  explicit CBox(const SBoxWriteConfig& boxData);

  uint64_t size() const override { return m_size; }

  ilo::Fourcc type() const override { return m_type; }

  bool had64BitSizeInInput() const override { return m_had64BitSizeInInput; }

  void write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const override;

  SAttributeList getAttributeList() const override;

 protected:
  void updateSize(uint64_t size) override;

  //! check if the box has 64 bits size
  virtual bool hasLargeSize() const;

  //! function to write the header (size and type)
  virtual void writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const;

  //! function to write the box-content
  //! This method must be overridden in box implementations
  virtual void writeBox(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const = 0;

 private:
  //! function to parse the header (size and type)
  void parse(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  uint64_t m_size;
  ilo::Fourcc m_type;
  bool m_had64BitSizeInInput = false;
};

//! FullBox after ISO 14496-12
class CFullBox : public CBox, public IFullBox {
 public:
  //! constructor to init member variables through parsing
  CFullBox(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  //! constructor to init member variables by setting
  explicit CFullBox(const SFullBoxWriteConfig& fullBoxData);

 public:
  //! function to get the flags
  virtual uint32_t flags() const override {
    return m_flags & 0x00FFFFFFU;  // Just to be safe
  }

  //! function to get the version
  virtual uint8_t version() const override { return m_version; }

  //! function to write the header including version and flags
  void writeHeader(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position) const override;

 protected:
  void updateSize(uint64_t size) override;

  void updateVersion(uint8_t value) { m_version = value; }

  void updateFlags(uint32_t value) { m_flags = value; }

 private:
  //! function to parse the header including version and flags
  void parse(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  uint8_t m_version;
  uint32_t m_flags;
};

}  // namespace box
}  // namespace isobmff
}  // namespace mmt
