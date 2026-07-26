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

#include "reader/sample_extractor.h"

#include "box/sttsbox.h"
#include "box/stszbox.h"
#include "box/stcobox.h"
#include "box/cttsbox.h"
#include "box/stcobox.h"
#include "box/co64box.h"
#include "box/stscbox.h"
#include "box/mfhdbox.h"
#include "box/stssbox.h"
#include "box/tkhdbox.h"

namespace mmt {
namespace isobmff {
// This constant is meant to mitigate erroneous big sample buffer allocation with defective MP4
// files. The value was chosen to be big enough to work in 99,999% of all cases and still prevent a
// decent out-of-memory error protection.
const uint32_t MAX_ALLOWED_SAMPLES_SIZE_IN_BYTE = 200000000;

void ISampleExtractor::fillDefaultSampleGroupInfo(SampleToSampleGroupInfoMap& infoMap,
                                                  const SDefaultConfig& config) const {
  for (uint32_t i = config.startIndex; i < config.nrOfSamples; ++i) {
    // Version >= 2 has a default descr. index. Otherwise signal "no group"
    if (config.sgpd->version() >= 2) {
      infoMap[i].push_back(CSampleGroupInfo{config.sgpd->groupingType(),
                                            config.sgpd->defaultSampleDescriptionIndex()});
    } else {
      infoMap[i].push_back(CSampleGroupInfo{config.sgpd->groupingType(), 0});
    }
  }
}

void ISampleExtractor::createSampleToSampleGroupInfoMap(const size_t nrOfSamples) {
  m_SampleGroupSampleMap.clear();
  m_groupingTypeMap.clear();
  std::vector<SDefaultConfig> defConfigs;

  for (const auto& sgpd : m_currentSgpdBoxes) {
    if (m_groupingTypeMap.size() != 0) {
      ILO_ASSERT(m_groupingTypeMap.find(sgpd->groupingType()) == m_groupingTypeMap.end(),
                 "Grouping types in sgpd box are not unique");
    }
    // Zero based sample group type index
    auto nrOfGroups = m_groupingTypeMap.size();
    m_groupingTypeMap[sgpd->groupingType()] = nrOfGroups;

    const auto sbgpIter = std::find_if(m_currentSbgpBoxes.begin(), m_currentSbgpBoxes.end(),
                                       [&](const std::shared_ptr<box::CSampleToGroupBox>& sbgp) {
                                         return sbgp->groupingType() == sgpd->groupingType();
                                       });

    SDefaultConfig config;
    config.nrOfSamples = nrOfSamples;
    config.sgpd = sgpd;

    if (sbgpIter != m_currentSbgpBoxes.end()) {
      // sbgp box found. Apply index from box.
      ILO_ASSERT(*sbgpIter != nullptr, "Sbgp Box was found but parsing returned a zero pointer");
      auto entries = (*sbgpIter)->sampleGroupEntries();

      uint32_t currSample = 0;
      for (const auto& entry : entries) {
        // Fail fast to not run out-of-memory before running into the final size check below
        ILO_ASSERT((m_SampleGroupSampleMap.size() + entry.sampleCount) <= nrOfSamples,
                   "Nr of samples from sample group is bigger than total nr of samples");
        for (uint32_t i = 0; i < entry.sampleCount; ++i) {
          m_SampleGroupSampleMap[currSample++].push_back(
              CSampleGroupInfo{(*sbgpIter)->groupingType(), entry.groupDescriptionIndex});
        }
      }
      // Handle left over samples with default index.
      config.startIndex = currSample;
      defConfigs.push_back(config);
    } else {
      // No sbgp box found for this sample group type. Use default index.
      config.startIndex = 0;
      defConfigs.push_back(config);
    }
  }

  // Handle samples with default values
  for (auto& config : defConfigs) {
    fillDefaultSampleGroupInfo(m_SampleGroupSampleMap, config);
  }

  ILO_ASSERT(m_SampleGroupSampleMap.size() <= nrOfSamples,
             "Nr of samples from sample group is bigger than total nr of samples");
}

void ISampleExtractor::setSampleSampleGroupInfo(
    const std::vector<CSampleGroupInfo>& sampleGroupInfos, CMetaSample& metaSample) {
  for (const auto& sgi : sampleGroupInfos) {
    // Desc. index of 0 or 0x10000 means "no sample group"
    if (sgi.groupDescIndex == 0 || sgi.groupDescIndex == 0x10000u) {
      continue;
    }

    ILO_ASSERT(metaSample.sampleGroupInfo.type == SampleGroupType::none,
               "Having multiple SampleGroups in one file is currently not supported");

    uint32_t groupDescIndexOffset = 1;
    if (sgi.groupDescIndex > 0x10000u) {
      groupDescIndexOffset = 0x10001u;
    }

    auto sgpdBox = m_currentSgpdBoxes.at(m_groupingTypeMap.at(sgi.groupingType));

    if (sgi.groupingType == ilo::toFcc("roll")) {
      metaSample.sampleGroupInfo.type = SampleGroupType::roll;
      auto sampleGroupEntries = sgpdBox->downCastSampleGroupEntries<CAudioRollRecoveryEntry>();
      auto sampleGroupEntry = sampleGroupEntries.at(sgi.groupDescIndex - groupDescIndexOffset);
      metaSample.sampleGroupInfo.rollDistance = sampleGroupEntry->rollDistance();
    } else if (sgi.groupingType == ilo::toFcc("prol")) {
      metaSample.sampleGroupInfo.type = SampleGroupType::prol;
      auto sampleGroupEntries = sgpdBox->downCastSampleGroupEntries<CAudioPreRollEntry>();
      auto sampleGroupEntry = sampleGroupEntries.at(sgi.groupDescIndex - groupDescIndexOffset);
      metaSample.sampleGroupInfo.rollDistance = sampleGroupEntry->rollDistance();
    } else if (sgi.groupingType == ilo::toFcc("sap ")) {
      metaSample.sampleGroupInfo.type = SampleGroupType::sap;
      auto sampleGroupEntries = sgpdBox->downCastSampleGroupEntries<CSAPEntry>();
      auto sampleGroupEntry = sampleGroupEntries.at(sgi.groupDescIndex - groupDescIndexOffset);
      metaSample.sampleGroupInfo.sapType = sampleGroupEntry->sapType();
    } else {
      // Do not throw here. Just log error and handle as no sample group
      ILO_LOG_ERROR("Unknown SampleGroupType found: %s", ilo::toString(sgi.groupingType).c_str());
      return;
    }
  }
}

CFragmentedSampleExtractor::CFragmentedSampleExtractor(const BoxTree& tree) {
  m_currentTfhdBox = nullptr;
  m_currentTrunBox = nullptr;
  m_currentTrexBox = nullptr;
  m_currentMdhdBox = nullptr;
  m_sampleInfoTable = std::make_shared<TrackIdToTrackSampleInfo>();

  uint64_t totalDataOffset = 0;
  std::vector<std::shared_ptr<box::CTrackExtendsBox>> trexBoxes;
  std::vector<std::shared_ptr<box::CTrackHeaderBox>> tkhdBoxes;
  std::vector<std::shared_ptr<box::CMediaHeaderBox>> mhdhBoxes;
  std::vector<std::shared_ptr<box::CSampleGroupDescriptionBox>> sgpdTrakBoxes;

  for (size_t i = 0; i < tree.childCount(); ++i) {
    if (tree[i].item->type() == ilo::toFcc("moov")) {
      trexBoxes = findAllBoxesWithFourccAndType<box::CTrackExtendsBox>(tree[i], ilo::toFcc("trex"));
      tkhdBoxes = findAllBoxesWithFourccAndType<box::CTrackHeaderBox>(tree[i], ilo::toFcc("tkhd"));
      mhdhBoxes = findAllBoxesWithFourccAndType<box::CMediaHeaderBox>(tree[i], ilo::toFcc("mdhd"));
      sgpdTrakBoxes = findAllBoxesWithFourccAndType<box::CSampleGroupDescriptionBox>(
          tree[i], ilo::toFcc("sgpd"));

      ILO_ASSERT(
          tkhdBoxes.size() == mhdhBoxes.size(),
          "Malformed tree found. There is at least one Trak with one TKHD or MDHD boxes missing");
    } else if (tree[i].item->type() == ilo::toFcc("moof")) {
      m_currentMfhdBox =
          findFirstBoxWithFourccAndType<box::CMovieFragmentHeaderBox>(tree[i], ilo::toFcc("mfhd"));
      ILO_ASSERT(m_currentMfhdBox != nullptr,
                 "MFHD box is required for fragmented mp4, but it was not found");

      auto trafs =
          findAllElementsWithFourccAndBoxType<box::CContainerBox>(tree[i], ilo::toFcc("traf"));

      for (const auto& traf : trafs) {
        m_currentTfhdBox = findFirstBoxWithFourccAndType<box::CTrackFragmentHeaderBox>(
            traf.get(), ilo::toFcc("tfhd"));
        ILO_ASSERT(m_currentTfhdBox != nullptr,
                   "TFHD box is required for fragmented mp4, but it was not found");

        // Hint, tfdt is optional. So no asserts here.
        m_currentTfdtBox = findFirstBoxWithFourccAndType<box::CTrackFragmentMDTBox>(
            traf.get(), ilo::toFcc("tfdt"));

        m_currentTrunBox =
            findFirstBoxWithFourccAndType<box::CTrackRunBox>(traf.get(), ilo::toFcc("trun"));
        ILO_ASSERT(m_currentTrunBox != nullptr,
                   "TRUN box is required for fragmented mp4, but it was not found");

        m_currentSgpdBoxes = findAllBoxesWithFourccAndType<box::CSampleGroupDescriptionBox>(
            tree[i], ilo::toFcc("sgpd"));
        m_currentSgpdBoxes.insert(m_currentSgpdBoxes.begin(), sgpdTrakBoxes.begin(),
                                  sgpdTrakBoxes.end());
        m_currentSbgpBoxes =
            findAllBoxesWithFourccAndType<box::CSampleToGroupBox>(tree[i], ilo::toFcc("sbgp"));
        ILO_ASSERT(
            m_currentSbgpBoxes.size() <= m_currentSgpdBoxes.size(),
            "Malformed tree found. At least one track has a sbgp box without having a sgpd box");

        for (auto& trex : trexBoxes) {
          if (trex->trackID() == m_currentTfhdBox->trackId()) {
            m_currentTrexBox = trex;
          }
        }

        for (size_t j = 0; j < tkhdBoxes.size(); ++j) {
          if (tkhdBoxes[j]->trackID() == m_currentTfhdBox->trackId()) {
            m_currentMdhdBox = mhdhBoxes[j];
          }
        }

        uint64_t dataOffset = calculateDataOffset(totalDataOffset);

        fillSampleInfoTable(dataOffset);
      }
    }
    totalDataOffset += tree[i].item->size();
  }
}

uint64_t CFragmentedSampleExtractor::calculateDataOffset(uint64_t totalDataOffset) {
  uint64_t dataOffset = 0;

  if (m_currentTfhdBox->baseDataOffsetPresent()) {
    dataOffset = m_currentTfhdBox->baseDataOffset();
  } else if (m_currentTfhdBox->defaultBaseIsMoof()) {
    dataOffset = totalDataOffset;
  } else {
    ILO_ASSERT(false, "Data offset mode not implemented");
  }
  return dataOffset;
}

void CFragmentedSampleExtractor::fillSampleInfoTable(uint64_t dataOffset) {
  auto trunEntries = m_currentTrunBox->trunEntries();

  uint64_t currentSampleOffset = 0;
  uint64_t currentDtsValue = 0;
  if (m_currentTfdtBox) {
    currentDtsValue = m_currentTfdtBox->baseMediaDecodeTime();
  } else {
    ILO_LOG_INFO("Fragment does not contain tfdt box (optional).");
  }

  createSampleToSampleGroupInfoMap(trunEntries.size());

  for (size_t index = 0; index < trunEntries.size(); index++) {
    CMetaSample metaSample;

    setSampleSize(trunEntries[index], metaSample);
    setSampleDuration(trunEntries[index], metaSample);
    setSampleCtsOffset(trunEntries[index], metaSample);
    setSampleOffset(dataOffset, currentSampleOffset, metaSample);
    setSampleFragmentNumber(metaSample);
    setSyncSampleFlag(index, trunEntries[index], metaSample);
    setTimeScale(metaSample);
    setSampleSampleGroupInfoFrag(index, metaSample);

    currentSampleOffset += metaSample.size;
    metaSample.dtsValue = currentDtsValue;
    currentDtsValue += metaSample.duration;
    (*m_sampleInfoTable)[m_currentTfhdBox->trackId()].push_back(metaSample);
  }
}

void CFragmentedSampleExtractor::setSampleSize(const box::CTrunEntry& trunEntry,
                                               CMetaSample& metaSample) {
  if (m_currentTrunBox->sampleSizePresent()) {
    metaSample.size = trunEntry.sampleSize();
  } else if (m_currentTfhdBox->defaultSampleSizePresent()) {
    metaSample.size = m_currentTfhdBox->defaultSampleSize();
  } else if (m_currentTrexBox) {
    metaSample.size = m_currentTrexBox->defaultSampleSize();
  } else {
    ILO_LOG_ERROR("Sample with size zero found");
  }

  ILO_ASSERT_WITH(metaSample.size <= MAX_ALLOWED_SAMPLES_SIZE_IN_BYTE, std::length_error,
                  "Sample size of %" PRId64 " found that exceeds maximum allow size of %d",
                  metaSample.size, MAX_ALLOWED_SAMPLES_SIZE_IN_BYTE);
}

void CFragmentedSampleExtractor::setSampleDuration(const box::CTrunEntry& trunEntry,
                                                   CMetaSample& metaSample) {
  if (m_currentTrunBox->sampleDurationPresent()) {
    metaSample.duration = trunEntry.sampleDuration();
  } else if (m_currentTfhdBox->defaultSampleDurationPresent()) {
    metaSample.duration = m_currentTfhdBox->defaultSampleDuration();
  } else if (m_currentTrexBox) {
    metaSample.duration = m_currentTrexBox->defaultSampleDuration();
  } else {
    ILO_LOG_ERROR("No sample duration present");
  }
}

void CFragmentedSampleExtractor::setSampleCtsOffset(const box::CTrunEntry& trunEntry,
                                                    CMetaSample& metaSample) {
  if (m_currentTrunBox->sampleCtsOffsetPresent()) {
    metaSample.ctsOffset = trunEntry.sampleCtsOffset();
  } else {
    metaSample.ctsOffset = 0;
  }
}

void CFragmentedSampleExtractor::setSampleOffset(uint64_t dataOffset, uint64_t currentSampleOffset,
                                                 CMetaSample& metaSample) {
  metaSample.offset = dataOffset + currentSampleOffset;

  if (m_currentTrunBox->dataOffsetPresent()) {
    metaSample.offset += m_currentTrunBox->dataOffset();
  }
}

void CFragmentedSampleExtractor::setSampleFragmentNumber(CMetaSample& metaSample) {
  metaSample.fragmentNumber = m_currentMfhdBox->sequenceNumber();
}

void CFragmentedSampleExtractor::setSyncSampleFlag(const size_t index,
                                                   const box::CTrunEntry& trunEntry,
                                                   CMetaSample& metaSample) {
  if (index == 0 && m_currentTrunBox->sampleFlagsPresent() &&
      m_currentTrunBox->firstSampleFlagsPresent()) {
    ILO_LOG_WARNING("Both sample and first sample flags found. Using first sample flags");
  }

  if (index == 0 && m_currentTrunBox->firstSampleFlagsPresent()) {
    metaSample.isSyncSample =
        ((m_currentTrunBox->firstSampleFlags() & 0x10000) >> 16 == 1) ? false : true;
  } else if (m_currentTrunBox->sampleFlagsPresent()) {
    metaSample.isSyncSample = ((trunEntry.sampleFlags() & 0x10000) >> 16 == 1) ? false : true;
  } else if (m_currentTfhdBox->defaultSampleFlagsPresent()) {
    metaSample.isSyncSample =
        ((m_currentTfhdBox->defaultSampleFlags() & 0x10000) >> 16 == 1) ? false : true;
  } else if (m_currentTrexBox) {
    metaSample.isSyncSample =
        ((m_currentTrexBox->defaultSampleFlags() & 0x10000) >> 16 == 1) ? false : true;
  } else {
    metaSample.isSyncSample = true;
  }
}

void CFragmentedSampleExtractor::setTimeScale(CMetaSample& metaSample) {
  if (m_currentMdhdBox) {
    metaSample.timeScale = m_currentMdhdBox->timescale();
  } else {
    ILO_LOG_ERROR(
        "No mdhd box found to get timescale from. "
        "Timescale value on sampleMetadata will be 0");
    metaSample.timeScale = 0;
  }
}

void CFragmentedSampleExtractor::setSampleSampleGroupInfoFrag(const size_t& metaSampleIndex,
                                                              CMetaSample& metaSample) {
  if (m_SampleGroupSampleMap.size() == 0) {
    // No sample group info. Leave default
    return;
  }
  setSampleSampleGroupInfo(m_SampleGroupSampleMap.at(metaSampleIndex), metaSample);
}

std::shared_ptr<TrackIdToTrackSampleInfo> CFragmentedSampleExtractor::trackIdToTrackSampleInfo()
    const {
  return m_sampleInfoTable;
}

CRegularSampleExtractor::CRegularSampleExtractor(const BoxTree& tree) {
  m_sampleInfoTable = std::make_shared<TrackIdToTrackSampleInfo>();

  auto moovNode = findFirstElementWithFourccAndBoxType<box::IBox>(tree, ilo::toFcc("moov"));
  auto traks =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovNode, ilo::toFcc("trak"));

  for (auto trak : traks) {
    m_currentSgpdBoxes =
        findAllBoxesWithFourccAndType<box::CSampleGroupDescriptionBox>(trak, ilo::toFcc("sgpd"));
    m_currentSbgpBoxes =
        findAllBoxesWithFourccAndType<box::CSampleToGroupBox>(trak, ilo::toFcc("sbgp"));

    auto trakBox = findFirstBoxWithType<box::CTrackHeaderBox>(trak);
    ILO_ASSERT(trakBox, "No trak box found");
    auto trackId = trakBox->trackID();
    setSampleSizes(trackId, trak.get());  // has to come first since it populates the vector
    setSampleDurations(trackId, trak.get());
    setSampleOffsets(trackId, trak.get());
    setSampleCtsOffsets(trackId, trak.get());
    setSyncSampleFlag(trackId, trak.get());
    setTimeScale(trackId, trak.get());
    // order is important here
    createSampleToSampleGroupInfoMap((*m_sampleInfoTable)[trackId].size());
    setSampleSampleGroupInfoRegular(trackId);
  }
}

void CRegularSampleExtractor::setSampleSizes(const uint32_t& trackId, const BoxElement& node) {
  auto stsz = findFirstBoxWithType<box::CSampleSizeBox>(node);
  auto stz2 = findFirstBoxWithType<box::CCompactSampleSizeBox>(node);

  ILO_ASSERT(stsz != nullptr || stz2 != nullptr, "neither stsz nor stz2 box found");
  ILO_ASSERT(stsz == nullptr || stz2 == nullptr,
             "stsz and stz2 boxes can't exist at the same time.");

  m_sampleCount = stsz != nullptr ? stsz->sampleCount() : stz2->sampleCount();
  uint64_t defaultSampleSize = stsz == nullptr ? 0 : stsz->sampleSize();
  const auto sizeEntriesStsz = stsz != nullptr ? stsz->entrySize() : std::vector<uint32_t>();
  const auto sizeEntriesStz2 = stz2 != nullptr ? stz2->entrySizes() : std::vector<uint16_t>();
  auto sizeEntriesStszSize = sizeEntriesStsz.size();
  auto sizeEntriesStz2Size = sizeEntriesStz2.size();

  auto& currentSampleInfos = (*m_sampleInfoTable)[trackId];
  currentSampleInfos.resize(m_sampleCount);

  for (auto i = 0U; i < m_sampleCount; ++i) {
    if (defaultSampleSize) {
      currentSampleInfos[i].size = defaultSampleSize;
    } else if (sizeEntriesStszSize) {
      currentSampleInfos[i].size = sizeEntriesStsz[i];
    } else if (sizeEntriesStz2Size) {
      currentSampleInfos[i].size = sizeEntriesStz2[i];
    }

    ILO_ASSERT_WITH(currentSampleInfos[i].size <= MAX_ALLOWED_SAMPLES_SIZE_IN_BYTE,
                    std::length_error,
                    "Sample size of %" PRId64 " found that exceeds maximum allow size of %d",
                    currentSampleInfos[i].size, MAX_ALLOWED_SAMPLES_SIZE_IN_BYTE);
  }
}

void CRegularSampleExtractor::setSampleDurations(const uint32_t& trackId, const BoxElement& node) {
  auto stts = findFirstBoxWithType<box::CDecodingTimeToSampleBox>(node);
  ILO_ASSERT(stts != nullptr, "no stts box found");
  size_t totalSampleCount = 0;
  uint64_t currentDtsValue = 0;
  const size_t sampleInfoEntries = (*m_sampleInfoTable)[trackId].size();
  const auto& sttsEntries = stts->entries();
  const auto nrOfSttsEntries = sttsEntries.size();
  auto& currentTrackSampleInfos = (*m_sampleInfoTable)[trackId];
  for (auto i = 0U; i < nrOfSttsEntries; ++i) {
    for (auto n = 0U; n < sttsEntries[i].sampleCount; ++n) {
      ILO_ASSERT(totalSampleCount < sampleInfoEntries, "stts: sample duration count too high");
      currentTrackSampleInfos[totalSampleCount].duration = sttsEntries[i].sampleDelta;
      currentTrackSampleInfos[totalSampleCount].dtsValue = currentDtsValue;
      currentDtsValue += sttsEntries[i].sampleDelta;
      totalSampleCount++;
    }
  }
  ILO_ASSERT(totalSampleCount == (*m_sampleInfoTable)[trackId].size(),
             "stts does not have enough entries");
}

std::vector<uint32_t> getChunkCountPerEntry(const box::CSampleToChunkBox::CVectorEntry& entry_list,
                                            uint32_t totalChunkCount) {
  std::vector<uint32_t> result;

  if (entry_list.size() > 1) {
    for (uint32_t l_i = 0U, i = 1; i < entry_list.size(); ++i, ++l_i) {
      result.push_back(entry_list[i].first_chunk - entry_list[l_i].first_chunk);
    }
  }
  result.push_back(totalChunkCount + 1 - entry_list.back().first_chunk);

  return result;
}

template <class stco_type, class co64_type>
uint32_t totalChunkCount(const stco_type& stco, const co64_type& co64) {
  if (stco != nullptr) {
    return static_cast<uint32_t>(stco->chunkOffsets().size());
  }
  return static_cast<uint32_t>(co64->chunkOffsets().size());
}

template <class stco_type, class co64_type>
uint64_t getChunkOffsetByIndex(const stco_type& stco, const co64_type& co64, uint32_t index) {
  if (stco != nullptr) {
    return stco->chunkOffsets().at(index);
  }
  return co64->chunkOffsets().at(index);
}

void CRegularSampleExtractor::setSampleOffsets(const uint32_t& trackId, const BoxElement& node) {
  auto stco = findFirstBoxWithType<box::CChunkOffsetBox>(node);
  auto co64 = findFirstBoxWithType<box::CChunkOffset64Box>(node);
  ILO_ASSERT(((stco == nullptr && co64 != nullptr) || (co64 == nullptr && stco != nullptr)),
             "no sample offset (stco/co64) box found");

  auto stsc = findFirstBoxWithType<box::CSampleToChunkBox>(node);
  ILO_ASSERT(stsc != nullptr, "no stsc box found");

  auto sampleToChunkEntries = stsc->entries();

  if (!sampleToChunkEntries.size()) {
    ILO_ASSERT(!(*m_sampleInfoTable)[trackId].size(), "stsc does not have enough entries");
    return;
  }

  ILO_ASSERT(sampleToChunkEntries.size() && sampleToChunkEntries.front().first_chunk == 1,
             "first chunk of first record in stsc must be 1");

  auto chunkCountPerEntry =
      getChunkCountPerEntry(sampleToChunkEntries, totalChunkCount(stco, co64));

  size_t totalSampleCount = 0;
  uint64_t sampleOffset = 0;

  auto& currentSampleInfos = (*m_sampleInfoTable)[trackId];
  const size_t nrOfSampleInfos = currentSampleInfos.size();

  for (auto i = 0U; i < sampleToChunkEntries.size(); ++i) {
    auto chunk_index = sampleToChunkEntries[i].first_chunk - 1;

    for (auto c = 0U; c < chunkCountPerEntry[i]; ++c) {
      sampleOffset = getChunkOffsetByIndex(stco, co64, chunk_index);
      for (auto s = 0U; s < sampleToChunkEntries[i].samples_per_chunk; ++s) {
        ILO_ASSERT(totalSampleCount < nrOfSampleInfos, "stsc: sample chunk offset count too high");
        currentSampleInfos[totalSampleCount].offset = sampleOffset;

        const auto currSize = currentSampleInfos[totalSampleCount].size;
        ILO_ASSERT(sampleOffset + currSize <= std::numeric_limits<uint64_t>::max(),
                   "sample offset exceeds the maximum length!");

        sampleOffset += currSize;
        totalSampleCount++;
      }
      ++chunk_index;
    }
  }

  ILO_ASSERT(totalSampleCount == (*m_sampleInfoTable)[trackId].size(),
             "stsc does not have enough entries");
}

void CRegularSampleExtractor::setSampleCtsOffsets(const uint32_t& trackId, const BoxElement& node) {
  auto ctts = findFirstBoxWithType<box::CCompositionTimeToSampleBox>(node);
  if (ctts == nullptr) {
    return;
  }

  uint32_t totalSampleCount = 0;

  for (const auto& entry : ctts->entries()) {
    for (auto i = 0U; i < entry.sampleCount; ++i) {
      ILO_ASSERT(totalSampleCount < (*m_sampleInfoTable)[trackId].size(),
                 "ctts: entry count too high");
      (*m_sampleInfoTable)[trackId].at(totalSampleCount++).ctsOffset = entry.sampleOffset;
    }
  }
  ILO_ASSERT(totalSampleCount == (*m_sampleInfoTable)[trackId].size(),
             "ctts does not have enough entries");
}

void CRegularSampleExtractor::setSyncSampleFlag(const uint32_t& trackId, const BoxElement& node) {
  auto stss = findFirstBoxWithType<box::CSyncSampleTableBox>(node);
  if (stss == nullptr) {
    for (auto& entry : (*m_sampleInfoTable)[trackId]) {
      entry.isSyncSample = true;
    }
  } else {
    for (const auto& entry : stss->entries()) {
      ILO_ASSERT(entry.sampleNumber > 0,
                 "Sample Number 0 is not defined in Sync Sample Box stss. Box is not zero-indexed");
      (*m_sampleInfoTable)[trackId].at(entry.sampleNumber - 1).isSyncSample = true;
    }
  }
}

void CRegularSampleExtractor::setTimeScale(const uint32_t& trackId, const BoxElement& node) {
  auto mdhd = findFirstBoxWithType<box::CMediaHeaderBox>(node);
  ILO_ASSERT(mdhd != nullptr, "No mdhd box found to get timescale from");

  for (auto& sampleInfo : (*m_sampleInfoTable)[trackId]) {
    sampleInfo.timeScale = mdhd->timescale();
  }
}

void CRegularSampleExtractor::setSampleSampleGroupInfoRegular(const uint32_t& trackId) {
  if (m_SampleGroupSampleMap.size() == 0) {
    // No sample group info. Leave default
    return;
  }

  auto& currentSampleInfos = (*m_sampleInfoTable)[trackId];
  ILO_ASSERT(currentSampleInfos.size() == m_SampleGroupSampleMap.size(),
             "SampleInfo table and SampleGroupInfo table are of different size.");

  for (size_t i = 0; i < currentSampleInfos.size(); ++i) {
    setSampleSampleGroupInfo(m_SampleGroupSampleMap[i], currentSampleInfos[i]);
  }
}

std::shared_ptr<TrackIdToTrackSampleInfo> CRegularSampleExtractor::trackIdToTrackSampleInfo()
    const {
  return m_sampleInfoTable;
}

std::unique_ptr<ISampleExtractor> CSampleExtractorFactory::create(const BoxTree& tree) {
  auto moofBox = findFirstBoxWithFourccAndType<box::CContainerBox>(tree, ilo::toFcc("moof"));
  if (moofBox != nullptr) {
    return std::unique_ptr<ISampleExtractor>(new CFragmentedSampleExtractor(tree));
  }
  return std::unique_ptr<ISampleExtractor>(new CRegularSampleExtractor(tree));
}
}  // namespace isobmff
}  // namespace mmt
