/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2017 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file writer.h
 * @brief Interface for file writing MP4 files
 * \defgroup mp4writer Main interface to create an MP4 writer
 *
 * Main interface to create an MP4 writer
 */

#pragma once

// System includes
#include <memory>
#include <vector>

// External includes
#include "ilo/common_types.h"
#include "ilo/string_utils.h"

// Internal includes
#include "mmtisobmff/writer/output.h"

namespace mmt {
namespace isobmff {

//! Supported Stream Access Point types
enum class ESapType : uint8_t {
  SapTypeInvalid = 0, /**< Unknown stream access type */
  SapType1 = 1,       /**< Indicates a sync sample (see codec standard) */
  SapType2 = 2,       /**< Indicates a sync sample (see codec standard) */
  SapType3 = 3,       /**< SAP is marked as a member of a sample group of type 'rap' */
  SapType4 =
      4, /**< SAP is marked as a member of a sample group of type 'roll', with rolldistance > 0 */
  SapType5 = 5, /**< No specific signalling in isobmff standard supported */
  SapType6 = 6  /**< No specific signalling in isobmff standard supported */
};

//! Config for the Segment Index Box
struct SSidxConfig {
  ESapType sapType = ESapType::SapTypeInvalid;
};

//! Config for the Initial Object descriptor (Can be used for AAC based codecs)
struct SIodsConfig {
  /*! @note A value of 0xFF means "no audio capability required" as described in ISO/IEC 14496-3 */
  uint8_t audioProfileLevelIndication = 0xFF;
};

//! General config to create an CIsobmffWriter instance
struct SMovieConfig {
  SMovieConfig() {}
  SMovieConfig(SMovieConfig&& otherConf);

  //! Required value, defining the major brand being used for the MP4 file
  ilo::Fourcc majorBrand = ilo::toFcc("0000");
  //! Optional value, defining compatible brand sets used for the MP4 file
  std::vector<ilo::Fourcc> compatibleBrands;
  //! Optional value, movie time scale (default is 600)
  uint32_t movieTimeScale = 600;
  //! Optional value, creation/modification time (default is to autogenerate it)
  uint64_t currentTimeInUtc = 0;
  //! Optional value, forces baseMediaDecodeTime to be 64bit in size (default to autodetect it)
  bool forceTfdtBoxV1 = false;
  //! Optional value, create and set the sidxConfig to write an 'sidx' box (default is off)
  std::unique_ptr<SSidxConfig> sidxConfig = nullptr;
  //! Optional value, create and set the iodsConfig to write an 'iods' box (default is off)
  std::unique_ptr<SIodsConfig> iodsConfig = nullptr;
  /*!
   * @brief Optional value, create and set the userData to enable user data writing on movie level
   * (default is none)
   *
   * Each call of this function will generate a child box in the 'udta' container box in 'moov'.
   * The structure of the user data buffer is defined in ISO/IEC 14496-12, Clause 4.2 and looks
   * likes this:
   * @code
   * unsigned int(32) size (in bytes)
   * unsigned int(32) fourCC
   * unsigned int(8*(size - 8 bytes)) payload
   * @endcode
   *
   * @note The buffer structure must all be big endian style.
   */
  std::vector<ilo::ByteBuffer> userData;
};

struct ITrackWriter;

/*!
 * @brief MP4 writer interface
 *
 * CIsobmffWriter is the main starting point of the writing api.
 * There are several different writer types (e.g. file, memory, plain,
 * fragmented, segmented, etc.). It also gives access to codec specific
 * track writer to write samples.
 *
 * One writer instance can be used to create multiple tracks by registering
 * several track writers. If several track writers are active on the same
 * file writer, the output will be multiplexed.
 */
struct CIsobmffWriter {
  CIsobmffWriter();
  virtual ~CIsobmffWriter();

  /*!
   * @brief Creates a track writer from this reader
   *
   * Main entry point for writing track related data. Multiple track writers can
   * be active to write a multiplexed MP4 file.
   *
   * @code
   * auto twriter = writer.trackWriter<type>(config)
   * @endcode
   * With 'type' being a valid track writer class and 'config' being a
   * configuration struct defined for that particular track writer.
   *
   * @param config Config to initialize a specific track writer
   * @return Track writer for writing track based information e.g. samples
   *
   * See @ref mp4trackwriter for available track writers
   */
  template <class type, class configType>
  std::unique_ptr<type> trackWriter(const configType& config) {
    static_assert(std::is_base_of<ITrackWriter, type>::value,
                  "queried type must be a descendant of ITrackWriter");

    auto twriter = std::unique_ptr<type>(new type(std::weak_ptr<CIsobmffWriter::Pimpl>(p), config));
    return twriter;
  }

  /*!
   * @brief Creates media fragments from the currently added samples
   *
   * @note Can only be called from @ref CIsobmffFragFileWriter
   */
  virtual void createMediaFragments();
  /*!
   * @brief Creates an init segment containing only static meta data and writes it to file
   *
   * @note Can only be called from @ref CIsobmffFragFileSegWriter
   */
  virtual void createInitFileSegment(const std::string&);
  /*!
   * @brief Creates a media segment containing an arbitrary number of fragments and writes it to
   * file
   *
   * @note Can only be called from @ref CIsobmffFragFileSegWriter
   */
  virtual void createMediaFileSegment(const std::string&, const bool&);
  /*!
   * @brief Creates an init segment containing only static metadata and write it to a ByteBuffer
   *
   * @note Can only be called from @ref CIsobmffFragMemoryWriter
   */
  virtual ilo::CUniqueBuffer createInitSegment();
  /*!
   * @brief Creates a media segment containing 1 to n fragments and write it to a ByteBuffer
   *
   * @note Can only be called from @ref CIsobmffFragMemoryWriter
   */
  virtual ilo::CUniqueBuffer createMediaMemSegment(const bool&, const bool&);
  /*!
   * @brief Creates a serialized byte stream of a non fragmented, plain MP4 file
   *
   * @note Can only be called from @ref CIsobmffMemoryWriter
   */
  virtual ilo::CUniqueBuffer serialize();
  /*!
   * @brief Close the write instance, finalize data and delete temporary data
   *
   * @note Should always be called to ensure proper shutdown of the library
   */
  virtual void close();

  struct Pimpl;

 protected:
  std::shared_ptr<Pimpl> p;
};

//! Base Fragmented writer
struct CIsobmffBaseFragWriter : CIsobmffWriter {
  CIsobmffBaseFragWriter(std::unique_ptr<IIsobmffOutput>&& output, const SMovieConfig& config);
  virtual ~CIsobmffBaseFragWriter();
};

/*!
 * @brief Fragmented MP4 file writer (one file with init and fragments)
 *
 * Main entry point for writing a fragmented MP4 file to disc.
 * The fragmented file will be written as one big file containing
 * fragments in the format [file-start 'moov' 'moof' 'moof' 'moof' ... file-end]
 * It will not contain any 'sidx' or 'stype' boxes.
 *
 * \ingroup mp4writer
 */
struct CIsobmffFragFileWriter : public CIsobmffBaseFragWriter {
  struct SOutputConfig {
    std::string outputUri;
  };

  CIsobmffFragFileWriter(const SOutputConfig& outConf, const SMovieConfig& config);
  ~CIsobmffFragFileWriter();

  /*!
   * @brief Creates a media fragment of the added samples
   *
   * When writing a fragmented MP4 file, all samples added via a track writer
   * will temporary be held in memory. Calling this function will instruct the
   * library to write all currently uncommitted samples to disk.
   *
   * To define what sample will be present in which fragment, the fragmentNumber
   * of the samples added via the track writer must be set.
   *
   * This function can be called multiple times. It is usually advised to call this function
   * directly before increasing the fragment counter on the sample.
   * In case 'sidx' box writing is enabled, calling this function will also
   * create a new entry in the 'sidx' box.
   */
  void createMediaFragments() override;
  /*!
   * @brief Closes the library discarding any unwritten data.
   *
   * This function should be always called at the very end to close
   * the library. It will ensure the 'moov' index table entries are written,
   * the file handles are flushed and temporary files are removed.
   *
   * @note Any non committed data via @ref createMediaFragments is discarded.
   */
  void close() override;
};

/*!
 * @brief File Segmented MP4 file writer (init segment and media segments separate)
 *
 * Main entry point for writing file segmented MP4 files (for e.g. DASH segment
 * template or general CMAF based HLS live streaming.
 *
 * In contrast to @ref CIsobmffFragFileWriter the output will be physically separated
 * file segments in the format [file-start 'moov' file-end] [file-start 'styp' 'moof' file-end]
 * [file-start 'styp' 'moof' file-end].
 *
 * In case multiple fragments are placed into one file segment, the output looks like:
 * [file-start 'moov' file-end] [file-start 'styp' 'moof' 'moof' ... file-end]
 * [file-start 'styp' 'moof' 'moof' ... file-end].
 *
 * \ingroup mp4writer
 */
struct CIsobmffFragFileSegWriter : CIsobmffBaseFragWriter {
  CIsobmffFragFileSegWriter(const SMovieConfig& config);

  /*!
   * @brief Creates an init segment containing only static metadata and write it to file
   *
   * This will create a file of just containing the 'moov' section of the MP4 file.
   * The 'moov' will not contain any sample related meta data in this case, but only
   * static metadata. Needs to be called only once.
   */
  void createInitFileSegment(const std::string& segOutputUri) override;
  /*!
   * @brief Creates a media segment containing the added samples since last call
   *
   * This will write all uncommitted samples since the last call into a separate file segment.
   * All samples will remain im memory until called.
   *
   * The call can write multiple fragments into one file segment if needed. To define what
   * sample will be present in which fragment, the fragmentNumber of the samples
   * added via the track writer must be set.
   *
   * @note Usually one file segment contains only one fragment.
   *
   * If the last segment is being created, isLastSegment should be set to true to indicate
   * this via 'lmsg' compatibility brand in 'styp' box.
   */
  void createMediaFileSegment(const std::string& segOutputUri, const bool& isLastSegment) override;
};

/*!
 * @brief Fragmented MP4 memory writer (init segment and media segments separate)
 *
 * @brief Main entry point for writing mp4 segments directly to buffer.
 *
 * In contrast to @ref CIsobmffFragFileSegWriter the output will be outputted to buffers instead of
 * files in the format [buffer-start 'moov' buffer-end] [buffer-start 'styp' 'moof' buffer-end]
 * [buffer-start 'styp' 'moof' buffer-end].
 *
 * In case multiple fragments are placed into one buffer media segment, the output looks like:
 * [buffer-start 'moov' buffer-end] [buffer-start 'styp' 'moof' 'moof' ... buffer-end]
 * [buffer-start 'styp' 'moof' 'moof' ... buffer-end].
 *
 * There is an extra switch to control whether the 'styp' box should be written.
 *
 * \ingroup mp4writer
 */
struct CIsobmffFragMemoryWriter : CIsobmffBaseFragWriter {
  CIsobmffFragMemoryWriter(const SMovieConfig& config);

  /*!
   * @brief Creates an init segment containing only static metadata and writes it into a buffer
   *
   * This will create a buffer containing only the 'moov' section of the MP4 file.
   * The 'moov' will not contain any sample related meta data in this case, but only
   * static metadata. Needs to be called only once.
   */
  ilo::CUniqueBuffer createInitSegment() override;
  /*!
   * @brief Creates a media segment containing the added samples since last call
   *
   * The call can write multiple fragments into one media segment buffer if needed. To define what
   * sample will be present in which fragment, the fragmentNumber parameter of the samples been
   * added via the track writer must be set.
   *
   * @note Usually one media segment buffer contains only one fragment.
   *
   * If the last segment is being created, the isLastSegment parameter should be set to true to
   * indicate this via 'lmsg' compatible brand in 'styp' box.
   *
   * @param useStyp If set to true, add 'styp' box at the start of media segment
   * @param isLastSegment If set to true, signal last segment on 'styp' box. Shall only be used for
   * the last segment being created.
   */
  ilo::CUniqueBuffer createMediaMemSegment(const bool& useStyp, const bool& isLastSegment) override;
};

//! Base writer
struct CIsobmffBaseWriter : CIsobmffWriter {
  CIsobmffBaseWriter(const std::string& outUri, const std::string& tmpUri,
                     const SMovieConfig& config, const bool memoryWriting = false);
};

/*!
 * @brief Flat (plain, non fragmented) MP4 file writer
 *
 * Main entry point for writing MP4 segments.
 *
 * Create a standard (plain, non fragmented) MP4 file on disc. The structure is
 * suitable for progressive download putting the 'moov' element containing all
 * track and sample specific information at the beginning of the file and the
 * 'mdat' payload section at the very end.
 *
 * This means all samples added via a track writer will be written into a temporary file first.
 * When calling close, the temporary file will be read and multiplexed into the final MP4 file. This
 * can take a while for big files and slow discs and the close call will block until finished.
 *
 * @note It is advised to always call close at the end to ensure everything is written to disc.
 *
 * \ingroup mp4writer
 */
struct CIsobmffFileWriter : public CIsobmffBaseWriter {
  struct SOutputConfig {
    //! Output File Uri to write the final MP4 file to (required)
    std::string outputUri;
    /*!
     * @brief Path to a temporary file that is used for intermediate data (optional)
     *
     * If not specified, a unique temporary file in system tmp path will be used.
     */
    std::string tmpUri;
  };

  CIsobmffFileWriter(const SOutputConfig& outConf, const SMovieConfig& config);
  ~CIsobmffFileWriter();

  /*!
   * @brief Close the library, flush the data, process and remove temporary files
   *
   * When calling close, any temporary file will be read and multiplexed into the final MP4 file.
   * This can take a while for big files and slow discs and the close call will block until
   * finished.
   */
  void close() override;
};

/*!
 * @brief Flat (plain, non-fragmented) MP4 memory writer
 *
 * Special version of the plain, non-fragmented MP4 writer. Instead of
 * writing to a file it will hold all samples added in memory until the user
 * wants to serialise the MP4 structure into a buffer.
 *
 * \ingroup mp4writer
 */
struct CIsobmffMemoryWriter : public CIsobmffBaseWriter {
  CIsobmffMemoryWriter(const SMovieConfig& config);
  ~CIsobmffMemoryWriter();

  /*!
   * @brief Create memory representation of the collected data in form of an MP4 file serialized to
   * a ByteBuffer
   *
   * Calling will create a full non-fragmented MP4 structure and serializes it into a buffer. Only
   * call this function after all samples were added via track writer.
   *
   * @note Can only be called once. If called multiple times a nullptr is returned and a warning is
   * logged.
   */
  ilo::CUniqueBuffer serialize() override;

  //! Close the library and throw away any data not serialized until now
  void close() override;
};
}  // namespace isobmff
}  // namespace mmt
