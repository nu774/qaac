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
 * @file gtest_helper.h
 * @brief GTest helper functions
 */

#pragma once

// System includes
#include <functional>
#include <string>
#include <algorithm>

#ifndef LOG_COMPONENT
#error define LOG_COMPONENT before including this header
#endif

// Internal includes
#include "ilo/logging.h"
#include "ilo/string_utils.h"

namespace {
/*!
 * @brief Sanitize string tokens for later use in a filename. Special characters will be replaced in
 * this process.
 *
 * Note: Useful for generating log file names based on test frameworks like gtest
 */
std::string sanitize(std::string token, const bool replaceSlash = false) {
  token = token.substr(0, token.find(":"));
  std::transform(token.begin(), token.end(), token.begin(), [&replaceSlash](char c) {
    switch (c) {
      case '/':
      case '\\':
        if (replaceSlash) {
          return '_';
        }
        return c;
      case '*':
      case '?':
        return '_';
      default:
        return c;
    }
  });

  return token;
}

/*!
 * @brief Generate a log filename from gtest based argument lists
 *
 * @param defaultName Should have the following format [projectName]_[testType]_test.log
 * @return This method return a test specific filename if the gtest_filter is present. Otherwise the
 * defaultName is used
 */
std::string makeLogFilename(int argc, char** argv, std::string defaultName) {
  std::string newName = defaultName;
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      std::string parameter = std::string(argv[i]);

      std::deque<std::string> tokens;
      bool replaceSlash = false;

      if (parameter.find("--gtest_filter=") == 0) {
        tokens = ilo::tokenize(parameter, '=');
        replaceSlash = true;
      } else if (parameter.find("--input=") == 0) {
        tokens = ilo::tokenize(parameter, '/');
      }

      if (!tokens.empty()) {
        auto pos = defaultName.find("_");
        std::size_t pos2 = defaultName.find("_", pos + 1);
        newName.replace(pos + 1, pos2 - pos - 1, sanitize(tokens.back(), replaceSlash));
      }
    }
  }

  return newName;
}

//! Extract location next to binary to place logfiles
std::string extractLogFileLocation(int argc, char** argv) {
  if (argc < 1) {
    throw std::runtime_error("Not enough arguments given.");
  }

  std::string arg(argv[0]);

  size_t endPos;

#if defined(_WIN32) || defined(WIN32)
  endPos = arg.find_last_of("\\/");
#else
  endPos = arg.find_last_of("/");
#endif

  if (endPos == std::string::npos) {
    return std::string();
  }

  return arg.substr(0, endPos + 1);
}
}  // namespace

namespace ilo {
/*! \defgroup GTestHelper GTest helper
 *  @{
 *  Custom implementation of the GTest runTests function with logging. Can be used in custom
 * gtest_main functions.
 */

//! Test function handed over by custom gtest main
typedef std::function<int(int*, char**)> coreTestFunction;

/*!
 * @brief GTest test runner implementation to be used in custom gtest main.
 *
 * Takes care of executing all tests and logging details into a file.
 * When being used with ctest, each test will generate a separate log file, based on
 * the name of the test given in via gtest filters.
 */
int runTests(int* argc, char** argv, std::string defaultFilename, coreTestFunction coreTest) {
  auto logFilePath =
      extractLogFileLocation(*argc, argv) + makeLogFilename(*argc, argv, defaultFilename);
  LOG_REDIRECT_TO_FILE_APPEND(logFilePath);
  ILO_LOG_INFO("Starting test binary %s with the following arguments", argv[0]);
  for (auto i = 1; i < *argc; ++i) {
    ILO_LOG_INFO("%s", argv[i]);
  }

  auto result = coreTest(argc, argv);

  ILO_LOG_INFO("End of test binary %s with the following arguments", argv[0]);
  for (auto i = 1; i < *argc; ++i) {
    ILO_LOG_INFO("%s", argv[i]);
  }
  ILO_LOG_INFO("");

  return result;
}

/**@}*/
}  // namespace ilo
