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
 * @file logging.h
 * @brief Platform abstracted logger
 */

#pragma once

// System includes
#include <chrono>
#include <memory>
#include <stdarg.h>

// Internal includes
#include "ilo/version.h"
#include "ilo/logging_backend.h"

/*! \addtogroup Logging Custom logger implementation
 *  @{
 *  Logger macro definitions making use of CLogger class
 */

#if defined(WIN32) || defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#define ILO_FUNCTION __FUNCTION__
#ifndef LOG_COMPONENT
#error("LOG_COMPONENT not defined - please define before including logging.h (hint: use a common header in your component)")
#endif
#else
#define ILO_FUNCTION __PRETTY_FUNCTION__
#ifndef LOG_COMPONENT
#error \
    "LOG_COMPONENT not defined - please define before including logging.h (hint: use a common header in your component)"
#endif
#endif

//! Macro to log errors - use it like printf
#define ILO_LOG_ERROR(...) ilo::impl::log_function(LOG_COMPONENT, "E", __VA_ARGS__)
//! Macro to log warnings - use it like printf
#define ILO_LOG_WARNING(...) ilo::impl::log_function(LOG_COMPONENT, "W", __VA_ARGS__)
//! Macro to log informative stuff - use it like printf
#define ILO_LOG_INFO(...) ilo::impl::log_function(LOG_COMPONENT, "I", __VA_ARGS__)

//! Macro to log stuff every milli seconds - use it like printf
#define ILO_LOG_EVERY(ms, ...)                                                            \
  {                                                                                       \
    using namespace std::chrono;                                                          \
    static time_point<high_resolution_clock> last;                                        \
    if (duration_cast<milliseconds>(high_resolution_clock::now() - last).count() >= ms) { \
      ilo::impl::log_function(LOG_COMPONENT, "R", __VA_ARGS__);                           \
      last = high_resolution_clock::now();                                                \
    }                                                                                     \
  }

#define ILO_XCONCAT(x, y) x##y
#define ILO_CONCAT(x, y) ILO_XCONCAT(x, y)
#ifdef __COUNTER__
#define ILO_UNIQVARNAME(name) ILO_CONCAT(name, __COUNTER__)
#else
#define ILO_UNIQVARNAME(name) ILO_CONCAT(name, __LINE__)
#endif

//! Macro to log a scope, e.g. a function call with a return value (ret)
#define ILO_LOG_SCOPE_RET(ret, ...) \
  auto ILO_UNIQVARNAME(log_scope) = \
      ilo::impl::make_ScopeLoggerRet(LOG_COMPONENT, ret, ILO_FUNCTION, __VA_ARGS__)
//! Macro to log a scope
#define ILO_LOG_SCOPE(...)          \
  auto ILO_UNIQVARNAME(log_scope) = \
      ilo::impl::make_ScopeLogger(LOG_COMPONENT, ILO_FUNCTION, __VA_ARGS__)

//! Writes a log and throws a runtime_error
#define ILO_FAIL(...)                                                                 \
  (ilo::impl::log_assert(LOG_COMPONENT, "ILO_FAIL", __FILE__, __LINE__, __VA_ARGS__), \
   throw std::runtime_error(ilo::impl::format_to_string(__VA_ARGS__)))

//! Writes a log and throws the exception as given by parameter exceptionType
#define ILO_FAIL_WITH(exceptionType, ...)                                             \
  (ilo::impl::log_assert(LOG_COMPONENT, "ILO_FAIL", __FILE__, __LINE__, __VA_ARGS__), \
   throw exceptionType(ilo::impl::format_to_string(__VA_ARGS__)))

//! ASSERT checks the condition, writes a log and throws a runtime_error if condition failed
#define ILO_ASSERT(x, ...)                                                              \
  (!(x) && ilo::impl::log_assert(LOG_COMPONENT, #x, __FILE__, __LINE__, __VA_ARGS__) && \
   (throw std::runtime_error(ilo::impl::format_to_string(__VA_ARGS__)), 1))

//! ASSERT checks the condition, writes a log and throws the exception as given by parameter
//! exceptionType if condition failed
#define ILO_ASSERT_WITH(x, exceptionType, ...)                                          \
  (!(x) && ilo::impl::log_assert(LOG_COMPONENT, #x, __FILE__, __LINE__, __VA_ARGS__) && \
   (throw exceptionType(ilo::impl::format_to_string(__VA_ARGS__)), 1))

//! Redirect logging to file - file will be truncated
#define LOG_REDIRECT_TO_FILE(filename) ilo::impl::CLogger::instance().redirect_to_file(filename)
//! Redirect logging to file - file will be appended
#define LOG_REDIRECT_TO_FILE_APPEND(filename) \
  ilo::impl::CLogger::instance().redirect_to_file(filename, true)
//! Redirect logging to system log - logs will be written to system specific logging systems
#define LOG_REDIRECT_TO_SYSTEM_LOG() ilo::impl::CLogger::instance().redirect_to_system_log()
//! Turn off logging redirection (i.e. log to stdout)
#define LOG_REDIRECT_TO_CONSOLE() ilo::impl::CLogger::instance().redirect_to_console()
//! Turn off logging - can be turned on again using any of the above mentioned redirectors
#define LOG_DISABLE_LOGGING() ilo::impl::CLogger::instance().disable_logging()

/**@}*/

#if defined(WIN32) || defined(_WIN32)
#pragma warning(pop)
#endif
