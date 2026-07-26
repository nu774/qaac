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
 * @file logging_backend.h
 * @brief Backend for platform abstracted logger
 */

#pragma once

// System includes
#include <array>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <thread>
#include <iomanip>
#include <cstdarg>
#include <string.h>
#include <fcntl.h>
#include <atomic>
#include <mutex>

#ifdef __EMSCRIPTEN__
#include <iostream>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

#if defined __APPLE__ && !defined __EMSCRIPTEN__
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101200
#include <syslog.h>
#define MMT_FORCE_SYSLOG
#else
#include <os/log.h>
#endif
#endif

#if defined __linux__ && !defined __ANDROID__ && !defined __EMSCRIPTEN__
#include <syslog.h>
#endif

// Internal includes
#include "ilo/version.h"
#include "ilo/async_fileio.h"

#if __cplusplus >= 201402L
#define ILO_DEPRECATED [[deprecated]]
#elif defined(__GNUC__) || defined(__clang__)
#define ILO_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define ILO_DEPRECATED __declspec(deprecated)
#else
#define ILO_DEPRECATED
#endif

using LineType = std::array<char, 512>;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla-extension"
#endif

namespace ilo {
namespace impl {
#ifdef __ANDROID__
template <class line_type>
inline void print_log_system(const line_type& line, const char* Component, const char* Category) {
  android_LogPriority prio = ANDROID_LOG_DEFAULT;

  if (*Category == 'I') {
    prio = ANDROID_LOG_INFO;
  } else if (*Category == 'W') {
    prio = ANDROID_LOG_WARN;
  } else if (*Category == 'E') {
    prio = ANDROID_LOG_ERROR;
  }

  __android_log_write(prio, Component, line.data());
}
#endif

#ifdef __EMSCRIPTEN__
template <class line_type>
inline void print_log_system(const line_type& line, const char* Component, const char* Category) {
  if (*Category == 'I') {
    std::cout << line.data();
  } else if (*Category == 'W') {
    std::cout << line.data();
  } else if (*Category == 'E') {
    std::cerr << line.data();
  }
}
#endif

#if defined __APPLE__ && !defined MMT_FORCE_SYSLOG && !defined __EMSCRIPTEN__
template <class line_type>
inline void print_log_system(const line_type& line, const char* /*Component*/,
                             const char* Category) {
  switch (*Category) {
    case 'I':
      os_log_info(OS_LOG_DEFAULT, "%{public}s", line.data());
      break;
    case 'W':
      os_log_info(OS_LOG_DEFAULT, "%{public}s", line.data());
      break;
    case 'E':
      os_log_error(OS_LOG_DEFAULT, "%{public}s", line.data());
      break;
    default:
      os_log(OS_LOG_DEFAULT, "%{public}s", line.data());
      break;
  }
}
#endif

#if defined __linux__ && !defined __ANDROID__ || defined MMT_FORCE_SYSLOG
template <class line_type>
inline void print_log_system(const line_type& line, const char* Component, const char* Category) {
  openlog(Component, LOG_PID | LOG_CONS, LOG_USER);
  if (*Category == 'I') {
    syslog(LOG_INFO, "%s", line.data());
  } else if (*Category == 'W') {
    syslog(LOG_WARNING, "%s", line.data());
  } else if (*Category == 'E') {
    syslog(LOG_ERR, "%s", line.data());
  } else {
    syslog(LOG_INFO, "%s", line.data());
  }
  closelog();
}
#endif

#if defined _WIN32 && !defined __EMSCRIPTEN__
void print_log_windows(const char* line, const char* Component, const char* Category);

template <class line_type>
inline void print_log_system(const line_type& line, const char* Component, const char* Category) {
  print_log_windows(line.data(), Component, Category);
}
#endif

/*!
 * @brief Logger implementation
 * \ingroup Logging
 */
class CLogger {
 public:
  //! Access static instance of the logger
  static CLogger& instance();

  /*!
   * @brief Redirect logging to file
   *
   * @param fname File path (including file name) to write log file to.
   * @param append If set to true, append to existing file. Otherwise, file is being overwritten.
   */
  void redirect_to_file(const std::string& fname, bool append = false);
  //! Redirect logging to console output
  void redirect_to_console();
  //! Redirect logging to default system logger implementation
  void redirect_to_system_log();
  //! Disable logging output
  void disable_logging();
  //! Deprecated, remove in ilo v3
  ILO_DEPRECATED void print(const char* format, ...);
  //! Log content to the currently selected logger system
  void log(const char* line);
  //! Query if system logger is currently selected
  bool isSystemLogEnabled();
  //! Query if logging is currently disabled
  bool isLoggingDisabled();

 private:
  CLogger() = default;

 private:
  std::unique_ptr<CAsyncFileWriter> m_fileWriter;
  std::atomic<bool> m_enableSystemLog{false};
  std::atomic<bool> m_disableLogging{false};
};

inline int xsnprintf(char* buffer, size_t count, const char* format, ...) {
  va_list args;
  va_start(args, format);
  auto written = vsnprintf(buffer, count, format, args);
  va_end(args);

  if (-1 == written) {
    return static_cast<int>(count) + 1;
  }
  return static_cast<int>(written);
}

// All logs use this function in the end
template <class line_type>
inline void print_log(const line_type& line, const char* Component, const char* Category) {
  if (CLogger::instance().isLoggingDisabled()) {
    // Just return.
    return;
  }
  if (CLogger::instance().isSystemLogEnabled()) {
    print_log_system(line, Component, Category);
    return;
  }
  CLogger::instance().log(line.data());
}

template <class line_type>
inline void indicate_overlength(line_type& line) {
  *(line.end() - 2) = '*';
  *(line.end() - 1) = '\0';  // Windows does not terminate at overlength
}

template <class line_type>
inline bool counter_update_and_space_left(line_type& line, int written, int& write_at) {
  if (written > static_cast<int>(line.size() - write_at)) {
    indicate_overlength(line);
    return false;
  }
  write_at += written;
  return true;
}

template <class line_type>
inline bool add_date_string(line_type& line, int& write_at) {
  using namespace std;
  using namespace std::chrono;

  if (static_cast<int>(line.size()) > write_at) {
    auto now = system_clock::now();
    std::time_t tmp = system_clock::to_time_t(now);
    struct tm now_c;
#ifdef _WIN32
    localtime_s(&now_c, &tmp);
#else
    localtime_r(&tmp, &now_c);
#endif
    auto tp = now.time_since_epoch();
    tp -= duration_cast<seconds>(tp);

    auto written = xsnprintf(
        line.data() + write_at, line.size() - write_at, "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
        now_c.tm_year + 1900, now_c.tm_mon + 1, now_c.tm_mday, now_c.tm_hour, now_c.tm_min,
        now_c.tm_sec, static_cast<long long>(duration_cast<milliseconds>(tp).count()));

    return counter_update_and_space_left(line, written, write_at);
  }
  indicate_overlength(line);
  return false;
}

template <class line_type>
inline bool add_thread_id_string(line_type& line, int& write_at) {
  using namespace std;
  if (static_cast<int>(line.size()) > write_at) {
    ostringstream thread_id;
    thread_id << this_thread::get_id();
    auto written =
        xsnprintf(line.data() + write_at, line.size() - write_at, " %s", thread_id.str().c_str());

    return counter_update_and_space_left(line, written, write_at);
  }
  indicate_overlength(line);
  return false;
}

template <class line_type>
inline bool add_component_category(line_type& line, int& write_at, const char* Component,
                                   const char* Category) {
  using namespace std;
  if (static_cast<int>(line.size()) > write_at) {
    auto written =
        xsnprintf(line.data() + write_at, line.size() - write_at, " %s:%s", Component, Category);
    return counter_update_and_space_left(line, written, write_at);
  }
  indicate_overlength(line);
  return false;
}

inline const char* basename(const char* path) {
  const char* file_only = path + strlen(path) - 1;
  while (file_only != path && *file_only != '\\' && *file_only != '/') {
    file_only--;
  }
  return file_only;
}

template <class line_type = LineType>
inline void log_function(const char* Component, const char* Category, const char* format, ...) {
  using namespace std;
  line_type line;
  int write_at = 0;

  bool success = add_date_string(line, write_at) && add_thread_id_string(line, write_at) &&
                 add_component_category(line, write_at, Component, Category);

  if (success && (write_at + 1 < static_cast<int>(line.size()))) {
    line[write_at++] = ' ';

    va_list args;
    va_start(args, format);
    auto written = vsnprintf(line.data() + write_at, line.size() - write_at, format, args);
    va_end(args);

    counter_update_and_space_left(line, written, write_at);
  } else {
    indicate_overlength(line);
  }

  print_log(line, Component, Category);
}

template <class line_type = LineType>
inline std::string format_to_string(const char* format, ...) {
  line_type line;
  va_list args;
  va_start(args, format);
  auto written = vsnprintf(line.data(), line.size(), format, args);
  va_end(args);

  if (written >= static_cast<int>(line.size())) {
    indicate_overlength(line);
  }
  return std::string(line.data());
}

template <class line_type = LineType>
inline bool log_assert(const char* component, const char* predicate, const char* filename,
                       int lineNumber, const char* format, ...) {
  using namespace std;
  line_type line;
  int write_at = 0;

  bool success = add_date_string(line, write_at) && add_thread_id_string(line, write_at) &&
                 add_component_category(line, write_at, component, "E");

  if (success && (write_at + 1 < static_cast<int>(line.size()))) {
    line[write_at++] = ' ';

    auto written = xsnprintf(line.data() + write_at, line.size() - write_at,
                             "Assert failed: '%s' in file %s, line %d ", predicate,
                             basename(filename), lineNumber);

    if (counter_update_and_space_left(line, written, write_at)) {
      va_list args;
      va_start(args, format);
      written = vsnprintf(line.data() + write_at, line.size() - write_at, format, args);
      va_end(args);

      if (written >= static_cast<int>(line.size()) - write_at) {
        indicate_overlength(line);
      }
    }
  }
  print_log(line, component, "E");
  return true;
}

/*!
 * @brief Scope logger implementation
 * \ingroup Logging
 */
struct ScopeLogger {
 public:
  /*!
   * @brief Create scope logger
   *
   * @param component Name of the code project/section the scope is logged in (e.g. library name).
   * @param function Name of the function/section the scope is executed in (e.g. __FUNCTION__).
   * @param format Message to log and format (see format from printf) according to a parameter list.
   * @param args Variable arguments list used in format.
   */
  ScopeLogger(const char* component, const char* function, const char* format, va_list args);
  virtual ~ScopeLogger();

 protected:
  bool add_frame(LineType& line, int& write_at, const char* format, va_list args) const;
  bool add_brackets(LineType& line, int& write_at) const;
  bool add_closing(LineType& line, int& write_at) const;
  void print_entry(const char* format, va_list args) const;
  virtual void print_exit() const;

 protected:
  const char* comp;
  const char* func;
  bool skip_exit_log = false;
};

/*!
 * @brief Scope logger implementation with return capture
 * \ingroup Logging
 */
template <class rtype>
struct ScopeLoggerRet : ScopeLogger {
  /*!
   * @brief Create scope logger
   *
   * @param component Name of the code project/section the scope is logged in (e.g. library name).
   * @param ret Function return value of type rtype that should be captured in the log.
   * @param function Name of the function/section the scope is executed in (e.g. __FUNCTION__).
   * @param format Message to log and format (see format from printf) according to a parameter list.
   * @param args Variable arguments list used in format.
   */
  ScopeLoggerRet(const char* component, rtype& ret, const char* function, const char* format,
                 va_list args)
      : ScopeLogger(component, function, format, args), retval(ret) {}

  ~ScopeLoggerRet() {
    print_exit();
    skip_exit_log = true;
  }

  ScopeLoggerRet& operator=(const ScopeLoggerRet&) = delete;

 private:
  template <class line_type = LineType>
  void print_exit() const {
    line_type line;

    int write_at = 0;
    add_date_string(line, write_at) && add_thread_id_string(line, write_at) &&
        add_return_value_closing(line, write_at);

    print_log(line, comp, "I");
  }

  template <class line_type>
  bool add_return_value_closing(line_type& line, int& write_at) const {
    if (write_at + 1 < static_cast<int>(line.size())) {
      int written;
      line[write_at++] = ' ';
      std::ostringstream retval_str;
      retval_str << retval;
      written = xsnprintf(line.data() + write_at, line.size() - write_at, "%s -> (%s) }", func,
                          retval_str.str().c_str());

      return counter_update_and_space_left(line, written, write_at);
    }
    return false;
  }

 private:
  rtype& retval;
};

/*! \addtogroup Logging Custom logger implementation
 *  @{
 *  Scope logger creation helpers
 */

/*!
 * @ref Create scope logger
 *
 * @param component Name of the code project/section the scope is logged in (e.g. library name).
 * @param function Name of the function/section the scope is executed in (e.g. __FUNCTION__).
 * @param fmt Message to log and format (see format from printf) according to a parameter list.
 * @param ... Variable arguments list used in format.
 */
inline ScopeLogger make_ScopeLogger(const char* component, const char* function, const char* fmt,
                                    ...) {
  va_list args;
  va_start(args, fmt);
  ScopeLogger result = ScopeLogger(component, function, fmt, args);
  va_end(args);
  return result;
}

/*!
 * @brief Create scope logger
 *
 * @param component Name of the code project/section the scope is logged in (e.g. library name).
 * @param ret Function return value of type retval that should be captured in the log.
 * @param function Name of the function/section the scope is executed in (e.g. __FUNCTION__).
 * @param fmt Message to log and format (see format from printf) according to a parameter list.
 * @param ... Variable arguments list used in format.
 */
template <typename retval>
ScopeLoggerRet<retval> make_ScopeLoggerRet(const char* component, retval& ret, const char* function,
                                           const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  ScopeLoggerRet<retval> result = ScopeLoggerRet<retval>(component, ret, function, fmt, args);
  va_end(args);
  return result;
}
/**@}*/
}  // namespace impl
}  // namespace ilo

#ifdef _WIN32
#pragma warning(pop)
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#undef ILO_DEPRECATED
