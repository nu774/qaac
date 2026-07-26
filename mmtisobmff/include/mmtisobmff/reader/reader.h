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
 * @file reader.h
 * @brief Interface for reading MP4 files
 * \defgroup mp4reader Main interface to create an MP4 reader
 *
 * Main interface to create an MP4 reader
 */

#pragma once

// System includes
#include <memory>
#include <cstdint>
#include <algorithm>
#include <type_traits>

// External includes
#include "ilo/string_utils.h"

// Internal includes
#include "mmtisobmff/types.h"
#include "mmtisobmff/reader/input.h"

namespace mmt {
namespace isobmff {

//! Interface for box info struct
struct IBoxInfo {
  virtual ~IBoxInfo() {}
};

/*!
 * @brief Struct containing info about the global "movie" data of the file
 *
 * Can be queried by @ref CIsobmffReader::movieInfo
 *
 * \ingroup mp4reader
 */
struct CMovieInfo {
  //! Major brand of the movie
  ilo::Fourcc majorBrand = ilo::toFcc("0000");
  //! Compatible brands of the movie
  std::vector<ilo::Fourcc> compatibleBrands;
  //! Movie creation time in seconds since 1904-01-01 00:00:00
  uint64_t creationTime = 0;
  //! Movie modification time in seconds since 1904-01-01 00:00:00
  uint64_t modificationTime = 0;
  //! Time scale of the movie
  uint32_t timeScale = 0;
  //! Duration of the movie in time scale ticks
  uint64_t duration = 0;
  /*!
   * @brief User defined data at movie level
   *
   * If available, the vector will include user data found on movie ('moov') level.
   *
   * The structure of the user data buffer is defined in ISO/IEC 14496-12, Clause 4.2 and looks
   * likes this:
   * @code
   * unsigned int(32) size (in bytes)
   * unsigned int(32) fourCC
   * unsigned int(8*(size - 8 bytes)) payload
   * @endcode
   *
   * @note The buffer structure is big endian style.
   */
  std::vector<ilo::ByteBuffer> userData;
};

/*!
 * @brief Struct containing info about the track related data of the file
 *
 * Can be queried by @ref CIsobmffReader::trackInfos
 *
 * \ingroup mp4reader
 */
struct CTrackInfo {
  CTrackInfo() : handler(ilo::toFcc("0000")), codingName(ilo::toFcc("0000")) {}
  //! ID of the track (i.e. unique identifier within the file)
  uint32_t trackId = 0;
  //! Track Index (i.e. the index of the track in the trackInfo vector)
  uint32_t trackIndex = 0;
  //! Track handler (for debugging and inspection purposes).
  ilo::Fourcc handler = ilo::toFcc("0000");
  //! Track coding name as specified in the sample entry
  ilo::Fourcc codingName = ilo::toFcc("0000");
  //! Track type @see TrackType
  TrackType type = TrackType::undefined;
  //! Track codec @see Codec
  Codec codec = Codec::undefined;
  //! Track/media time scale
  uint32_t timescale = 0U;
  //! Track duration in time scale ticks
  uint64_t duration = 0U;
  //! Track language
  ilo::IsoLang language = ilo::toIsoLang("und");
  //! Maximum sample size in bytes
  size_t maxSampleSize = 0;
  //! Sample count (total number of samples in this track)
  size_t sampleCount = 0;
  //! Edit list of the track
  SEditList editList;
  /*!
   * @brief User defined data at track level
   *
   * If available, the vector will include user data found on track ('trak'/'traf') level.
   *
   * The structure of the user data buffer is defined in ISO/IEC 14496-12, Clause 4.2 and looks
   * likes this:
   * @code
   * unsigned int(32) size (in bytes)
   * unsigned int(32) fourCC
   * unsigned int(8*(size - 8 bytes)) payload
   * @endcode
   *
   * @note The buffer structure is big endian style.
   */
  std::vector<ilo::ByteBuffer> userData;
};

//! Information about all tracks found in the MP4 file
using CTrackInfoVec = std::vector<CTrackInfo>;

struct ITrackReader;

/*!
 * @brief MP4 reader interface
 *
 * CIsobmffReader is the main starting point of the reading API.
 *
 * It can read data from several input types and provides access to
 * general movie and track related data.
 *
 * Several track readers can be active on a single reader to read
 * multiplexed MP4 files.
 *
 * \ingroup mp4reader
 */
struct CIsobmffReader {
  /*!
   * @brief Create an MP4 reader instance
   *
   * Creates instance of an MP4 reader. The type of reader (file, memory, etc.) is
   * determined by the type of input used for initialization.
   *
   * @param input @ref IIsobmffInput object to read from.
   *
   * @note Input ownership gets transferred into instance during instance construction
   * @warning The reader requires the input to always contain the 'moov' section. For
   * reading file or memory segments, the init and media segment must first be
   * concatenated before they can be read.
   */
  CIsobmffReader(std::unique_ptr<IIsobmffInput>&& input);

  //! Get movie information
  CMovieInfo movieInfo() const;
  //! Get number of tracks contained in the file
  size_t trackCount() const;
  //! Get track information vector
  CTrackInfoVec trackInfos() const;

  /*!
   * @brief Create MP4 reader instance
   *
   * Main entry point for reading track related data. Create track reader instance
   * by index (starting with 0) of the specified class (must be of type ITrackReader).
   *
   * Multiple track readers can be active to read a multiplexed MP4 file.
   *
   * @code
   * auto treader = reader.trackByIndex<type>(trackNumber);
   * @endcode
   * With 'type' being a valid track reader class and 'trackNumber' being a
   * 0-based index indicating from which track to create the reader.
   *
   * See @ref mp4trackreader for available track readers
   *
   * @param trackNumber 0-based index indicating for which track to create the reader.
   * @return Track reader for reading track based information e.g. samples.
   */
  template <class type>
  std::unique_ptr<type> trackByIndex(size_t trackNumber) const {
    static_assert(std::is_base_of<ITrackReader, type>::value,
                  "queried type must be a descendant of ITrackReader");

    return std::unique_ptr<type>(new type(std::weak_ptr<CIsobmffReader::Pimpl>(p), trackNumber));
  }

  /*!
   * @brief Interface to query information for specific use cases
   *
   * Gathers lower level info from boxes and present them grouped into
   * use-cases.
   *
   * @code
   * auto info = reader.specificBoxInfo<type>();
   * @endcode
   * With 'type' being a valid box info class implementing @ref IBoxInfo.
   *
   * @see SpecificBoxInfo
   */
  template <class type>
  std::unique_ptr<type> specificBoxInfo() const {
    static_assert(std::is_base_of<IBoxInfo, type>::value,
                  "queried type must be a descendant of IBoxInfo");

    return std::unique_ptr<type>(new type(std::weak_ptr<Pimpl>(p)));
  }

  struct Pimpl;

 private:
  std::shared_ptr<Pimpl> p;
};
}  // namespace isobmff
}  // namespace mmt
