/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2019 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file jxs_decoderconfigrecord.h
 * @brief Definition of a JXS config record
 *
 * Config record class holding JPEG XS specific data required to
 * initialize a decoder
 */

#pragma once

// System includes
#include <vector>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
namespace config {
/*!
 * @brief The JXS decoder config record holding data needed to initialize a JPEG XS decoder out of
 * band
 *
 * Details on the fields contained here can be taken from Annex B of ISO/IEC 21122-3.
 *
 * This class contains the "Codestream_Header" as defined in Annex A.5.5 as a byte buffer. This is
 * used in Annex B.3.3 to form the "JPEGXSCodestreamHeaderBox" of type 'jxsH'
 */
class CJxsDecoderConfigRecord {
 public:
  //! Constructor for creating the config record by parsing a buffer
  CJxsDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                          const ilo::ByteBuffer::const_iterator& end);
  //! Constructor for creating an empty config record for manual filling
  CJxsDecoderConfigRecord();

  /*!
   * @brief Returns a copy of the data structure defined as "Codestream_Header" in the JXS
   * specification
   *
   * Can be used to initialize a JPEG XS decoder out-of-band.
   *
   * @see ISO/IEC 21122-3 Annex B.3.3 and Annex A.5.5
   */
  const ilo::ByteBuffer& codestreamHeader() const;

  /*!
   * @brief Set the data structure defined as "Codestream_Header" in the JXS specification
   * (required)
   *
   * @see ISO/IEC 21122-3 Annex B.3.3 and Annex A.5.5
   */
  void setCodestreamHeader(const ilo::ByteBuffer& codestreamHeader);

  /*!
   * @brief Serialize the class into a byte buffer according to ISO/IEC 21122-3 Annex B
   *
   * @note This is only required for the @ref tools::SEasyTrackConfig feature from
   * 'commonhelpertools.h'. The standard track writer will do this itself.
   */
  void write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position);

  /*!
   * @brief Query the size of this class structure
   *
   * Needed in combination with @ref CJxsDecoderConfigRecord::write to create a buffer big enough to
   * serialize into.
   */
  uint64_t size() const;

  /*!
   * @brief A key/value attribute list containing name and value as strings.
   *
   * Can be used for generic printing
   */
  SAttributeList getAttributeList() const;

 private:
  void parse(ilo::ByteBuffer::const_iterator& begin, const ilo::ByteBuffer::const_iterator& end);

  ilo::ByteBuffer m_codestreamHeader;
};
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
