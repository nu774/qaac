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

/*
 * Project: MPEG-4 ISO Base Media File Format (ISO BMFF) library
 * Content: extractor classes to enrich track based info objects
 */

#pragma once

#include <map>
#include <type_traits>

#include "mmtisobmff/reader/reader.h"
#include "tree/boxtree.h"

#include "box/elstbox.h"
#include "box/mdhdbox.h"
#include "box/sampleentry.h"
#include "box/hdlrbox.h"
#include "box/tkhdbox.h"
#include "box/unknownbox.h"

namespace mmt {
namespace isobmff {
struct CReaderMaps {
  CReaderMaps() {
    handlerToType.insert({{(ilo::toFcc("soun")), TrackType::audio},
                          {(ilo::toFcc("vide")), TrackType::video},
                          {(ilo::toFcc("hint")), TrackType::hint}});

    codingNameToCodec.insert({{ilo::toFcc("mp4a"), Codec::mp4a},
                              {ilo::toFcc("mha1"), Codec::mpegh_mha},
                              {ilo::toFcc("mha2"), Codec::mpegh_mha},
                              {ilo::toFcc("mhm1"), Codec::mpegh_mhm},
                              {ilo::toFcc("mhm2"), Codec::mpegh_mhm},
                              {ilo::toFcc("hvc1"), Codec::hevc},
                              {ilo::toFcc("hev1"), Codec::hevc},
                              {ilo::toFcc("avc1"), Codec::avc},
                              {ilo::toFcc("avc3"), Codec::avc},
                              {ilo::toFcc("jxsm"), Codec::jxs},
                              {ilo::toFcc("vvc1"), Codec::vvc},
                              {ilo::toFcc("vvi1"), Codec::vvc}});
  }

  std::map<ilo::Fourcc, TrackType> handlerToType;
  std::map<ilo::Fourcc, Codec> codingNameToCodec;

  static CReaderMaps inst;
};

struct CEditListExtractor {
  static void store(const BoxElement& t, CTrackInfo& ti) {
    auto elst = findFirstBoxWithFourccAndType<box::CEditListBox>(t, ilo::toFcc("elst"));
    if (elst == nullptr) {
      return;
    }

    for (const auto& edit : elst->entries()) {
      SEdit sedit;
      sedit.segmentDuration = edit.segmentDuration;
      sedit.mediaTime = edit.mediaTime;
      sedit.mediaRate =
          edit.mediaRateInteger +
          edit.mediaRateFraction / static_cast<float>(std::numeric_limits<int16_t>::max());
      ti.editList.push_back(sedit);

      if (edit.mediaRateFraction != 0) {
        ILO_LOG_WARNING("Invalid mediaRateFraction of %d in edit list found");
      }
    }
  }
};

struct CUserDataExtractor {
  template <class T>
  static void store(const BoxElement& t, T& ti) {
    auto isTrackInfo = std::is_same<T, CTrackInfo>::value;
    auto isMovieInfo = std::is_same<T, CMovieInfo>::value;

    ILO_ASSERT(isMovieInfo || isTrackInfo, "user data can only be in movie info or track info");

    auto udtaElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(t, ilo::toFcc("udta"), 1);
    if (udtaElements.size() == 0) {
      return;
    }

    ILO_ASSERT(udtaElements.size() == 1,
               "Multiple udta containers on the same level are forbidden");

    const auto& udataTree = udtaElements[0].get();
    for (size_t nodeNr = 0; nodeNr < udataTree.childCount(); ++nodeNr) {
      const auto& currentNode = udataTree[nodeNr];
      auto size = currentNode.item->size();
      ilo::ByteBuffer data(static_cast<size_t>(size));
      ilo::ByteBuffer::iterator dataIter = data.begin();
      serializeTree(currentNode, data, dataIter);
      ti.userData.push_back(data);
    }
  }
};

struct CMediaTimeInfoExtractor {
  static void store(const BoxElement& t, CTrackInfo& ti) {
    auto mdhd = findFirstBoxWithPathAndType<box::CMediaHeaderBox>(t, "mdia/mdhd");
    ILO_ASSERT(mdhd.get() != nullptr, "mdhd not found");
    ti.timescale = mdhd->timescale();
    ti.duration = mdhd->duration();
    ti.language = mdhd->language();
  }
};

struct CTrackIdExtractor {
  static void store(const BoxElement& t, CTrackInfo& ti) {
    auto tkhd = findFirstBoxWithFourccAndType<box::CTrackHeaderBox>(t, ilo::toFcc("tkhd"));
    ILO_ASSERT(tkhd.get() != nullptr, "tkhd box not found");
    ti.trackId = tkhd->trackID();
  }
};

struct CCodingNameExtractor {
  static void store(const BoxElement& t, CTrackInfo& ti) {
    auto sampleentry = findFirstBoxWithType<box::CSampleEntry>(t);
    if (sampleentry) {
      ti.codingName = sampleentry->type();
    }

    if (CReaderMaps::inst.codingNameToCodec.find(ti.codingName) !=
        CReaderMaps::inst.codingNameToCodec.end()) {
      ti.codec = CReaderMaps::inst.codingNameToCodec.at(ti.codingName);
    }
  }
};

struct CHandlerExtractor {
  static void store(const BoxElement& t, CTrackInfo& ti) {
    auto hdlrbox = findFirstBoxWithPathAndType<box::CHandlerReferenceBox>(t, "mdia/hdlr");
    ILO_ASSERT(hdlrbox.get() != nullptr, "handler box not found");
    ti.handler = hdlrbox->handlerType();

    if (CReaderMaps::inst.handlerToType.find(ti.handler) != CReaderMaps::inst.handlerToType.end()) {
      ti.type = CReaderMaps::inst.handlerToType.at(ti.handler);
    }
  }
};
}  // namespace isobmff
}  // namespace mmt
