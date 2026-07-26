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
 * @file input.h
 * @brief Interface for OS agnostic file and memory input
 * \defgroup input Main interface for handling file and memory input
 *
 * Main interface for file and memory input abstraction
 */

#pragma once

// System includes
#include <string>
#include <fstream>
#include <stdexcept>

#include <stdio.h>

// External includes
#include "ilo/common_types.h"
#include "ilo/fileio.h"

// Internal includes
#include "mmtisobmff/types.h"

namespace mmt {
namespace isobmff {
/*!
 * @brief Input interface
 *
 * Input interface that is used by @ref CIsobmffReader to read files from either disk or memory.
 */
struct IIsobmffInput {
  virtual ~IIsobmffInput() {}

  /*!
   * @brief Read data from input into a buffer
   *
   * The user must provide a pre-allocated buffer to read into. The number of bytes to read
   * from the input is determined by the begin and end iterators of the buffer provided.
   *
   * @note The returned size must be checked. In case the input has less data left, the returned
   * size will signal real available data in the target buffer.
   *
   * @note begin and end iterators must both point to the same underlying buffer structure
   * and memory must be accessible in a continuous way.
   */
  virtual size_t read(ilo::ByteBuffer::iterator inBegin, ilo::ByteBuffer::iterator inEnd) = 0;
  //! Function to seek to a fixed position in the input stream
  virtual void seek(pos_type pos) = 0;
  //! Function to seek relative to a given origin
  virtual void seek(offset_type offset, SeekingOrigin origin) = 0;
  //! Function to get the current reading position in the stream in bytes
  virtual pos_type tell() = 0;
  //! Function to check if input is "end of input"
  virtual bool isEOI() = 0;
  //! Clones the input
  virtual std::unique_ptr<IIsobmffInput> clone() = 0;
};

/*!
 * @brief Implementation of a file input reader
 *
 * Reads files from disk.
 *
 * \ingroup input
 */
struct CIsobmffFileInput : public IIsobmffInput {
  /*!
   * @brief File input constructor
   *
   * @param filename Path to the input file
   */
  explicit CIsobmffFileInput(const std::string& filename)
      : m_file(filename, ilo::CFileWrapper::OpenMode::read), m_filename(filename) {}

  /*!
   * @brief Read data from file input into a buffer
   *
   * The user must provide a pre-allocated buffer to read into. The number of bytes to read is
   * determined by the distance between the given begin and end iterators of the output buffer.
   *
   * @note The returned size must be checked. In case the input has less data left than requested,
   * the returned size will signal the number of bytes actually written to the output buffer.
   *
   * @note begin and end iterators must both point to the same underlying buffer structure
   * and memory must be accessible in a continuous way.
   */
  virtual size_t read(ilo::ByteBuffer::iterator inBegin, ilo::ByteBuffer::iterator inEnd) override {
    size_t len = static_cast<size_t>(inEnd - inBegin);
    char* buffer = reinterpret_cast<char*>(&(*inBegin));
    size_t actuallyRead = fread(buffer, sizeof(uint8_t), len, m_file.get());

    return actuallyRead;
  }

  /*!
   * @brief Function to seek to a fixed position in the input file
   *
   * When called, the reader pointer for the next @ref read call is set to parameter pos.
   *
   * @param pos Position in bytes relative to file start at which to continue reading with the next
   * @ref read call.
   */
  virtual void seek(pos_type pos) override {
    int err = ilo_fseeko(m_file.get(), pos, SEEK_SET);

    if (err != 0) {
      throw std::runtime_error("Could not seek to position");
    }
  }

  /*!
   * @brief Function to seek relative to a given origin
   *
   * When called, the pointer to read from with a future @ref read call is set to an
   * offset relative to origin.
   *
   * @param offset Offset in bytes to seek to relative to origin.
   *              A positive value indicates seeking towards the end,
   *              a negative value seeking towards the front.
   * @param origin Origin to start seeking at.
   */
  virtual void seek(offset_type offset, SeekingOrigin origin) override {
    int err = 0;

    switch (origin) {
      case SeekingOrigin::beg:
        seek((pos_type)offset);
        break;
      case SeekingOrigin::end:
        err = ilo_fseeko(m_file.get(), offset, SEEK_END);
        break;
      case SeekingOrigin::cur:
        err = ilo_fseeko(m_file.get(), offset, SEEK_CUR);
        break;
    }

    if (err != 0) {
      throw std::runtime_error("Could not seek to position");
    }
  }

  //! Function to get the current position in the stream
  virtual pos_type tell() override { return static_cast<pos_type>(ilo_ftello(m_file.get())); }

  //! Function to check if input is "end of input"
  virtual bool isEOI() override {
    auto c = fgetc(m_file.get());
    ungetc(c, m_file.get());
    return feof(m_file.get()) != 0;
  }

  //! Clones the input
  virtual std::unique_ptr<IIsobmffInput> clone() override {
    return std::unique_ptr<IIsobmffInput>(new CIsobmffFileInput(m_filename));
  }

 private:
  ilo::CFileWrapper m_file;
  std::string m_filename;
};

/*!
 * @brief Implementation of a memory input reader
 *
 * Reads data from a buffer backed input.
 *
 * \ingroup input
 */
struct CIsobmffMemoryInput : public IIsobmffInput {
  /*!
   * @brief Memory input constructor
   *
   * This input works on an externally managed buffer from which data can be read.
   *
   * @param buff Externally managed shared buffer containing the data that should be read.
   */
  explicit CIsobmffMemoryInput(std::shared_ptr<const ilo::ByteBuffer> buff)
      : buffer(buff), ptr(buffer->begin()) {}

  /*!
   * @brief Read data from a buffer backed input
   *
   * The user must provide a pre-allocated buffer to read into. The number of bytes to read is
   * determined by the distance between the given begin and end iterators of the output buffer.
   *
   * @note The returned size must be checked. In case the input has less data left than requested,
   * the returned size will signal the number of bytes actually written to the output buffer.
   *
   * @note begin and end iterators must both point to the same underlying buffer structure
   * and memory must be accessible in a continuous way.
   */
  virtual size_t read(ilo::ByteBuffer::iterator inBegin, ilo::ByteBuffer::iterator inEnd) override {
    auto copyCount =
        std::min(static_cast<size_t>(inEnd - inBegin), static_cast<size_t>(buffer->end() - ptr));

    std::copy(ptr, ptr + copyCount, inBegin);

    ptr += copyCount;
    return copyCount;
  }

  /*!
   * @brief Function to seek to a fixed position in the input buffer
   *
   * When called, the reader pointer for the next @ref read call is set to parameter pos.
   *
   * @param pos Position in bytes relative to buffer begin at which to continue reading with the
   * next @ref read call.
   */
  virtual void seek(pos_type pos) override {
    if (pos > buffer->size()) {
      throw std::out_of_range("Position to seek to is out of range");
    }
    ptr = buffer->begin() + static_cast<ilo::ByteBuffer::difference_type>(pos);
  }

  /*!
   * @brief Function to seek relative to a given origin
   *
   * When called, the reader pointer for the next @ref read call is set to an
   * offset relative to origin.
   *
   * @param offset Offset in bytes to seek to (relative to origin).
   *              A positive value indicates seeking towards the end,
   *              a negative value seeking towards the front.
   * @param origin Origin to start seeking at.
   */
  virtual void seek(offset_type offset, SeekingOrigin origin) override {
    switch (origin) {
      case SeekingOrigin::beg:
        seek((pos_type)offset);
        break;
      case SeekingOrigin::end:
        if (offset > 0 || buffer->size() < static_cast<uint64_t>(std::abs(offset))) {
          throw std::out_of_range("Position to seek to is out of range");
        }
        ptr = buffer->end() + static_cast<ilo::ByteBuffer::difference_type>(offset);
        break;
      case SeekingOrigin::cur:
        if (offset > 0 && offset > (buffer->end() - ptr)) {
          throw std::out_of_range("Position to seek to is out of range");
        }

        if (offset < 0 && offset < (buffer->begin() - ptr)) {
          throw std::out_of_range("Position to seek to is out of range");
        }

        ptr += static_cast<ilo::ByteBuffer::difference_type>(offset);
        break;
    }
  }

  //! Function to get the current reading position in the stream in bytes
  virtual pos_type tell() override { return static_cast<pos_type>(ptr - buffer->begin()); }

  //! Function to check if input is "end of input"
  virtual bool isEOI() override { return ptr == buffer->end(); }

  //! Clones the input
  virtual std::unique_ptr<IIsobmffInput> clone() override {
    return std::unique_ptr<IIsobmffInput>(new CIsobmffMemoryInput(buffer));
  }

 private:
  std::shared_ptr<const ilo::ByteBuffer> buffer;
  ilo::ByteBuffer::const_iterator ptr;
};
}  // namespace isobmff
}  // namespace mmt
