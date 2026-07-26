/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2020 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
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
 * @file bittool_utils.h
 * @brief Utilities used in bit reader and writer
 * \defgroup bittools Tools for bit-wise buffer reading and writing
 * @{
 * Collection of tools for bit-wise buffer reading and writing
 */

#pragma once

// System includes
#include <stdexcept>
#include <string>

namespace ilo {
//! Position type used to indicate relative to what position an operation should be executed.
enum class EPosType {
  //! Use beginning as a starting point for operation
  begin = 0,
  //! Use ending as a starting point for operation
  end,
  //! Use the current position as a starting point for operation
  cur
};

//! Custom exception to indicate an issue with a read operation
struct ReadException : public std::runtime_error {
  //! Create a custom read exception with user message
  ReadException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with a write operation
struct WriteException : public std::runtime_error {
  //! Create a custom write exception with user message
  WriteException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with an append operation
struct AppendException : public std::runtime_error {
  //! Create a custom append exception with user message
  AppendException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with an insert operation
struct InsertException : public std::runtime_error {
  //! Create a custom insert exception with user message
  InsertException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with a seek operation
struct SeekException : public std::runtime_error {
  //! Create a custom seek exception with user message
  SeekException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with an erase operation
struct EraseException : public std::runtime_error {
  //! Create a custom erase exception with user message
  EraseException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

//! Custom exception to indicate an issue with a reserve operation
struct ReserveException : public std::runtime_error {
  //! Create a custom reserve exception with user message
  ReserveException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};
}  // namespace ilo
