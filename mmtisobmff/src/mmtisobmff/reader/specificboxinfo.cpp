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
 * Content: advanced box info class
 */

// External includes
#include "ilo/string_utils.h"
#include "ilo/memory.h"

// Internal includes
#include "common/logging.h"
#include "mmtisobmff/specificboxinfo.h"
#include "tree/boxtree.h"
#include "box/containerbox.h"
#include "box/box.h"
#include "box/tkhdbox.h"
#include "box/tfdtbox.h"
#include "box/mdatbox.h"
#include "box/mfhdbox.h"
#include "box/sidxbox.h"
#include "box/mfhdbox.h"
#include "box/loudnessbox.h"
#include "box/iodsbox.h"
#include "pimpl.h"

namespace mmt {
namespace isobmff {
SDashInfo::SDashInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl) {
  auto p = reader_pimpl.lock();
  ILO_ASSERT(p != nullptr, "reader expired");
  const BoxTree& tree(p->tree());

  // Extract sidx information
  auto boxlist = findAllBoxesWithFourccAndType<box::IBox>(tree, ilo::toFcc("sidx"));

  if (!boxlist.empty()) {
    ILO_ASSERT(boxlist.size() <= 1, "Only a single sidx box is supported.");
    auto sidxBox = std::dynamic_pointer_cast<box::CSegmentIndexBox>(boxlist.at(0));
    ILO_ASSERT(sidxBox != nullptr, "sidx box could not be accessed.");

    m_sidxInfo = ilo::make_unique<SSidxInfo>();
    m_sidxInfo->referenceId = sidxBox->referenceId();
    m_sidxInfo->timescale = sidxBox->timescale();
    m_sidxInfo->earliestPresentationTime = sidxBox->earliestPresentationTime();
    m_sidxInfo->firstOffset = sidxBox->firstOffset();
    m_sidxInfo->referenceCount = sidxBox->referenceCount();

    for (const auto& ref : sidxBox->references()) {
      SSidxInfo::SSidxReference sidxReference;

      sidxReference.referenceType = ref.referenceType;
      sidxReference.referenceSize = ref.referenceSize;
      sidxReference.subsegmentDuration = ref.subsegmentDuration;
      sidxReference.startsWithSap = ref.startsWithSap;
      sidxReference.sapType = ref.sapType;
      sidxReference.sapDeltaTime = ref.sapDeltaTime;

      m_sidxInfo->references.push_back(sidxReference);
    }
  }

  // Extract tfdt information
  boxlist = findAllBoxesWithFourccAndType<box::IBox>(tree, ilo::toFcc("tfdt"));

  if (!boxlist.empty()) {
    m_tfdtInfo = ilo::make_unique<STfdtInfo>();

    for (const auto& box : boxlist) {
      auto tdftBox = std::dynamic_pointer_cast<box::CTrackFragmentMDTBox>(box);
      ILO_ASSERT(tdftBox != nullptr, "TFDT box could not be accessed.");

      m_tfdtInfo->m_baseMediaDecodeTimes.push_back(tdftBox->baseMediaDecodeTime());
    }
  }
}

SMmtpInfo::SMmtpInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl) {
  auto p = reader_pimpl.lock();
  ILO_ASSERT(p != nullptr, "reader expired");
  const BoxTree& tree(p->tree());

  auto mfhdBoxes = findAllBoxesWithFourccAndType<box::IBox>(tree, ilo::toFcc("mfhd"));
  ILO_ASSERT(mfhdBoxes.size() == 1, "Requested mfhd box info is not unique or not available.");

  auto mfhdBox = std::dynamic_pointer_cast<box::CMovieFragmentHeaderBox>(mfhdBoxes.at(0));
  ILO_ASSERT(mfhdBox != nullptr, "MFHD box could not be accessed.");

  auto mdatBoxes = findAllBoxesWithFourccAndType<box::IBox>(tree, ilo::toFcc("mdat"));
  ILO_ASSERT(mdatBoxes.size() == 1, "Requested mdat box info is not unique or not available.");

  auto mdatBox = std::dynamic_pointer_cast<box::CBox>(mdatBoxes.at(0));
  ILO_ASSERT(mdatBox != nullptr, "MDAT box could not be accessed.");

  auto trunBoxes = findAllBoxesWithType<box::CTrackRunBox>(tree);
  ILO_ASSERT(trunBoxes.size() >= 1, "At least 1 trun box shall be present.");

  auto tfhdBoxes = findAllBoxesWithType<box::CTrackFragmentHeaderBox>(tree);
  // This is a known limitation because the spec would allow it.
  // If we encounter MPUs where we have multiple 'trun' within a single 'traf' we will address this.
  ILO_ASSERT(tfhdBoxes.size() == trunBoxes.size(),
             "TFHD boxes occurrencies must equal TRUN boxes occurrencies.");

  for (size_t i = 0; i < trunBoxes.size(); ++i) {
    STrunInfo trunInfo;
    for (const auto& entry : trunBoxes[i]->trunEntries()) {
      uint32_t sampleSize = 0;
      if (trunBoxes[i]->sampleSizePresent()) {
        sampleSize = entry.sampleSize();
      } else if (tfhdBoxes[i]->defaultSampleSizePresent()) {
        sampleSize = tfhdBoxes[i]->defaultSampleSize();
      }
      ILO_ASSERT(sampleSize != 0, "Found sample with size 0.");
      trunInfo.m_sampleSizes.push_back(sampleSize);
    }
    m_truns.push_back(trunInfo);
  }

  m_moofSequenceNumber = mfhdBox->sequenceNumber();

  uint64_t size = mdatBox->size();
  m_mdatPayloadSize = mdatBox->had64BitSizeInInput() ? size - 16 : size - 8;
}

struct SDrcInfo::SPimpl {
 public:
  struct SLudtInfo {
    std::vector<box::CLoudnessBaseBox> m_tlouData;
    std::vector<box::CLoudnessBaseBox> m_alouData;

    bool empty() { return m_tlouData.empty() && m_alouData.empty(); }
  };

  SPimpl(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl) : m_readerPimpl(reader_pimpl) {}

  void handleMoov() {
    auto p = m_readerPimpl.lock();
    ILO_ASSERT(p != nullptr, "reader expired");
    const BoxTree& tree(p->tree());

    auto moovElement =
        findFirstElementWithFourccAndBoxType<box::CContainerBox>(tree, ilo::toFcc("moov"));
    auto trakElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovElement, ilo::toFcc("trak"));

    for (uint32_t index = 0; index < trakElements.size(); ++index) {
      mapTrackIdToIndex(trakElements.at(index), index);
      auto ludInfo = findUdta(trakElements.at(index));
      if (!ludInfo.empty()) {
        storeLudtInfo(ludInfo, index);
      }
    }
  }

  void handleMoof() {
    auto p = m_readerPimpl.lock();
    ILO_ASSERT(p != nullptr, "reader expired");
    const BoxTree& tree(p->tree());

    auto moofElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(tree, ilo::toFcc("moof"));
    for (const auto& moofElement : moofElements) {
      auto mfhd = findFirstBoxWithFourccAndType<box::CMovieFragmentHeaderBox>(moofElement,
                                                                              ilo::toFcc("mfhd"));
      ILO_ASSERT(mfhd != nullptr,
                 "no mfhd box found when looking sequence number of the current fragment");

      auto trafElements =
          findAllElementsWithFourccAndBoxType<box::CContainerBox>(moofElement, ilo::toFcc("traf"));
      for (const auto& trafElement : trafElements) {
        auto tfhd = findFirstBoxWithFourccAndType<box::CTrackFragmentHeaderBox>(trafElement,
                                                                                ilo::toFcc("tfhd"));
        ILO_ASSERT(tfhd != nullptr, "no tfhd box found when looking for udta of the traf box");

        auto ludInfo = findUdta(trafElement);
        if (!ludInfo.empty()) {
          storeFragLudtInfo(ludInfo, trackIndexFromId(tfhd->trackId()), mfhd->sequenceNumber());
        }
      }
    }
  }

  SLudtInfo findUdta(std::reference_wrapper<const BoxElement> nodeElement) {
    auto udtaElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(nodeElement, ilo::toFcc("udta"), 1);
    if (udtaElements.size() > 1) {
      ILO_LOG_WARNING(
          "Multiple udta boxes found on node element %s which violates the standard. Only using "
          "the first.",
          ilo::toString(nodeElement.get().item->type()).c_str());
    }
    if (udtaElements.size() == 1) {
      return createLudtInfo(udtaElements[0]);
    }

    return SLudtInfo();
  }

  SLudtInfo createLudtInfo(std::reference_wrapper<const BoxElement> udataELement) {
    SLudtInfo ludtInfo;

    auto ludtElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(udataELement, ilo::toFcc("ludt"));
    for (auto ludtElement : ludtElements) {
      auto tlouBoxes =
          findAllBoxesWithFourccAndType<box::CLoudnessBaseBox>(ludtElement, ilo::toFcc("tlou"));
      for (const auto& tlouBox : tlouBoxes) {
        ludtInfo.m_tlouData.push_back(*tlouBox);
      }

      auto alouBoxes =
          findAllBoxesWithFourccAndType<box::CLoudnessBaseBox>(ludtElement, ilo::toFcc("alou"));
      for (const auto& alouBox : alouBoxes) {
        ludtInfo.m_alouData.push_back(*alouBox);
      }
    }
    return ludtInfo;
  }

  void storeLudtInfo(const SLudtInfo& ludtInfo, const uint32_t tIndex) {
    m_trackIndexToGlobalLudt[tIndex].m_tlouData.insert(
        m_trackIndexToGlobalLudt[tIndex].m_tlouData.end(), ludtInfo.m_tlouData.begin(),
        ludtInfo.m_tlouData.end());
    m_trackIndexToGlobalLudt[tIndex].m_alouData.insert(
        m_trackIndexToGlobalLudt[tIndex].m_alouData.end(), ludtInfo.m_alouData.begin(),
        ludtInfo.m_alouData.end());
  }

  void storeFragLudtInfo(const SLudtInfo& ludtInfo, const uint32_t tIndex,
                         const uint32_t fSequenceNr) {
    m_trackFragIndexToFragLudt[tIndex][fSequenceNr].m_tlouData.insert(
        m_trackFragIndexToFragLudt[tIndex][fSequenceNr].m_tlouData.end(),
        ludtInfo.m_tlouData.begin(), ludtInfo.m_tlouData.end());
    m_trackFragIndexToFragLudt[tIndex][fSequenceNr].m_alouData.insert(
        m_trackFragIndexToFragLudt[tIndex][fSequenceNr].m_alouData.end(),
        ludtInfo.m_alouData.begin(), ludtInfo.m_alouData.end());
  }

  void mapTrackIdToIndex(std::reference_wrapper<const BoxElement> trakElement,
                         const uint32_t index) {
    auto tkhd =
        findFirstBoxWithFourccAndType<box::CTrackHeaderBox>(trakElement, ilo::toFcc("tkhd"));
    ILO_ASSERT(tkhd != nullptr, "no tkhd box found when looking for the trackId of the traf box");
    m_trackIdToIndex[tkhd->trackID()] = index;
  }

  uint32_t trackIndexFromId(const uint32_t index) { return m_trackIdToIndex.at(index); }

  ilo::ByteBuffer concatBuffers(const SLudtInfo& ludtInfo) {
    uint64_t totalSize = 0;
    for (const auto& box : ludtInfo.m_tlouData) {
      totalSize += box.size();
    }

    for (const auto& box : ludtInfo.m_alouData) {
      totalSize += box.size();
    }

    ilo::ByteBuffer buff;
    buff.resize(static_cast<size_t>(totalSize));

    ilo::ByteBuffer::iterator iter = buff.begin();

    for (auto& box : ludtInfo.m_tlouData) {
      box.write(buff, iter);
    }

    for (auto& box : ludtInfo.m_alouData) {
      box.write(buff, iter);
    }

    return buff;
  }

 public:
  std::map<uint32_t, SLudtInfo> m_trackIndexToGlobalLudt;
  std::map<uint32_t, std::map<uint32_t, SLudtInfo>> m_trackFragIndexToFragLudt;

 private:
  std::weak_ptr<CIsobmffReader::Pimpl> m_readerPimpl;
  std::map<uint32_t, uint32_t> m_trackIdToIndex;
};

SDrcInfo::SDrcInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl)
    : m_pimpl(new SDrcInfo::SPimpl(reader_pimpl)) {
  // Look for global ludt info on trak level
  m_pimpl->handleMoov();

  // Look for ludt info on traf level
  m_pimpl->handleMoof();
}

ilo::ByteBuffer SDrcInfo::globalLudtData(const uint32_t trackIndex) const {
  if (m_pimpl->m_trackIndexToGlobalLudt.size() == 0) {
    ILO_LOG_WARNING("Emtpy global tlou data requested by user");
    return ilo::ByteBuffer();
  }
  ILO_ASSERT(trackIndex < m_pimpl->m_trackIndexToGlobalLudt.size(), "TrackIndex is out of range");

  return m_pimpl->concatBuffers(m_pimpl->m_trackIndexToGlobalLudt.at(trackIndex));
}

bool SDrcInfo::trackHasLudtUpdates(const uint32_t trackIndex) const {
  if (m_pimpl->m_trackFragIndexToFragLudt.empty()) {
    return false;
  }

  if (trackIndex >= m_pimpl->m_trackFragIndexToFragLudt.size()) {
    ILO_LOG_WARNING(
        "User requested info about ludt updates from an invalid "
        "trackIndex of %d with a total of %d tracks.",
        trackIndex, m_pimpl->m_trackFragIndexToFragLudt.size());
    return false;
  }

  if (m_pimpl->m_trackFragIndexToFragLudt.at(trackIndex).empty()) {
    return false;
  }

  return true;
}

ilo::ByteBuffer SDrcInfo::fragmentLudtData(const uint32_t trackIndex,
                                           const uint32_t fragmentNr) const {
  if (m_pimpl->m_trackFragIndexToFragLudt.empty()) {
    ILO_LOG_WARNING("User requested frag ludt data, but no fragment with ludt data was found");
    return ilo::ByteBuffer();
  }
  ILO_ASSERT(trackIndex < m_pimpl->m_trackFragIndexToFragLudt.size(), "TrackIndex is out of range");

  auto fragNrToLudtMap = m_pimpl->m_trackFragIndexToFragLudt[trackIndex];
  auto iter = fragNrToLudtMap.find(fragmentNr);
  if (iter != fragNrToLudtMap.end()) {
    return m_pimpl->concatBuffers(iter->second);
  } else {
    return ilo::ByteBuffer();
  }
}

struct SDrcExtendedInfo::SPimpl {
 public:
  SPimpl(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl)
      : m_drcInfo(new SDrcInfo(reader_pimpl)) {}

  std::vector<SDrcExtendedInfo::SLoudnessBaseInfo> parseData(
      ilo::ByteBuffer::const_iterator& iter, const ilo::ByteBuffer::const_iterator& end) {
    std::vector<SDrcExtendedInfo::SLoudnessBaseInfo> extDrcInfo;

    while (iter != end) {
      SDrcExtendedInfo::SLoudnessBaseInfo ludtData;

      box::CLoudnessBaseBox lBox(iter, end);
      ludtData.type = lBox.type();

      auto lbsVect = lBox.loudnessBaseSets();
      for (const auto& lbs : lbsVect) {
        SBaseData bData;
        bData.eqSetId = lbs.eqSetId;
        bData.downmixId = lbs.downmixId;
        bData.drcSetId = lbs.drcSetId;
        bData.bsSamplePeakLevel = lbs.bsSamplePeakLevel;
        bData.bsTruePeakLevel = lbs.bsTruePeakLevel;
        bData.measurementSystemForTp = lbs.measurementSystemForTp;
        bData.reliabilityForTp = lbs.reliabilityForTp;

        for (const auto& ms : lbs.measurementSets) {
          SDrcExtendedInfo::SMeasurementSet mSet;
          mSet.methodDefinition = ms.methodDefinition;
          mSet.methodValue = ms.methodValue;
          mSet.measurementSystem = ms.measurementSystem;
          mSet.reliability = ms.reliability;
          bData.measurementSets.push_back(mSet);
        }
        ludtData.baseData.push_back(bData);
      }
      extDrcInfo.push_back(ludtData);
    }

    return extDrcInfo;
  }

  std::unique_ptr<SDrcInfo> m_drcInfo;
};

SDrcExtendedInfo::SDrcExtendedInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl)
    : m_pimpl(new SDrcExtendedInfo::SPimpl(reader_pimpl)) {}

std::vector<SDrcExtendedInfo::SLoudnessBaseInfo> SDrcExtendedInfo::globalLudtData(
    const uint32_t trackIndex) const {
  auto ludtDataBlob = m_pimpl->m_drcInfo->globalLudtData(trackIndex);

  ilo::ByteBuffer::const_iterator iter = ludtDataBlob.begin();
  ilo::ByteBuffer::const_iterator end = ludtDataBlob.end();

  return m_pimpl->parseData(iter, end);
}

bool SDrcExtendedInfo::trackHasLudtUpdates(const uint32_t trackIndex) const {
  return m_pimpl->m_drcInfo->trackHasLudtUpdates(trackIndex);
}

std::vector<SDrcExtendedInfo::SLoudnessBaseInfo> SDrcExtendedInfo::fragmentLudtData(
    const uint32_t trackIndex, const uint32_t fragmentNr) const {
  auto ludtDataBlob = m_pimpl->m_drcInfo->fragmentLudtData(trackIndex, fragmentNr);

  ilo::ByteBuffer::const_iterator iter = ludtDataBlob.begin();
  ilo::ByteBuffer::const_iterator end = ludtDataBlob.end();

  return m_pimpl->parseData(iter, end);
}

SIodsInfo::SIodsInfo(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl) {
  auto p = reader_pimpl.lock();
  ILO_ASSERT(p != nullptr, "reader expired");
  const BoxTree& tree(p->tree());

  auto iods = findFirstBoxWithFourccAndType<box::CObjectDescriptorBox>(tree, ilo::toFcc("iods"));
  if (iods) {
    auto iodsInfo = ilo::make_unique<SIodsInfo::SIodsEntry>();
    iodsInfo->audioProfileLevelIndication = iods->audioProfileLevelIndication();
    m_iodsEntry = std::move(iodsInfo);
  }
}

bool SIodsInfo::iodsInfoAvailable() const {
  return m_iodsEntry != nullptr;
}

uint8_t SIodsInfo::audioProfileLevelIndication() const {
  ILO_ASSERT(m_iodsEntry != nullptr, "No iods box available to retrieve information from.");
  return m_iodsEntry->audioProfileLevelIndication;
}
}  // namespace isobmff
}  // namespace mmt
