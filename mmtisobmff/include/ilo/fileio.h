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
 * @file fileio.h
 * @brief Platform abstracted FILE(I/O) wrapper
 */

#pragma once

// System includes
#include <string>
#include <inttypes.h>

// Internal includes
#include "ilo/version.h"

#if defined(WIN32) || defined(_WIN32)
#include <cstdio>
#define ilo_dup _dup
#define ilo_dup2 _dup2
#define ilo_fileno _fileno
#define ilo_fseeko _fseeki64
#define ilo_ftello _ftelli64
#define ilo_close fclose
#elif defined __ANDROID__ && __ANDROID_API__ >= 24
// Overwrite with 64bit versions to allow LFS
// _FILE_OFFSET_BITS=64 was not working with NDK
// Only works with Android API level >= 24
#include <unistd.h>
#define ilo_dup dup
#define ilo_dup2 dup2
#define ilo_fileno fileno
#define ilo_fseeko fseeko64
#define ilo_ftello ftello64
#define ilo_close fclose
#else
#include <unistd.h>
#define ilo_dup dup
#define ilo_dup2 dup2
#define ilo_fileno fileno
#define ilo_fseeko fseeko
#define ilo_ftello ftello
#define ilo_close fclose
#endif

namespace ilo {
/*! \addtogroup FileHelpers Collection of helpers related to file workflows
 *  @{
 */

//! Platform abstract version of fopen
inline FILE* fopen(std::string filename, const char* mode) {
  FILE* file;
#if defined(WIN32) || defined(_WIN32)
  fopen_s(&file, filename.c_str(), mode);
#else
  file = ::fopen(filename.c_str(), mode);
#endif
  return file;
}

/**@}*/

/*!
 * @brief Platform abstract file handling wrapper. Automatically closes and flushes file on
 * destruction.
 * \ingroup FileHelpers
 */
class CFileWrapper {
 public:
  //! Modes to control file access
  enum class OpenMode {
    //! Open the file for binary reading (rb)
    read = 0,
    //! Open the file for binary writing (wb). Existing file will be overwritten.
    write,
    //! Open the file for binary reading/reading (w+b). Existing file will be overwritten.
    writeExtended
  };

  /*!
   * @brief Create file wrapper
   *
   * @param filename Path to the file to operate on
   * @param mode File mode defining how to open the file
   */
  explicit CFileWrapper(const std::string& filename, const OpenMode mode);
  ~CFileWrapper();

  //! Re-open the same file with a new mode
  void reopen(const OpenMode mode);
  //! Get size of the file in bytes
  uint64_t getSize();
  //! Return the raw file pointer
  FILE* get();
  //! Get file name of the currently used file
  std::string filename();
  //! Close the file handle (removes any file-locks and ensures flushing to disc)
  void close();

 private:
  void open(const std::string& filename, const OpenMode mode);

 private:
  FILE* m_file = nullptr;
  std::string m_filename;
};
}  // namespace ilo
