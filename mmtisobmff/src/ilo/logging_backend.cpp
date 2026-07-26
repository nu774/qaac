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

// System includes
#include <iostream>
#include "ilo_logging.h"

#if defined(WIN32) || defined(_WIN32)
#pragma warning(disable : 4996)
#define __func__ __FUNCTION__

#define NOMINMAX             // Disables conflicting min/max def from Windows.h
#define WIN32_LEAN_AND_MEAN  // Disables some rarely used includes in windows.h
#include <windows.h>
#pragma comment(lib, "advapi32.lib")
#endif

// Internal includes
#include "ilo/memory.h"

namespace ilo {
namespace impl {
CLogger& CLogger::instance() {
  static CLogger s_instance;
  return s_instance;
}

void CLogger::redirect_to_file(const std::string& fname, bool append) {
  m_fileWriter = ilo::make_unique<CAsyncFileWriter>(fname, append);
  m_enableSystemLog.store(false);
  m_disableLogging.store(false);
}

void CLogger::redirect_to_console() {
  m_fileWriter.reset();
  m_enableSystemLog.store(false);
  m_disableLogging.store(false);
}

void CLogger::redirect_to_system_log() {
  m_fileWriter.reset();
  m_enableSystemLog.store(true);
  m_disableLogging.store(false);
}

void CLogger::disable_logging() {
  m_fileWriter.reset();
  m_enableSystemLog.store(false);
  m_disableLogging.store(true);
}

void CLogger::print(const char* format, ...) {
  ILO_ASSERT(std::string(format) == "%s\n", "Disallowed usage of CLogger::print.");

  va_list args;
  va_start(args, format);
  const char* line = va_arg(args, const char*);
  va_end(args);

  log(line);
}

void CLogger::log(const char* line) {
  if (m_fileWriter) {
    m_fileWriter->writeAsync(std::string(line) + '\n');
  } else {
    puts(line);
  }
}

bool CLogger::isSystemLogEnabled() {
  return m_enableSystemLog.load();
}

bool CLogger::isLoggingDisabled() {
  return m_disableLogging.load();
}

ScopeLogger::ScopeLogger(const char* component, const char* function, const char* format,
                         va_list args)
    : comp(component), func(function) {
  print_entry(format, args);
}

ScopeLogger::~ScopeLogger() {
  if (!skip_exit_log) {
    print_exit();
  }
}

bool ScopeLogger::add_frame(LineType& line, int& write_at, const char* format, va_list args) const {
  if (write_at + 1 < static_cast<int>(line.size())) {
    line[write_at++] = ' ';

    auto written = xsnprintf(line.data() + write_at, line.size() - write_at, "%s (", func);

    if (counter_update_and_space_left(line, written, write_at)) {
      written = vsnprintf(line.data() + write_at, line.size() - write_at, format, args);

      if (counter_update_and_space_left(line, written, write_at)) {
        return true;
      }
    }
  }
  indicate_overlength(line);
  return false;
}

bool ScopeLogger::add_brackets(LineType& line, int& write_at) const {
  auto written = xsnprintf(line.data() + write_at, line.size() - write_at, ") {");
  return counter_update_and_space_left(line, written, write_at);
}

bool ScopeLogger::add_closing(LineType& line, int& write_at) const {
  if (write_at + 1 < static_cast<int>(line.size())) {
    int written;
    line[write_at++] = ' ';
    written = xsnprintf(line.data() + write_at, line.size() - write_at, "%s }", func);
    return counter_update_and_space_left(line, written, write_at);
  }
  return false;
}

void ScopeLogger::print_entry(const char* format, va_list args) const {
  LineType line;

  int write_at = 0;

  add_date_string(line, write_at) && add_thread_id_string(line, write_at) &&
      add_component_category(line, write_at, comp, "I") &&
      add_frame(line, write_at, format, args) && add_brackets(line, write_at);

  print_log(line, comp, "I");
}

void ScopeLogger::print_exit() const {
  LineType line;

  int write_at = 0;
  add_date_string(line, write_at) && add_thread_id_string(line, write_at) &&
      add_component_category(line, write_at, comp, "I") && add_closing(line, write_at);

  print_log(line, comp, "I");
}

#if defined(WIN32) || defined(_WIN32)
// Hint: Do not use template + inline, because of problems with Windows.h include in header file.
//       If Windows.h is included in logging.h, several symbol clashes will occure!
void print_log_windows(const char* line, const char* Component, const char* Category) {
  HANDLE hEventLog = NULL;
  LPCSTR lpszStrings[2] = {NULL, NULL};

  std::string component = std::string(Component);
  std::string message = std::string(line);
  lpszStrings[0] = component.c_str();
  lpszStrings[1] = message.c_str();

  // The source name (provider) must exist as a subkey of Application.
  hEventLog = RegisterEventSource(NULL, Component);
  if (hEventLog) {
    if (*Category == 'I') {
      ReportEvent(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 2, 0, (LPCSTR*)lpszStrings,
                  NULL);
    } else if (*Category == 'W') {
      ReportEvent(hEventLog, EVENTLOG_WARNING_TYPE, 0, 0, NULL, 2, 0, (LPCSTR*)lpszStrings, NULL);
    } else if (*Category == 'E') {
      ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 2, 0, (LPCSTR*)lpszStrings, NULL);
    } else {
      ReportEvent(hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 2, 0, (LPCSTR*)lpszStrings,
                  NULL);
    }

    DeregisterEventSource(hEventLog);
  }
}
#endif
}  // namespace impl
}  // namespace ilo
