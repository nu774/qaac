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
 * Content: classes to extract meta samples from track reader
 */

#pragma once

// System includes
#include <cstdint>
#include <vector>
#include <memory>
#include <map>

// Internal includes
#include "common/tracksampleinfo.h"
#include "tree/boxtree.h"

#include "box/containerbox.h"
#include "box/tfhdbox.h"
#include "box/tfdtbox.h"
#include "box/trunbox.h"
#include "box/mfhdbox.h"
#include "box/trexbox.h"
#include "box/mdhdbox.h"
#include "box/sgpdbox.h"
#include "box/sbgpbox.h"
#include "box/stszbox.h"
#include "box/stz2box.h"

namespace mmt {
namespace isobmff {
struct CSampleGroupInfo {
  CSampleGroupInfo(const ilo::Fourcc& sGroupingType, const uint32_t& sGroupDescIndex)
      : groupingType(sGroupingType), groupDescIndex(sGroupDescIndex) {}

  ilo::Fourcc groupingType = ilo::toFcc("0000");
  uint32_t groupDescIndex = 0;
};

struct SDefaultConfig {
  uint32_t startIndex = 0;
  size_t nrOfSamples = 0;
  std::shared_ptr<box::CSampleGroupDescriptionBox> sgpd = nullptr;
};

using SampleToSampleGroupInfoMap = std::map<size_t, std::vector<CSampleGroupInfo>>;
using GroupingTypeToVectorIndexMap = std::map<ilo::Fourcc, size_t>;

struct ISampleExtractor {
  virtual ~ISampleExtractor() {}
  virtual std::shared_ptr<TrackIdToTrackSampleInfo> trackIdToTrackSampleInfo() const = 0;

 protected:
  void createSampleToSampleGroupInfoMap(const size_t nrOfSamples);
  void fillDefaultSampleGroupInfo(SampleToSampleGroupInfoMap& indexMap,
                                  const SDefaultConfig& config) const;
  void setSampleSampleGroupInfo(const std::vector<CSampleGroupInfo>& sampleGroupInfos,
                                CMetaSample& metaSample);

  std::shared_ptr<TrackIdToTrackSampleInfo> m_sampleInfoTable = nullptr;
  std::vector<std::shared_ptr<box::CSampleGroupDescriptionBox>> m_currentSgpdBoxes;
  std::vector<std::shared_ptr<box::CSampleToGroupBox>> m_currentSbgpBoxes;
  SampleToSampleGroupInfoMap m_SampleGroupSampleMap;
  GroupingTypeToVectorIndexMap m_groupingTypeMap;
};

struct CSampleExtractorFactory {
  static std::unique_ptr<ISampleExtractor> create(const BoxTree& tree);
};

struct CFragmentedSampleExtractor : public ISampleExtractor {
  CFragmentedSampleExtractor(const BoxTree& tree);

  std::shared_ptr<TrackIdToTrackSampleInfo> trackIdToTrackSampleInfo() const;

 private:
  uint64_t calculateDataOffset(uint64_t totalDataOffset);

  void fillSampleInfoTable(uint64_t dataOffset);

  void setSampleSize(const box::CTrunEntry& trunEntry, CMetaSample& metaSample);
  void setSampleDuration(const box::CTrunEntry& trunEntry, CMetaSample& metaSample);
  void setSampleCtsOffset(const box::CTrunEntry& trunEntry, CMetaSample& metaSample);
  void setSampleOffset(uint64_t dataOffset, uint64_t currentSampleOffset, CMetaSample& metaSample);
  void setSampleFragmentNumber(CMetaSample& metaSample);
  void setSyncSampleFlag(const size_t index, const box::CTrunEntry& trunEntry,
                         CMetaSample& metaSample);
  void setTimeScale(CMetaSample& metaSample);
  void setSampleSampleGroupInfoFrag(const size_t& metaSampleIndex, CMetaSample& metaSample);

  std::shared_ptr<box::CTrackRunBox> m_currentTrunBox;
  std::shared_ptr<box::CTrackFragmentHeaderBox> m_currentTfhdBox;
  std::shared_ptr<box::CMovieFragmentHeaderBox> m_currentMfhdBox;
  std::shared_ptr<box::CTrackExtendsBox> m_currentTrexBox;
  std::shared_ptr<box::CMediaHeaderBox> m_currentMdhdBox;
  std::shared_ptr<box::CTrackFragmentMDTBox> m_currentTfdtBox;
};

struct CRegularSampleExtractor : public ISampleExtractor {
  CRegularSampleExtractor(const BoxTree& tree);

  std::shared_ptr<TrackIdToTrackSampleInfo> trackIdToTrackSampleInfo() const;

 private:
  void setSampleSizes(const uint32_t& trackId, const BoxElement& node);
  void setSampleDurations(const uint32_t& trackId, const BoxElement& node);
  void setSampleOffsets(const uint32_t& trackId, const BoxElement& node);
  void setSampleCtsOffsets(const uint32_t& trackId, const BoxElement& node);
  void setSyncSampleFlag(const uint32_t& trackId, const BoxElement& node);
  void setTimeScale(const uint32_t& trackId, const BoxElement& node);
  void setSampleSampleGroupInfoRegular(const uint32_t& trackId);

  uint32_t m_sampleCount;
};
}  // namespace isobmff
}  // namespace mmt
