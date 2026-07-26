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
 * @file mp4a_decoderconfigrecord.h
 * @brief Definition of an MP4A config record
 *
 * Config record class holding AAC specific data required to
 * initialize a decoder
 */

#pragma once

// System includes
#include <vector>
#include <limits>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
namespace config {
/*!
 * @brief The MP4A decoder config record holding data needed to initialize an AAC decoder
 * out-of-band
 *
 * This class implements the a subset of the 'ES_Descriptor'  (ISO/IEC 14496-1)
 */
class CMp4aDecoderConfigRecord {
 public:
  /*!
   * @brief Config needed to create a new AAC config record class instance (for writing)
   *
   */
  struct SConfig {
    /*!
     * @brief Maximum audio bitrate value in bits per second of the encoded data (required)
     *
     * @note If available, use the value provided by an AAC encoder API
     */
    uint32_t maxBitrate = 0;
    /*!
     * @brief Average audio bitrate value in bits per second of the encoded data (required)
     *
     * @note If available, use the value provided by an AAC encoder API
     */
    uint32_t avgBitrate = 0;
    /*!
     * @brief The size of the decoding buffer for the elementary stream in bytes (ISO/IEC 14496-1)
     * (required)
     *
     * @note If available, use the value provided by an AAC encoder API
     */
    uint32_t bufferSizeDB = 0;
    /*!
     * @brief Audio Specific Config data block (required)
     *
     * Depends on the encoder configuration and must match the
     * config being used to encode the audio access units (AUs) stored in the track.
     *
     * @warning Should not be handcrafted, but taken from an encoder instance
     */
    ilo::ByteBuffer asc;
  };

  //! Constructor for creating the config record by parsing a buffer
  CMp4aDecoderConfigRecord(ilo::ByteBuffer::const_iterator& begin,
                           const ilo::ByteBuffer::const_iterator& end);
  //! Constructor for creating the config record by filling in a user config
  CMp4aDecoderConfigRecord(const SConfig& config);

  /*!
   * @brief Optional function to retrieve complete 'ES_Descriptor' byte buffer
   *
   * @ref CMp4aDecoderConfigRecord class only implements a specific subset of the 'ES_Descriptor'
   * structure to allow access to AAC related data required for decoding or writing a valid
   * MP4 file containing AAC.
   *
   * This function gives access to the complete 'ES_Descriptor' byte structure for further external
   * parsing or copying.
   *
   * Can be used to get access to unknown descriptors that are not handled by this class or if a
   * specific decoder instance requires the complete 'ES_Descriptor' buffer for initialization (for
   * example the Apple AAC decoder).
   */
  const ilo::ByteBuffer& esdsByteBlob() const { return m_esdsByteBlob; }

  /*!
   * @brief Get object/scene description
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint8_t objectTypeIndication() { return m_objectTypeIndication; }
  /*!
   * @brief Get type of elementary stream
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint8_t streamType() { return m_streamType; }
  /*!
   * @brief Check if the stream is used for upstream information (boolean value)
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint8_t upStream() { return m_upStream; }
  /*!
   * @brief Get the size of the decoding buffer for the elementary stream in bytes
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint32_t bufferSizeDB() { return m_config.bufferSizeDB; }
  /*!
   * @brief Get the maximum bitrate in bits per second of the elementary stream
   * in any time window of one second duration
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint32_t maxBitrate() { return m_config.maxBitrate; }
  /*!
   * @brief Get the average bitrate in bits per second of the elementary stream
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  uint32_t avgBitrate() { return m_config.avgBitrate; }
  /*!
   * @brief Get the audio specific config required to initialize a typical AAC decoder
   *
   * @see ISO/IEC 14496-1 section 7.2.6.6
   */
  const ilo::ByteBuffer& asc() const { return m_config.asc; }

  /*!
   * @brief Serialize the class into a byte buffer according to ISO/IEC 14496-1 (ES_Descriptor)
   *
   * @note This is only required for the @ref tools::SEasyTrackConfig feature from
   * 'commonhelpertools.h'. The standard track writer will do this itself.
   */
  uint64_t write(ilo::ByteBuffer& buffer, ilo::ByteBuffer::iterator& position);
  /*!
   * @brief Query the size of this class structure
   *
   * Needed in combination with @ref CMp4aDecoderConfigRecord::write to create a buffer big enough
   * to serialize into.
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

  uint8_t m_objectTypeIndication;
  uint8_t m_streamType;
  uint8_t m_upStream;
  SConfig m_config;
  ilo::ByteBuffer m_esdsByteBlob;
};
}  // namespace config
}  // namespace isobmff
}  // namespace mmt
