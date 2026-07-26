/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2021 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * Note: This file is the synchronous implementation as fallback in case asynchronous writing is not
 * available for a platform of if it was disabled.
 */

// System includes
#include <system_error>
#if defined(WIN32) || defined(_WIN32)
#include <io.h>
#include <wchar.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>

// Internal includes
#include "ilo/async_fileio.h"

namespace ilo {
static void throwLastErrorIf(bool predicate) {
  if (predicate) {
    throw std::system_error(static_cast<int>(errno), std::system_category());
  }
}

static os_file_t openForWriting(const std::string& utf8Path, bool append) {
  int creation = O_CREAT | O_WRONLY | (append ? O_APPEND : O_TRUNC);
  os_file_t file;
#if defined(WIN32) || defined(_WIN32)
  errno_t err = _sopen_s(&file, utf8Path.c_str(), creation, _SH_DENYNO, _S_IREAD | _S_IWRITE);
  throwLastErrorIf(file == -1 || err != 0);
#else
  file = open(utf8Path.c_str(), creation, 0644);
  throwLastErrorIf(file == -1);
#endif
  return file;
}

static os_file_t openForWriting(const std::wstring& widePath, bool append) {
  char utf8Path[FILENAME_MAX];
#if defined(WIN32) || defined(_WIN32)
  size_t convertedSize = 0;
  errno_t result = wcstombs_s(&convertedSize, utf8Path, widePath.c_str(), FILENAME_MAX - 1);
  throwLastErrorIf(result != 0);
#else
  size_t result = wcstombs(utf8Path, widePath.c_str(), FILENAME_MAX);
  throwLastErrorIf(result == static_cast<size_t>(-1));
#endif
  return openForWriting(utf8Path, append);
}

class CAsyncFileWriter::Operation {};

CAsyncFileWriter::CAsyncFileWriter(const std::string& path, bool append) {
  m_file = openForWriting(path, append);
}

CAsyncFileWriter::CAsyncFileWriter(const std::wstring& path, bool append) {
  m_file = openForWriting(path, append);
}

CAsyncFileWriter::~CAsyncFileWriter() {
#if defined(WIN32) || defined(_WIN32)
  _close(m_file);
#else
  close(m_file);
#endif
}

void CAsyncFileWriter::writeAsync(std::string&& data) {
  // Use synchronous IO since asynchronous writing is not available or disabled.
#if defined(WIN32) || defined(_WIN32)
  // In case our data size exceeds the unsigned int range, we just truncate the data.
  auto result = _write(m_file, data.data(), static_cast<unsigned int>(data.size()));
  throwLastErrorIf(result < 0);
#else
  ssize_t result = write(m_file, data.data(), data.size());
  throwLastErrorIf(result < 0);
#endif
}
}  // namespace ilo
