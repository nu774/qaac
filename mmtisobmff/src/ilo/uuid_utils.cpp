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

// System includes
#include <sstream>

// Internal includes
#include "ilo_logging.h"
#include "ilo/uuid_utils.h"

namespace ilo {
std::mutex SUuid::s_mutex;
std::unique_ptr<std::mt19937> SUuid::s_rnd;

SUuid SUuid::s_createUuid() {
  std::lock_guard<std::mutex> locker(s_mutex);
  if (!s_rnd) {
    auto const seed = std::random_device()();
    ILO_LOG_INFO("Obtained the following seed for the rng used to generate UUID:  %u", seed);

    s_rnd = ilo::make_unique<std::mt19937>(seed);
  }

  std::array<uint8_t, 16> uuid;
  for (int i = 0; i < 4; ++i) {
    uint32_t value = static_cast<uint32_t>((*s_rnd)());
    uuid[(i * 4) + 0] = static_cast<uint8_t>(value);
    uuid[(i * 4) + 1] = static_cast<uint8_t>(value >> 8);
    uuid[(i * 4) + 2] = static_cast<uint8_t>(value >> 16);
    uuid[(i * 4) + 3] = static_cast<uint8_t>(value >> 24);
  }

  return SUuid(uuid);
}

std::string SUuid::toString() const {
  std::stringstream sstream;
  size_t currentIndex = 0;

  for (; currentIndex < 16; ++currentIndex) {
    sstream << std::nouppercase << std::setfill('0') << std::hex << std::setw(2)
            << (uint32_t)m_uuid[currentIndex];
    if (currentIndex == 3 || currentIndex == 5 || currentIndex == 7 || currentIndex == 9) {
      sstream << "-";
    }
  }

  return sstream.str();
}

std::array<uint8_t, 16> SUuid::uuid() const {
  return m_uuid;
}

bool SUuid::operator==(const SUuid& other) const {
  return m_uuid == other.m_uuid;
}

SUuid::SUuid(const std::array<uint8_t, 16>& uuid_) : m_uuid(uuid_) {}
}  // namespace ilo
