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

/**
 * @file specificboxinfo.h
 * @brief Definitions for specific box information aggregated by use-cases
 * \defgroup SpecificBoxInfo Querying low-level meta data for specific use-cases
 *
 * This is a collection of classes that can be used with CIsobmffReader::specificBoxInfo
 * to query information for a specific use-case that is usually part of low-level box information,
 * spread across multiple boxes and/or needs to be interpreted first.
 */

#pragma once

// System includes
#include <vector>
#include <memory>
#include <map>

// External includes
#include "ilo/common_types.h"

// Internal includes
#include "mmtisobmff/types.h"
#include "mmtisobmff/reader/reader.h"

namespace mmt {
namespace isobmff {
/*!
 * @brief Information relevant for DASH streaming
 *
 * Can be retrieved by calling CIsobmffReader::specificBoxInfo<SDashInfo> on an instantiated track
 * reader.
 * @note Depending on the MP4 type, not all or none of the specific information might be present. If
 * this is the case the corresponding member variable will be a nullptr. Otherwise the variable
 * contains valid data.
 * @note This structure is meant for non-multiplexed fragmented MP4 files containing only one media
 * track.
 *
 * \ingroup SpecificBoxInfo
 */
struct SDashInfo : public IBoxInfo {
 public:
  /*!
   * @brief Information about the top level segment index box
   *
   * If present, it contains the information from a top-level 'sidx' box in the 'moov' container.
   * 'sidx' is used as a look-up table for segments and provides compacts
   * access to their meta data without the need to download all of the segments.
   *
   * This information is already available after feeding the init segment to the library.
   *
   * For details please see ISO/IEC 14496-12 8.16.3
   */
  struct SSidxInfo {
    struct SSidxReference {
      bool referenceType = false;
      uint32_t referenceSize = 0;
      uint32_t subsegmentDuration = 0;
      bool startsWithSap = false;
      uint8_t sapType = 0;
      uint32_t sapDeltaTime = 0;
    };

    uint32_t referenceId = 0;
    uint32_t timescale = 0;
    uint64_t earliestPresentationTime = 0;
    uint64_t firstOffset = 0;
    uint16_t referenceCount = 0;
    std::vector<SSidxReference> references;
  };

  /*!
   * @brief Information about track fragment decode time
   *
   * If present, it contains the information from all 'tfdt' boxes of all segments.
   * It requires parsing of the media segments.
   * If the library is fed at least an init segment and 1..n media segments, the data in this class
   * represents the data found in the provided buffers.
   *
   * It is used (for example) during a tune-in or seeking operation of a player to determine the
   * starting time for a segment and calculate the sample timestamp.
   *
   * For details please see ISO/IEC 14496-12 8.8.12
   */
  struct STfdtInfo {
    /*!
     * @brief Base media decode time of all segments found in the buffer
     *
     * The returned vector contains 1 base media decode time per segment.
     */
    std::vector<uint64_t> m_baseMediaDecodeTimes;
  };

 public:
  /*!
   * @brief Creates SDashInfo object from an active reader instance
   *
   * @note Needs be created via CIsobmffReader::specificBoxInfo<SDashInfo> function call
   */
  SDashInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl);

  //! Sidx info - Can be nullptr if no such information exists
  std::unique_ptr<SSidxInfo> m_sidxInfo;
  //! Tfdt info - Can be nullptr if no such information exists
  std::unique_ptr<STfdtInfo> m_tfdtInfo;
};

//-----------MMTP Info-----------------

/*!
 * @brief Information relevant for MMTP transport format
 *
 * Can be retrieved by calling CIsobmffReader::specificBoxInfo<SMmtpInfo> on an instantiated reader.
 * @note If there is no MMTP relevant data in the MP4 file, m_truns will be empty and
 * m_mdatPayloadSize will be zero.
 *
 * \ingroup SpecificBoxInfo
 */
struct SMmtpInfo : public IBoxInfo {
 public:
  //! Containing information from 'trun' entries relevant for MMTP
  struct STrunInfo {
    //! Sample sizes contained in this 'trun' box
    std::vector<uint32_t> m_sampleSizes;
  };

 public:
  /*!
   * @brief Creates SMmtpInfo object from an active reader instance
   *
   * @note Needs be created via CIsobmffReader::specificBoxInfo<SMmtpInfo> function call
   */
  SMmtpInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl);

  uint32_t m_moofSequenceNumber = 0;
  uint64_t m_mdatPayloadSize = 0;
  std::vector<STrunInfo> m_truns;
};

//-----------MPEG-D DRC Info-----------------

/*!
 * @brief MPEG-D DRC information on file format level
 *
 * This class does not return parsed data. Instead, it provides concatenated buffers of MPEG-D DRC
 * related structures (e.g. 'tlou' and 'alou') that need to interpreted first.
 * The general byte structure follows the isobmff standard for a FullBox and is in big endian
 * format.
 *
 * @code
 * unsigned int(32) size (in bytes)
 * unsigned int(32) fourCC
 * unsigned int(8)  version
 * unsigned int(24) flags
 * unsigned int(8*(size - 12 bytes)) payload
 * .... complete pattern continues until end ....
 * @endcode
 *
 * The format of the payload is defined in ISO/IEC 14496-12 12.2.7 and mirrors the bitstream syntax
 * version. This can be handy if a decoder is to be fed with this data. In this case it is necessary
 * to just extract the payload sections from above mentioned format and feed them one-by-one to the
 * decoder.
 *
 * Can be retrieved by calling CIsobmffReader::specificBoxInfo<SDrcInfo> on an instantiated reader.
 * @note If there is no DRC relevant data in the mp4 file, any returned buffer is an empty buffer.
 *
 * \ingroup SpecificBoxInfo
 */
struct SDrcInfo : public IBoxInfo {
 public:
  /*!
   * @brief Creates SDrcInfo object from an active reader instance
   *
   * @note Needs be created via CIsobmffReader::specificBoxInfo<SDrcInfo> function call
   */
  SDrcInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl);

  /*!
   * @brief Returns serialized global 'ludt' data (e.g. 'tlou' and 'alou') for a given track index
   *
   * If no data is found, an empty buffer is returned
   */
  ilo::ByteBuffer globalLudtData(const uint32_t trackIndex) const;

  //! Checks if a track has 'ludt' updates in form of fragmented 'ludt' data
  bool trackHasLudtUpdates(const uint32_t trackIndex) const;

  /*!
   * @brief Returns serialized 'ludt' data updates (e.g. 'tlou' and 'alou') from one fragment
   *
   * If no data is found, an empty buffer is returned
   */
  ilo::ByteBuffer fragmentLudtData(const uint32_t trackIndex, const uint32_t fragmentNr) const;

 private:
  struct SPimpl;
  std::shared_ptr<SPimpl> m_pimpl;
};

/*!
 * @brief MPEG-D DRC information on file format level
 *
 * This is the extended version of SDrcInfo and returns information related
 * to MPEG-D DRC is a parsed format.
 *
 * Can be retrieved by calling CIsobmffReader::specificBoxInfo<SDrcExtendedInfo> on an instantiated
 * reader.
 * @note If there is no DRC relevant data in the MP4 file, any returned vector is an empty vector.
 *
 * \ingroup SpecificBoxInfo
 */
struct SDrcExtendedInfo : public IBoxInfo {
 public:
  //! Dataset for one measurement as defined in ISO/IEC 14496-12 12.2.7
  struct SMeasurementSet {
    uint8_t methodDefinition = 0;
    uint8_t methodValue = 0;
    uint8_t measurementSystem = 0;
    uint8_t reliability = 0;
  };

  //! Content of LoudnessBaseBox as defined in ISO/IEC 14496-12 12.2.7
  struct SBaseData {
    uint8_t eqSetId = 0;
    uint8_t downmixId = 0;
    uint8_t drcSetId = 0;
    int16_t bsSamplePeakLevel = 0;
    int16_t bsTruePeakLevel = 0;
    uint8_t measurementSystemForTp = 0;
    uint8_t reliabilityForTp = 0;
    std::vector<SMeasurementSet> measurementSets;
  };

  //! Loudness information grouped by type
  struct SLoudnessBaseInfo {
    ilo::Fourcc type;
    std::vector<SBaseData> baseData;
  };

  /*!
   * @brief Creates SDrcExtendedInfo object from an active reader instance
   *
   * @note Needs be created via CIsobmffReader::specificBoxInfo<SDrcExtendedInfo> function call
   */
  SDrcExtendedInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl);

  //! Returns all global 'ludt' box info structs (e.g. 'tlou' and 'alou') for a specific track index
  std::vector<SLoudnessBaseInfo> globalLudtData(const uint32_t trackIndex) const;

  //! Checks if a track has ludt updates in form of fragmented ludt data
  bool trackHasLudtUpdates(const uint32_t trackIndex) const;

  /*!
   * @brief Returns 'ludt' box updates as 'ludt' box info structs(e.g. 'tlou' and 'alou')
   *
   * from one fragment for a specific track index. If no data is found, an empty vector is returned
   */
  std::vector<SLoudnessBaseInfo> fragmentLudtData(const uint32_t trackIndex,
                                                  const uint32_t fragmentNr) const;

 private:
  struct SPimpl;
  std::shared_ptr<SPimpl> m_pimpl;
};

//-----------Iods Box Info-----------------

/*!
 * @brief InitialObjectDescriptor information
 *
 * Gathers information from an 'iods' box, if available (as defined in ISO/IEC 14496-1 7.2.6.4)
 *
 * Can be retrieved by calling CIsobmffReader::specificBoxInfo<SIodsInfo> on an instantiated reader.
 * @note Make sure to call SIodsInfo::iodsInfoAvailable before querying for data.
 *
 * \ingroup SpecificBoxInfo
 */
struct SIodsInfo : public IBoxInfo {
 public:
  /*!
   * @brief Creates SIodsInfo object from an active reader instance
   *
   * @note Needs be created via CIsobmffReader::specificBoxInfo<SIodsInfo> function call
   */
  SIodsInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl);

  //! Checks for an existing 'iods' box. If this returns false, accessing box information will
  //! result in an error.
  bool iodsInfoAvailable() const;

  //! Get audioProfileLevelIndication from 'iods' box as defined in ISO/IEC 14496-3
  uint8_t audioProfileLevelIndication() const;

 private:
  struct SIodsEntry {
    uint8_t audioProfileLevelIndication = 0;
  };

  std::unique_ptr<SIodsInfo::SIodsEntry> m_iodsEntry;
};
}  // namespace isobmff
}  // namespace mmt
