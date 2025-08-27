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
 * Content: mmtisobmff reader class(es)
 */

// System includes

// External includes

// Internal includes
#include "mmtisobmff/reader/reader.h"
#include "tree/boxtree.h"
#include "box/mvhdbox.h"
#include "box/ftypbox.h"
#include "box/containerbox.h"
#include "pimpl.h"
#include "reader/readerinfo.h"

namespace mmt {
namespace isobmff {

bool checkIfTrackIdHasSampleData(CIsobmffReader::Pimpl& p, uint32_t trackId) {
  bool hasSampleData = (p.trackIdToTrackSampleInfo().count(trackId) >= 1);
  if (!hasSampleData) {
    ILO_LOG_WARNING("Track with id %d does not have accessible sample data", trackId);
  }
  return hasSampleData;
}

size_t maxSampleSize(CIsobmffReader::Pimpl& p, uint32_t trackId) {
  if (!checkIfTrackIdHasSampleData(p, trackId)) {
    return 0;
  }
  size_t maxSize = 0;
  if (verboseLogLevel) {
    ILO_LOG_SCOPE_RET(maxSize, "trackId: %u", trackId);
  }
  for (const auto& sz : p.trackIdToTrackSampleInfo().at(trackId)) {
    maxSize = std::max(maxSize, static_cast<size_t>(sz.size));
  }
  return maxSize;
}

uint64_t sampleDurationSum(CIsobmffReader::Pimpl& p, uint32_t trackId) {
  if (!checkIfTrackIdHasSampleData(p, trackId)) {
    return 0;
  }
  uint64_t duration = 0;
  for (const auto& sample : p.trackIdToTrackSampleInfo().at(trackId)) {
    duration += sample.duration;
  }
  return duration;
}

size_t totalSampleCount(CIsobmffReader::Pimpl& p, uint32_t trackId) {
  if (!checkIfTrackIdHasSampleData(p, trackId)) {
    return 0;
  }
  return p.trackIdToTrackSampleInfo().at(trackId).size();
}

CTrackInfo createTrackInfoFromTrack(CIsobmffReader::Pimpl& p, const BoxElement& t) {
  CTrackInfo ti;

  CHandlerExtractor::store(t, ti);
  CCodingNameExtractor::store(t, ti);
  CTrackIdExtractor::store(t, ti);
  CMediaTimeInfoExtractor::store(t, ti);
  if (ti.duration == 0) {
    if (verboseLogLevel) {
      ILO_LOG_INFO("duration is zero, summing up sample durations");
    }
    ti.duration = sampleDurationSum(p, ti.trackId);
  }
  CEditListExtractor::store(t, ti);
  ti.maxSampleSize = maxSampleSize(p, ti.trackId);
  ti.sampleCount = totalSampleCount(p, ti.trackId);

  CUserDataExtractor::store<CTrackInfo>(t, ti);

  return ti;
}

CIsobmffReader::CIsobmffReader(std::unique_ptr<IIsobmffInput>&& input) {
  setupServicesOnce();
  p = std::make_shared<Pimpl>(std::move(input));
}

CMovieInfo CIsobmffReader::movieInfo() const {
  CMovieInfo result;
  auto mvhd = findFirstBoxWithFourccAndType<box::CMovieHeaderBox>(p->tree(), ilo::toFcc("mvhd"));
  ILO_ASSERT(mvhd != nullptr, "no mvhd box found");
  result.creationTime = mvhd->creationTime();
  result.modificationTime = mvhd->modificationTime();
  result.timeScale = static_cast<uint32_t>(mvhd->timescale());
  result.duration = mvhd->duration();

  auto ftype = findFirstBoxWithFourccAndType<box::CFileTypeBox>(p->tree(), ilo::toFcc("ftyp"));
  ILO_ASSERT(ftype != nullptr, "no ftyp box found");
  result.majorBrand = ftype->majorBrand();
  result.compatibleBrands = ftype->compatibleBrands();

  auto moov =
      findFirstElementWithFourccAndBoxType<box::CContainerBox>(p->tree(), ilo::toFcc("moov"));

  CUserDataExtractor::store<CMovieInfo>(moov, result);

  return result;
}

CTrackInfoVec CIsobmffReader::trackInfos() const {
  CTrackInfoVec result;
  auto traks =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(p->tree(), ilo::toFcc("trak"));
  uint32_t index = 0;
  for (const auto& t : traks) {
    auto ti = createTrackInfoFromTrack(*p, t.get());
    ti.trackIndex = index++;
    result.push_back(ti);
  }
  return result;
}

size_t CIsobmffReader::trackCount() const {
  return static_cast<size_t>(
      findAllBoxesWithFourccAndType<box::IBox>(p->tree(), ilo::toFcc("trak")).size());
}
}  // namespace isobmff
}  // namespace mmt
