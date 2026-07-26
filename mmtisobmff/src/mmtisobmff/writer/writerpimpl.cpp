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

/*
 * Project: MPEG-4 ISO Base Media File Format (ISO BMFF) library
 * Content: mmtisobmff writer pimpl class(es)
 */

// System includes
#include <limits>
#include <cstddef>
#include <utility>
#include <cmath>
#include <cerrno>
#include <chrono>
#include <thread>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "writer/writerpimpl.h"
#include "writer/mediafragment_tree_builder.h"
#include "writer/traf_sample_enhancer.h"
#include "writer/traf_tree_enhancer.h"
#include "writer/trak_samplegroups_enhancer.h"
#include "writer/trak_editlist_enhancer.h"
#include "writer/trak_userdata_enhancer.h"
#include "service/servicesingleton.h"
#include "box/trexbox.h"
#include "box/ftypbox.h"
#include "box/stypbox.h"
#include "box/sidxbox.h"
#include "box/elstbox.h"
#include "box/mdatbox.h"

namespace mmt {
namespace isobmff {
const std::unique_ptr<IIsobmffOutput>& CIsobmffWriter::Pimpl::output() const {
  ILO_ASSERT(m_output != nullptr, "Output is not pointing to a valid IIsobmffOutput instance.");
  return m_output;
}

void CIsobmffWriter::Pimpl::closeCurrentOutput() {
  m_output.reset();
}

void CIsobmffWriter::Pimpl::closeAllOutputs() {
  // Delete the sample store (needed to release the filehandlers for plain mp4 files)
  m_sampleStore.reset();

  // Delete the output (needed to release the filehandlers for fragmented mp4 files)
  m_tmpOutput.reset();
  m_output.reset();
}

void CIsobmffWriter::Pimpl::fillStaticMoovInfo() {
  auto moovBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(*(m_tree), ilo::toFcc("moov"));
  ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
  BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

  if (m_writeIods) {
    auto iodsBoxElements = findAllElementsWithFourccAndBoxType<box::CObjectDescriptorBox>(
        moovBoxElement, ilo::toFcc("iods"));
    ILO_ASSERT(iodsBoxElements.size() == 1, "one Object Descriptor box should be present");
    BoxElement& iodsBoxElement = const_cast<BoxElement&>(iodsBoxElements[0].get());
    auto iodsBox = std::dynamic_pointer_cast<box::CObjectDescriptorBox>(iodsBoxElement.item);

    // Copy data of old box and set new trackIds
    box::CObjectDescriptorBox::SIodsBoxWriteConfig config = createIodsConfig(iodsBox);
    config.trackIds = m_mp4aTrackIds;

    // Replace old iods box with new one
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
    nodefactory->replaceNode(iodsBoxElement,
                             box::CObjectDescriptorBox::SIodsBoxWriteConfig(config));
  }

  auto trakBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
  ILO_ASSERT(trakBoxElements.size() >= 1, "one or more trak boxes should be present");
}

void CIsobmffWriter::Pimpl::fillTrakEnhancersConfigs(STrakEnhancersConfig& config,
                                                     const MetaSampleVec& sampleMetaDataVec,
                                                     const uint32_t& trackId) {
  uint32_t stscSamplesPerChunk = 0;
  box::CDecodingTimeToSampleBox::SSttsEntry sttsEntry;

  ILO_ASSERT_WITH(sampleMetaDataVec.size() != 0, std::invalid_argument,
                  "There are no samples in the vector");
  ILO_ASSERT_WITH(
      (std::find_if(sampleMetaDataVec.begin(), sampleMetaDataVec.end(),
                    [&](const CMetaSample& sample) { return sample.trackId == trackId; }) !=
       sampleMetaDataVec.end()),
      std::invalid_argument, "There are no samples in the vector with trackId %d", trackId);

  // Check if any of the samples in the vector has an offset greater than the max possible 32bit
  // integer
  bool largeOffsets = false;
  if (std::find_if(sampleMetaDataVec.begin(), sampleMetaDataVec.end(),
                   [&](const CMetaSample& sample) {
                     return sample.offset > std::numeric_limits<uint32_t>::max();
                   }) != sampleMetaDataVec.end()) {
    largeOffsets = true;
  }

  for (size_t i = 0; i < sampleMetaDataVec.size(); ++i) {
    if (sampleMetaDataVec[i].trackId == trackId) {
      updateSampleGroupsConfig(config.sampleGroupsConfig, sampleMetaDataVec[i]);

      // -----------------------------STSZ-----------------------------
      config.trakSampleEnhancerConfig.stszConfig.entrySize.push_back(
          static_cast<uint32_t>(sampleMetaDataVec[i].size));
      config.trakSampleEnhancerConfig.stszConfig.sampleCount++;

      // -----------------------------STTS-----------------------------
      updateSttsBox(config.trakSampleEnhancerConfig, sttsEntry,
                    static_cast<uint32_t>(sampleMetaDataVec[i].duration));

      // --------------------------STCO/CO64---------------------------
      stscSamplesPerChunk++;

      // Set chunk offset in stco or co64 box. Done only at the beginning of each chunk
      if (stscSamplesPerChunk == 1) {
        if (largeOffsets) {
          config.trakSampleEnhancerConfig.co64Config.chunkOffsets.push_back(
              sampleMetaDataVec[i].offset);
        } else {
          config.trakSampleEnhancerConfig.stcoConfig.chunkOffsets.push_back(
              static_cast<uint32_t>(sampleMetaDataVec[i].offset));
        }
      }

      // -----------------------------STSS-----------------------------
      if (sampleMetaDataVec[i].isSyncSample) {
        // Hint: STSS entry is NOT zero-based!
        config.trakSampleEnhancerConfig.stssConfig.entries.push_back(
            box::CSyncSampleTableBox::SStssEntry{
                config.trakSampleEnhancerConfig.stszConfig.sampleCount});
      } else {
        // This triggers the stss box creating. If this stays true, no stss box is written
        config.trakSampleEnhancerConfig.allSamplesSyncSamples = false;
      }

      // -----------------------------CTTS-----------------------------
      if (config.trakSampleEnhancerConfig.cttsConfig.entries.empty() ||
          sampleMetaDataVec[i].ctsOffset !=
              config.trakSampleEnhancerConfig.cttsConfig.entries.back().sampleOffset) {
        box::CCompositionTimeToSampleBox::SCttsEntry cttsEntry;
        cttsEntry.sampleOffset = sampleMetaDataVec[i].ctsOffset;
        cttsEntry.sampleCount++;
        config.trakSampleEnhancerConfig.cttsConfig.entries.push_back(cttsEntry);
      } else {
        config.trakSampleEnhancerConfig.cttsConfig.entries.back().sampleCount++;
      }
    } else {
      // -----------------------------STSC-----------------------------
      updateStscBox(config.trakSampleEnhancerConfig, stscSamplesPerChunk, largeOffsets);
      stscSamplesPerChunk = 0;
    }
  }
  // At the end we need to create an stts entry and a stco/co64 entry with the information of the
  // last samples.
  // -----------------------------STTS-----------------------------
  config.trakSampleEnhancerConfig.sttsConfig.entries.push_back(sttsEntry);

  // -----------------------------STSC-----------------------------
  updateStscBox(config.trakSampleEnhancerConfig, stscSamplesPerChunk, largeOffsets);

  // -----------------------------CTTS-----------------------------
  // If there is only 1 entry in the cttsConfig and the sampleOffset of this entry is 0, then this
  // means that there are actually no cts offsets for the samples and the ctts box should not be
  // written at all. Therefore the entries are deleted.
  if (config.trakSampleEnhancerConfig.cttsConfig.entries.size() == 1 &&
      config.trakSampleEnhancerConfig.cttsConfig.entries[0].sampleOffset == 0) {
    config.trakSampleEnhancerConfig.cttsConfig.entries.clear();
  }
}

void CIsobmffWriter::Pimpl::createMvexBox() {
  ILO_ASSERT(m_hasFragments, "Mvex/Trex boxes cannot be used for a plain mp4 file");

  auto moovBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(*(m_tree), ilo::toFcc("moov"));
  ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
  BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

  ILO_ASSERT(
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("mvex"))
          .empty(),
      "Mvex box is already existing.");

  auto nodefactory = CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
  auto mvexBoxElement = nodefactory->createNode(
      moovBoxElement, box::CContainerBox::SContainerBoxWriteConfig(ilo::toFcc("mvex")));

  auto trakBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
  ILO_ASSERT(trakBoxElements.size() >= 1, "one or more trak boxes must be present");
  for (auto trakBoxElementRef : trakBoxElements) {
    auto tkhdBoxes =
        findAllBoxesWithFourccAndType<box::CTrackHeaderBox>(trakBoxElementRef, ilo::toFcc("tkhd"));
    ILO_ASSERT(tkhdBoxes.size() == 1, "one and only one tkhd box should be present for each trak");

    box::CTrackExtendsBox::STrexBoxWriteConfig trexConfig;
    trexConfig.trackID = tkhdBoxes.at(0)->trackID();
    trexConfig.defaultSampleDescriptionIndex = 1;

    nodefactory->createNode(mvexBoxElement, trexConfig);
  }
}

void CIsobmffWriter::Pimpl::createStypBox(ilo::ByteBuffer& stypBuff, const bool& isLastSegment) {
  auto ftypBoxes = findAllBoxesWithFourccAndType<box::CFileTypeBox>(*m_tree, ilo::toFcc("ftyp"));
  ILO_ASSERT(ftypBoxes.size() == 1, "one and only one ftyp box should be present");

  // Copy ftyp data to styp
  box::CSegmentTypeBox::SStypBoxWriteConfig stypConfig;
  stypConfig.minorVersion = ftypBoxes.at(0)->minorVersion();
  stypConfig.majorBrand = ftypBoxes.at(0)->majorBrand();
  stypConfig.compatibleBrands = ftypBoxes.at(0)->compatibleBrands();

  if (isLastSegment) {
    stypConfig.compatibleBrands.push_back(ilo::toFcc("lmsg"));
  }

  // Create styp box and write to tmp buffer
  box::CSegmentTypeBox stypBox(stypConfig);
  stypBuff.resize(static_cast<size_t>(stypBox.size()));
  ilo::ByteBuffer::iterator iter = stypBuff.begin();
  stypBox.write(stypBuff, iter);
}

void CIsobmffWriter::Pimpl::createSidxBox(ilo::ByteBuffer& sidxBuff) {
  box::CSegmentIndexBox::SSidxBoxWriteConfig sidxConfig;

  auto moovBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(*m_tree, ilo::toFcc("moov"));
  ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
  BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

  auto trakBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
  ILO_ASSERT(trakBoxElements.size() == 1,
             "We currently only support fragmented files with 1 track");
  BoxElement& trakBoxElement = const_cast<BoxElement&>(trakBoxElements[0].get());

  auto edtsBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("edts"));
  ILO_ASSERT(edtsBoxElements.size() <= 1, "zero or one edts box should be present");

  std::shared_ptr<box::CEditListBox> elstBox;
  if (edtsBoxElements.size() == 1) {
    BoxElement& edtsBoxElement = const_cast<BoxElement&>(edtsBoxElements[0].get());
    auto elstBoxElements =
        findAllElementsWithFourccAndBoxType<box::CEditListBox>(edtsBoxElement, ilo::toFcc("elst"));
    BoxElement& elstBoxElement = const_cast<BoxElement&>(elstBoxElements[0].get());
    elstBox = std::dynamic_pointer_cast<box::CEditListBox>(elstBoxElement.item);

    ILO_ASSERT(elstBox == nullptr,
               "Currently we don't support writing edit lists. When this support is enabled, "
               "we have to apply the edit list to the earliest_presentation_time");
  }

  auto tkhdBoxElements =
      findAllElementsWithFourccAndBoxType<box::CTrackHeaderBox>(trakBoxElement, ilo::toFcc("tkhd"));
  BoxElement& tkhdBoxElement = const_cast<BoxElement&>(tkhdBoxElements[0].get());
  auto tkhdBox = std::dynamic_pointer_cast<box::CTrackHeaderBox>(tkhdBoxElement.item);

  auto mdiaBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("mdia"));
  ILO_ASSERT(mdiaBoxElements.size() == 1,
             "one and only one mdia box should be present for each trak");

  auto mdhdBoxElements =
      findAllElementsWithFourccAndBoxType<box::CMediaHeaderBox>(trakBoxElement, ilo::toFcc("mdhd"));
  BoxElement& mdhdBoxElement = const_cast<BoxElement&>(mdhdBoxElements[0].get());
  auto mdhdBox = std::dynamic_pointer_cast<box::CMediaHeaderBox>(mdhdBoxElement.item);

  std::vector<uint64_t> earliestPtsAllFrags;
  size_t indexFragments = 0;
  uint64_t dts = 0;

  for (const auto& fragTree : m_fragTrees) {
    auto moofBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(*fragTree, ilo::toFcc("moof"));
    BoxElement& moofBoxElement = const_cast<BoxElement&>(moofBoxElements[0].get());

    auto trafBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(moofBoxElement, ilo::toFcc("traf"));
    ILO_ASSERT(trafBoxElements.size() == 1,
               "We currently only support fragmented files with 1 track");
    BoxElement& trafBoxElement = const_cast<BoxElement&>(trafBoxElements[0].get());

    auto trunBoxElements =
        findAllElementsWithFourccAndBoxType<box::CTrackRunBox>(trafBoxElement, ilo::toFcc("trun"));
    BoxElement& trunBoxElement = const_cast<BoxElement&>(trunBoxElements[0].get());
    auto trunBox = std::dynamic_pointer_cast<box::CTrackRunBox>(trunBoxElement.item);

    uint64_t curFragEarliestPts = std::numeric_limits<uint64_t>::max();
    uint64_t curFragEndPts = 0;
    uint64_t ptsFirstSap = 0;
    bool sapFound = false;

    size_t indexSample = 0;
    for (const auto& trunEntry : trunBox->trunEntries()) {
      uint64_t samplePts =
          static_cast<uint64_t>(static_cast<int64_t>(dts) + trunEntry.sampleCtsOffset());
      dts += trunEntry.sampleDuration();

      auto sampleFlags = tools::valueToSampleFlags(trunEntry.sampleFlags());
      if (!sampleFlags.isNonSyncSample && !sapFound)  // is_sync_sample
      {
        ptsFirstSap = samplePts;
        sapFound = true;
      }

      curFragEarliestPts = (samplePts < curFragEarliestPts) ? samplePts : curFragEarliestPts;
      curFragEndPts = (dts > curFragEndPts) ? dts : curFragEndPts;

      indexSample++;
    }

    earliestPtsAllFrags.push_back(curFragEarliestPts);

    box::CSegmentIndexBox::SSidxReference reference;

    reference.referenceType = 0;
    reference.referenceSize = static_cast<uint32_t>(updateSizeAndReturnTotalSize(*fragTree));

    // startsWithSap is true when sample with earliest presentation time is a sync Sample
    reference.startsWithSap = (sapFound && (curFragEarliestPts == ptsFirstSap));

    if (!reference.startsWithSap && sapFound) {
      reference.sapDeltaTime = static_cast<uint32_t>(ptsFirstSap - curFragEarliestPts);
    }

    ILO_ASSERT(m_sapType != ESapType::SapTypeInvalid, "invalid SAP Type");
    reference.sapType = static_cast<uint8_t>(m_sapType);

    sidxConfig.references.push_back(reference);

    // To calculate the subsegmentDuration of a fragment we need the earliestPresentationTime of the
    // next fragment. Therefore we always calculate the subsegmentDuration of the previous fragment
    // and in the case of last fragment we calculate the segmentDuration for both this fragment and
    // previous fragment.
    if (indexFragments == m_fragTrees.size() - 1)  // last fragment
    {
      sidxConfig.references[indexFragments].subsegmentDuration =
          static_cast<uint32_t>(curFragEndPts - curFragEarliestPts);
    }
    if (indexFragments != 0) {
      sidxConfig.references[indexFragments - 1].subsegmentDuration =
          static_cast<uint32_t>(curFragEarliestPts - earliestPtsAllFrags[indexFragments - 1]);
    }
    indexFragments++;
  }

  sidxConfig.referenceId = tkhdBox->trackID();
  sidxConfig.timescale = mdhdBox->timescale();
  if (!earliestPtsAllFrags.empty()) {
    sidxConfig.earliestPresentationTime =
        *(std::min_element(earliestPtsAllFrags.begin(), earliestPtsAllFrags.end()));
  }
  sidxConfig.firstOffset = 0;

  box::CSegmentIndexBox sidxBox(sidxConfig);
  sidxBuff.resize(static_cast<size_t>(sidxBox.size()));
  ilo::ByteBuffer::iterator iter = sidxBuff.begin();
  sidxBox.write(sidxBuff, iter);
}

void CIsobmffWriter::Pimpl::addSidxBox(const size_t maxChunkSize) {
  ILO_ASSERT(m_output != nullptr, "Output module is a zero pointer");
  ILO_ASSERT(m_tmpOutput != nullptr, "Tmp output module is a zero pointer");

  // switch m_output with m_tmpOutput again since we now want to finish writing the actual file
  std::swap(m_tmpOutput, m_output);

  ilo::ByteBuffer sidxBuff;
  createSidxBox(sidxBuff);

  // Write sidx box after init fragment
  m_output->write(sidxBuff.begin(), sidxBuff.end());

  pos_type endPos = m_tmpOutput->tell();
  pos_type curPos = 0;
  ilo::CUniqueBuffer readBuffer = nullptr;
  size_t bytesToRead = static_cast<size_t>(endPos - curPos);

  // Read tmp file and write it to output file
  while (bytesToRead != 0) {
    readBuffer = m_tmpOutput->read(static_cast<size_t>(curPos),
                                   (bytesToRead > maxChunkSize) ? maxChunkSize : bytesToRead);
    m_output->write(readBuffer->begin(), readBuffer->end());
    bytesToRead = static_cast<size_t>(bytesToRead - readBuffer->size());
  }
}

void CIsobmffWriter::Pimpl::createInitFragment(std::unique_ptr<IIsobmffOutput>&& outputInstance) {
  ILO_ASSERT(!m_initWritten, "Init Fragment was already written");

  if (outputInstance != nullptr) {
    m_output = std::move(outputInstance);
  }

  ILO_ASSERT(m_output != nullptr, "Output module is a zero pointer");

  updateNextTrackId();

  // Add default Sample Group description box (if avaialble)
  if (!m_defaultSampleGroupInfoMap.empty()) {
    auto moovBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(*m_tree, ilo::toFcc("moov"));
    ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
    BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

    auto trakBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
    ILO_ASSERT(trakBoxElements.size() >= 1, "one or more trak boxes should be present");
    for (auto trakBoxElementRef : trakBoxElements) {
      BoxElement& trakBoxElement = const_cast<BoxElement&>(trakBoxElementRef.get());

      auto tkhdBoxElements = findAllElementsWithFourccAndBoxType<box::CTrackHeaderBox>(
          trakBoxElement, ilo::toFcc("tkhd"));
      BoxElement& tkhdBoxElement = const_cast<BoxElement&>(tkhdBoxElements[0].get());
      auto tkhdBox = std::dynamic_pointer_cast<box::CTrackHeaderBox>(tkhdBoxElement.item);

      // Enhance only the 'trak' box of the right track
      if (m_defaultSampleGroupInfoMap.find(tkhdBox->trackID()) !=
          m_defaultSampleGroupInfoMap.end()) {
        auto stblBoxElements = findAllElementsWithFourccAndBoxType<box::CContainerBox>(
            trakBoxElement, ilo::toFcc("stbl"));
        ILO_ASSERT(stblBoxElements.size() == 1,
                   "one and only one stbl box should be present for each trak");
        BoxElement& stblBoxElement = const_cast<BoxElement&>(stblBoxElements[0].get());

        SSampleGroupsConfig config(1, 0, true);
        config.sampleGroupInfoNew = *m_defaultSampleGroupInfoMap[tkhdBox->trackID()];
        // Update the sample group entries
        updateEntries(config);

        if (config.prolConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() != 0) {
          CTrakSampleGroupsEnhancer{stblBoxElement, config.prolConfig.boxesConfig, true};
        }

        if (config.rollConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() != 0) {
          CTrakSampleGroupsEnhancer{stblBoxElement, config.rollConfig.boxesConfig, true};
        }

        if (config.sapConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() != 0) {
          CTrakSampleGroupsEnhancer{stblBoxElement, config.sapConfig.boxesConfig, true};
        }
      }
    }
  }

  createMvexBox();

  uint64_t treeSize = updateSizeAndReturnTotalSize(*m_tree);
  ilo::ByteBuffer buff(static_cast<size_t>(treeSize));
  ilo::ByteBuffer::iterator iter = buff.begin();
  ilo::ByteBuffer::const_iterator begConst = buff.begin();
  ilo::ByteBuffer::const_iterator endConst = buff.end();
  serializeTree(*m_tree, buff, iter);
  Pimpl::output()->write(begConst, endConst);

  if (m_writeSidx) {
    // Switch output file with tmp output file so that fragments will be written to tmp file
    std::swap(m_output, m_tmpOutput);
  }
}

void CIsobmffWriter::Pimpl::createFragments(std::unique_ptr<IIsobmffOutput>&& outputInstance) {
  if (outputInstance != nullptr) {
    m_output = std::move(outputInstance);
  }

  ILO_ASSERT(m_output != nullptr, "Output module is a zero pointer");

  std::vector<CMetaSample> metaDataSamples;

  auto sampleMetaDataVec = m_sampleStore->getSampleMetadata();

  ILO_ASSERT_WITH(sampleMetaDataVec.size() > 0, std::logic_error,
                  "No samples added to be written to a segment");
  auto currentFragmentNr = sampleMetaDataVec.at(0).fragmentNumber;

  for (const auto& sampleMetaData : sampleMetaDataVec) {
    if (currentFragmentNr != sampleMetaData.fragmentNumber) {
      // Make sure we have trackIDs grouped. It does not have to be sorted, but
      // it is convenient
      std::stable_sort(
          metaDataSamples.begin(), metaDataSamples.end(),
          [](const CMetaSample& lhs, const CMetaSample& rhs) { return lhs.trackId < rhs.trackId; });
      currentFragmentNr = sampleMetaData.fragmentNumber;
      createFragment(metaDataSamples);
      metaDataSamples.clear();
    }
    metaDataSamples.push_back(sampleMetaData);
  }

  // Write the rest of the samples to a fragment
  if (metaDataSamples.size() > 0) {
    createFragment(metaDataSamples);
  }

  auto sink = ilo::make_unique<CMemorySampleSink>();
  auto interleaver = ilo::make_unique<CTimeAligned>(m_chunkSize);
  auto sampleStore =
      ilo::make_unique<CInterleavingSampleStore>(std::move(sink), std::move(interleaver));
  m_sampleStore = std::move(sampleStore);
}

void CIsobmffWriter::Pimpl::createFragment(const std::vector<CMetaSample>& metaDataSamples) {
  SMediaFragmentTreeConfig fragConfig;
  fragConfig.mfhdConfig.sequenceNumber = metaDataSamples.at(0).fragmentNumber;

  CMediaFragmentTreeBuilder mediaFragTreeBuilder(fragConfig);
  auto fragTree = mediaFragTreeBuilder.build();
  auto nodefactory = CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();

  size_t index = 0;
  while (index < metaDataSamples.size()) {
    auto bmdt = m_baseMediaDecodeTime.find(metaDataSamples[index].trackId);
    if (bmdt == m_baseMediaDecodeTime.end()) {
      m_baseMediaDecodeTime.insert(std::make_pair(metaDataSamples[index].trackId, 0U));
    }

    auto trafBoxElement = nodefactory->createNode(
        (*fragTree)[0], box::CContainerBox::SContainerBoxWriteConfig(ilo::toFcc("traf")));

    STrafTreeEnhancerConfig trafTreeConfig;
    trafTreeConfig.tfhdConfig.trackId = metaDataSamples[index].trackId;
    trafTreeConfig.tfhdConfig.defaultSampleDurationPresent =
        true;  // Disable later if duration differs in fragment
    trafTreeConfig.tfhdConfig.defaultSampleDuration =
        static_cast<uint32_t>(metaDataSamples[index].duration);
    trafTreeConfig.tfhdConfig.defaultSampleFlagsPresent =
        true;  // Disable later if flags differ starting from startingIndex + 1

    trafTreeConfig.tfhdConfig.baseDataOffsetPresent = false;
    trafTreeConfig.tfhdConfig.defaultBaseIsMoof = true;
    trafTreeConfig.tfhdConfig.defaultSampleSizePresent = false;
    trafTreeConfig.tfhdConfig.durationIsEmpty = false;
    trafTreeConfig.tfhdConfig.sampleDescriptionIndexPresent = false;

    trafTreeConfig.tfdtConfig.baseMediaDecodeTime =
        m_baseMediaDecodeTime.at(metaDataSamples[index].trackId);

    if (m_forceTfdtV1) {
      trafTreeConfig.tfdtConfig.version = 1;
    }

    STrafSampleEnhancerConfig sampleConfig;
    sampleConfig.trunConfig.sampleSizePresent = true;
    sampleConfig.trunConfig.sampleCtsOffsetPresent =
        false;  // Enabled if value differs from 0 in fragment
    sampleConfig.trunConfig.dataOffsetPresent = true;
    sampleConfig.trunConfig.dataoffset = 0;  // Hint: This value is temporal!

    // sampleFlagsPresent and sampleDurationPresent will be set later

    // Check whether a default sample group description box has been already written in the trak
    bool defaultSampleGroupsFlag = false;
    if (m_defaultSampleGroupInfoMap.find(trafTreeConfig.tfhdConfig.trackId) !=
        m_defaultSampleGroupInfoMap.end()) {
      defaultSampleGroupsFlag = true;
    }

    SSampleGroupsConfig sampleGroupsConfig(1, 0, !defaultSampleGroupsFlag);

    size_t startingIndex = index;
    while (index < metaDataSamples.size() &&
           trafTreeConfig.tfhdConfig.trackId == metaDataSamples[index].trackId) {
      if (metaDataSamples[index].fragmentNumber == 0) {
        ILO_LOG_WARNING(
            "Fragment number of 0 is not a common fragment number. It usually starts with 1.");
      }

      ILO_ASSERT(metaDataSamples[index].duration <= std::numeric_limits<uint32_t>::max(),
                 "Sample duration value is bigger than 32bit");
      ILO_ASSERT(metaDataSamples[index].size <= std::numeric_limits<uint32_t>::max(),
                 "Sample size value is bigger than 32bit");

      // Sanity check: default sample group should match the one defined for each sample
      if (defaultSampleGroupsFlag &&
          metaDataSamples[index].sampleGroupInfo.type != SampleGroupType::none) {
        ILO_ASSERT_WITH(*(m_defaultSampleGroupInfoMap[trafTreeConfig.tfhdConfig.trackId]) ==
                            metaDataSamples[index].sampleGroupInfo,
                        std::invalid_argument,
                        "The sample group attached to the sample differs from the default sample "
                        "group: this is currently not supported");
      }

      if (trafTreeConfig.tfhdConfig.defaultSampleDurationPresent &&
          trafTreeConfig.tfhdConfig.defaultSampleDuration != metaDataSamples[index].duration) {
        trafTreeConfig.tfhdConfig.defaultSampleDurationPresent = false;
        trafTreeConfig.tfhdConfig.defaultSampleDuration = 0;
      }

      if (metaDataSamples[index].ctsOffset != 0) {
        sampleConfig.trunConfig.sampleCtsOffsetPresent = true;
      }

      switch (index - startingIndex) {
        case 0:
          sampleConfig.trunConfig.firstSampleFlags = getFlagsFromSample(metaDataSamples[index]);
          // Initialize equal to sampleConfig.trunConfig.firstSampleFlags to prevent
          // firstSampleFlags if only 1 entry is present
          trafTreeConfig.tfhdConfig.defaultSampleFlags = sampleConfig.trunConfig.firstSampleFlags;
          break;
        case 1:
          trafTreeConfig.tfhdConfig.defaultSampleFlags = getFlagsFromSample(metaDataSamples[index]);
          break;
        default:
          if (trafTreeConfig.tfhdConfig.defaultSampleFlags !=
              getFlagsFromSample(metaDataSamples[index])) {
            trafTreeConfig.tfhdConfig.defaultSampleFlagsPresent = false;
          }
      }

      box::CTrunEntry entry;
      // Will be ignored if sampleConfig.trunConfig.sampleDurationPresent flag is not set as well
      entry.setSampleDuration(static_cast<uint32_t>(metaDataSamples[index].duration));
      entry.setSampleCtsOffset(metaDataSamples[index].ctsOffset);
      entry.setSampleFlags(getFlagsFromSample(metaDataSamples[index]));

      if (sampleConfig.trunConfig.sampleSizePresent) {
        entry.setSampleSize(static_cast<uint32_t>(metaDataSamples[index].size));
      }

      sampleConfig.trunConfig.trunEntries.push_back(entry);

      m_baseMediaDecodeTime.at(metaDataSamples[index].trackId) += metaDataSamples[index].duration;

      updateSampleGroupsConfig(sampleGroupsConfig, metaDataSamples[index]);

      index++;
      sampleConfig.trunConfig.sampleCount++;
    }
    sampleConfig.trunConfig.sampleDurationPresent =
        !trafTreeConfig.tfhdConfig.defaultSampleDurationPresent;
    sampleConfig.trunConfig.sampleFlagsPresent =
        !trafTreeConfig.tfhdConfig.defaultSampleFlagsPresent;

    if (!sampleConfig.trunConfig.sampleFlagsPresent &&
        sampleConfig.trunConfig.firstSampleFlags != trafTreeConfig.tfhdConfig.defaultSampleFlags) {
      sampleConfig.trunConfig.firstSampleFlagsPresent = true;
    } else {
      sampleConfig.trunConfig.firstSampleFlagsPresent = false;
    }

    CTrafTreeEnhancer{trafBoxElement, trafTreeConfig};
    CTrafSampleEnhancer{trafBoxElement, sampleConfig};

    if (sampleGroupsConfig.prolConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() !=
        0) {
      CTrafSampleGroupsEnhancer{trafBoxElement, sampleGroupsConfig.prolConfig.boxesConfig,
                                defaultSampleGroupsFlag};
    }

    if (sampleGroupsConfig.rollConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() !=
        0) {
      CTrafSampleGroupsEnhancer{trafBoxElement, sampleGroupsConfig.rollConfig.boxesConfig,
                                defaultSampleGroupsFlag};
    }

    if (sampleGroupsConfig.sapConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.size() !=
        0) {
      CTrafSampleGroupsEnhancer{trafBoxElement, sampleGroupsConfig.sapConfig.boxesConfig,
                                defaultSampleGroupsFlag};
    }

    if (index < metaDataSamples.size() &&
        trafTreeConfig.tfhdConfig.trackId != metaDataSamples[index].trackId) {
      continue;
    }
  }

  // Get all samples of a fragment from the sample store
  auto storedSamples = m_sampleStore->storedSamples(0, fragConfig.mfhdConfig.sequenceNumber);
  box::CMediaDataBox::SMdatBoxWriteConfig mdatConfig;
  mdatConfig.payloadSize = storedSamples->size();
  nodefactory->createNode(*fragTree, mdatConfig);

  uint64_t treeSize = updateSizeAndReturnTotalSize(*fragTree);
  uint32_t treeSizeNoPayload = static_cast<uint32_t>(treeSize - storedSamples->size());
  updateTrunDataOffset(*fragTree, treeSizeNoPayload);
  ilo::ByteBuffer buff(static_cast<size_t>(treeSizeNoPayload));  // Hint: Exclude the mdat payload!
  ilo::ByteBuffer::iterator iter = buff.begin();
  ilo::ByteBuffer::const_iterator begConst = buff.begin();
  ilo::ByteBuffer::const_iterator endConst = buff.end();
  serializeTree(*fragTree, buff, iter);

  m_fragTrees.push_back(std::move(fragTree));

  ILO_ASSERT(m_output != nullptr, "Output module is a zero pointer");

  m_output->write(begConst, endConst);
  m_output->write(storedSamples->begin(), storedSamples->end());
}

uint32_t CIsobmffWriter::Pimpl::getFlagsFromSample(const CMetaSample& sample) {
  SSampleFlags flags;
  flags.isNonSyncSample = !sample.isSyncSample;
  return tools::sampleFlagsToValue(flags);
}

void CIsobmffWriter::Pimpl::finishNonFragmentedFile(const size_t maxChunkSize) {
  if (m_sampleStore->getStoreSize() == 0) {
    ILO_LOG_WARNING(
        "Isobmff writer was closed, but no samples where added. Nothing will be written");
    return;
  }
  MetaSampleVec sampleMetaDataVec;

  auto moovBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(*m_tree, ilo::toFcc("moov"));
  ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
  BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

  auto trakBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
  ILO_ASSERT(trakBoxElements.size() >= 1, "one or more trak boxes should be present");
  for (auto trakBoxElementRef : trakBoxElements) {
    BoxElement& trakBoxElement = const_cast<BoxElement&>(trakBoxElementRef.get());

    auto tkhdBoxElements = findAllElementsWithFourccAndBoxType<box::CTrackHeaderBox>(
        trakBoxElement, ilo::toFcc("tkhd"));
    BoxElement& tkhdBoxElement = const_cast<BoxElement&>(tkhdBoxElements[0].get());
    auto tkhdBox = std::dynamic_pointer_cast<box::CTrackHeaderBox>(tkhdBoxElement.item);

    auto stblBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("stbl"));
    ILO_ASSERT(stblBoxElements.size() == 1,
               "one and only one stbl box should be present for each trak");
    BoxElement& stblBoxElement = const_cast<BoxElement&>(stblBoxElements[0].get());

    sampleMetaDataVec = m_sampleStore->getSampleMetadata();
    STrakEnhancersConfig config(1, 0);
    fillTrakEnhancersConfigs(config, sampleMetaDataVec, tkhdBox->trackID());

    CTrakSampleEnhancer{stblBoxElement, config.trakSampleEnhancerConfig};
    if (config.sampleGroupsConfig.prolConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries
            .size() != 0) {
      CTrakSampleGroupsEnhancer{stblBoxElement, config.sampleGroupsConfig.prolConfig.boxesConfig};
    }

    if (config.sampleGroupsConfig.rollConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries
            .size() != 0) {
      CTrakSampleGroupsEnhancer{stblBoxElement, config.sampleGroupsConfig.rollConfig.boxesConfig};
    }

    if (config.sampleGroupsConfig.sapConfig.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries
            .size() != 0) {
      CTrakSampleGroupsEnhancer{stblBoxElement, config.sampleGroupsConfig.sapConfig.boxesConfig};
    }

    auto iter = m_editListMap.find(tkhdBox->trackID());
    if (iter != m_editListMap.end()) {
      CTrakEditListEnhancer{trakBoxElement, iter->second};
    }

    auto iter2 = m_userDataMap.find(tkhdBox->trackID());
    if (iter2 != m_userDataMap.end()) {
      CTrakUserDataEnhancer{trakBoxElement, iter2->second};
    }
  }
  // write file
  // Add the mdat box to the tree
  box::CMediaDataBox::SMdatBoxWriteConfig mdatConfig;
  mdatConfig.payloadSize = m_sampleStore->getStoreSize();

  {
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
    nodefactory->createNode(*m_tree, mdatConfig);
  }

  updateNextTrackId();
  updateDurationsInTree(sampleMetaDataVec);
  uint64_t treeSize = updateSizeAndReturnTotalSize(*m_tree);
  uint32_t treeSizeNoPayload = static_cast<uint32_t>(treeSize - m_sampleStore->getStoreSize());

  updateChunkOffsets(trakBoxElements, treeSizeNoPayload);

  ilo::ByteBuffer buff(static_cast<size_t>(treeSizeNoPayload));  // Hint: Exclude the mdat payload!
  auto iter = buff.begin();
  ilo::ByteBuffer::const_iterator begConst = buff.begin();
  ilo::ByteBuffer::const_iterator endConst = buff.end();
  serializeTree(*m_tree, buff, iter);
  ILO_ASSERT(buff.end() - iter == 0,
             "Serialized tree size is smaller than the pre-calculated buffer size for it.");
  m_output->write(begConst, endConst);

  while (m_sampleStore->getStoreSize() != 0) {
    // Fragment number has to be zero for non-fragmented mp4 files
    auto storedSamples = m_sampleStore->storedSamples(maxChunkSize, 0);
    m_output->write(storedSamples->begin(), storedSamples->end());
  }
}

void CIsobmffWriter::Pimpl::overwriteBaseMediaDecodeTime(uint32_t trackId, uint64_t newBmdtOffset) {
  m_baseMediaDecodeTime[trackId] = newBmdtOffset;
}

void CIsobmffWriter::Pimpl::updateNextTrackId() {
  auto mvhdBoxElements =
      findAllElementsWithFourccAndBoxType<box::CMovieHeaderBox>(*m_tree, ilo::toFcc("mvhd"));
  ILO_ASSERT(mvhdBoxElements.size() == 1, "one and only one mvhd box should be present");

  BoxElement& mvhdBoxElement = const_cast<BoxElement&>(mvhdBoxElements[0].get());
  auto mvhdBox = std::dynamic_pointer_cast<box::CMovieHeaderBox>(mvhdBoxElement.item);

  // Copy data of old box and set new nextTrackId
  box::CMovieHeaderBox::SMvhdBoxWriteConfig config = createMvhdConfig(mvhdBox);
  config.nextTrackID = m_nextTrackId;

  // Replace old mvhd box with new one
  auto nodefactory = CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
  nodefactory->replaceNode(mvhdBoxElement, box::CMovieHeaderBox::SMvhdBoxWriteConfig(config));
}

box::CTrackRunBox::STrunBoxWriteConfig CIsobmffWriter::Pimpl::createTrunConfig(
    const std::shared_ptr<box::CTrackRunBox>& trunBox) {
  box::CTrackRunBox::STrunBoxWriteConfig config;
  config.dataOffsetPresent = trunBox->dataOffsetPresent();
  if (trunBox->dataOffsetPresent()) {
    config.dataoffset = trunBox->dataOffset();
  }
  config.firstSampleFlagsPresent = trunBox->firstSampleFlagsPresent();
  if (trunBox->firstSampleFlagsPresent()) {
    config.firstSampleFlags = trunBox->firstSampleFlags();
  }
  config.sampleDurationPresent = trunBox->sampleDurationPresent();
  config.sampleSizePresent = trunBox->sampleSizePresent();
  config.sampleFlagsPresent = trunBox->sampleFlagsPresent();
  config.sampleCtsOffsetPresent = trunBox->sampleCtsOffsetPresent();
  config.sampleCount = trunBox->sampleCount();
  config.trunEntries = trunBox->trunEntries();

  return config;
}

box::CMovieHeaderBox::SMvhdBoxWriteConfig CIsobmffWriter::Pimpl::createMvhdConfig(
    const std::shared_ptr<box::CMovieHeaderBox>& mvhdBox) {
  box::CMovieHeaderBox::SMvhdBoxWriteConfig config;

  config.nextTrackID = mvhdBox->nextTrackID();
  config.creationTime = mvhdBox->creationTime();
  config.matrix = mvhdBox->matrix();
  config.modificationTime = mvhdBox->modificationTime();
  config.rate = mvhdBox->rate();
  config.timescale = mvhdBox->timescale();
  config.volume = mvhdBox->volume();
  config.creationTime = mvhdBox->creationTime();

  return config;
}

box::CMediaHeaderBox::SMdhdBoxWriteConfig CIsobmffWriter::Pimpl::createMdhdConfig(
    const std::shared_ptr<box::CMediaHeaderBox>& mdhdBox) {
  box::CMediaHeaderBox::SMdhdBoxWriteConfig mdhdConfig;

  mdhdConfig.language = mdhdBox->language();
  mdhdConfig.timescale = mdhdBox->timescale();
  mdhdConfig.modificationTime = mdhdBox->modificationTime();
  mdhdConfig.creationTime = mdhdBox->creationTime();

  return mdhdConfig;
}

box::CTrackHeaderBox::STkhdBoxWriteConfig CIsobmffWriter::Pimpl::createTkhdConfig(
    const std::shared_ptr<box::CTrackHeaderBox>& tkhdBox) {
  box::CTrackHeaderBox::STkhdBoxWriteConfig tkhdConfig;

  tkhdConfig.alternateGroup = tkhdBox->alternateGroup();
  tkhdConfig.creationTime = tkhdBox->creationTime();
  tkhdConfig.height = tkhdBox->height();
  tkhdConfig.layer = tkhdBox->layer();
  tkhdConfig.matrix = tkhdBox->matrix();
  tkhdConfig.modificationTime = tkhdBox->modificationTime();
  tkhdConfig.trackID = tkhdBox->trackID();
  tkhdConfig.width = tkhdBox->width();
  tkhdConfig.volume = tkhdBox->volume();
  tkhdConfig.trackIsEnabled = tkhdBox->isEnabled();
  tkhdConfig.trackInMovie = tkhdBox->inMovie();
  tkhdConfig.trackInPreview = tkhdBox->inPreview();
  tkhdConfig.trackSizeIsAspectRatio = tkhdBox->sizeIsAspectRatio();

  return tkhdConfig;
}

box::CObjectDescriptorBox::SIodsBoxWriteConfig CIsobmffWriter::Pimpl::createIodsConfig(
    const std::shared_ptr<box::CObjectDescriptorBox>& iodsBox) {
  box::CObjectDescriptorBox::SIodsBoxWriteConfig config;

  config.audioProfileLevelIndication = iodsBox->audioProfileLevelIndication();

  return config;
}

void CIsobmffWriter::Pimpl::updateTrunDataOffset(BoxTree::NodeType& subTree,
                                                 const uint32_t& dataOffset) {
  auto trunBoxElements =
      findAllElementsWithFourccAndBoxType<box::CTrackRunBox>(subTree, ilo::toFcc("trun"));

  for (auto& trunBoxElementPointer : trunBoxElements) {
    BoxElement& trunBoxElement = const_cast<BoxElement&>(trunBoxElementPointer.get());
    auto trunBox = std::dynamic_pointer_cast<box::CTrackRunBox>(trunBoxElement.item);

    // Create config from existing trun box
    auto trunConfig = createTrunConfig(trunBox);

    // Add the offset
    trunConfig.dataOffsetPresent = true;
    trunConfig.dataoffset = dataOffset;

    // Replace old trun box with new one
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
    nodefactory->replaceNode(trunBoxElement, box::CTrackRunBox::STrunBoxWriteConfig(trunConfig));
  }
}

void CIsobmffWriter::Pimpl::updateChunkOffsets(
    const std::vector<std::reference_wrapper<const BoxElement>>& trakBoxElements,
    const uint32_t& offset) {
  for (auto trakBoxElementRef : trakBoxElements) {
    BoxElement& trakBoxElement = const_cast<BoxElement&>(trakBoxElementRef.get());

    auto stblBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("stbl"));
    ILO_ASSERT(stblBoxElements.size() == 1,
               "one and only one stbl box should be present for each trak");
    BoxElement& stblBoxElement = const_cast<BoxElement&>(stblBoxElements[0].get());

    auto stcoBoxElements = findAllElementsWithFourccAndBoxType<box::CChunkOffsetBox>(
        stblBoxElement, ilo::toFcc("stco"));
    auto co64BoxElements = findAllElementsWithFourccAndBoxType<box::CChunkOffset64Box>(
        stblBoxElement, ilo::toFcc("co64"));
    ILO_ASSERT((stcoBoxElements.size() >= 1 && co64BoxElements.size() == 0) ||
                   (co64BoxElements.size() >= 1 && stcoBoxElements.size() == 0),
               "only one box of either stco or co64 should be present");
    if (stcoBoxElements.size() == 0) {
      BoxElement& co64BoxElement = const_cast<BoxElement&>(co64BoxElements[0].get());
      auto co64Box = std::dynamic_pointer_cast<box::CChunkOffset64Box>(co64BoxElement.item);
      ILO_ASSERT(co64Box != nullptr, "Casting to CChunkOffset64Box failed, wrong type.");

      // Create config from existing co64 box
      auto co64Config = box::CChunkOffset64Box::SCo64BoxWriteConfig();
      co64Config.chunkOffsets = co64Box->chunkOffsets();

      // Modify the offset
      std::for_each(co64Config.chunkOffsets.begin(), co64Config.chunkOffsets.end(),
                    [&offset](uint64_t& chunkOffset) { chunkOffset += offset; });

      // Replace old co64 box with new one
      auto nodefactory =
          CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
      nodefactory->replaceNode(co64BoxElement,
                               box::CChunkOffset64Box::SCo64BoxWriteConfig(co64Config));
    } else {
      BoxElement& stcoBoxElement = const_cast<BoxElement&>(stcoBoxElements[0].get());
      auto stcoBox = std::dynamic_pointer_cast<box::CChunkOffsetBox>(stcoBoxElement.item);
      ILO_ASSERT(stcoBox != nullptr, "Casting to CChunkOffsetBox failed, wrong type.");

      // Create config from existing stco box
      auto stcoConfig = box::CChunkOffsetBox::SStcoBoxWriteConfig();
      stcoConfig.chunkOffsets = stcoBox->chunkOffsets();

      // Modify the offset
      std::for_each(stcoConfig.chunkOffsets.begin(), stcoConfig.chunkOffsets.end(),
                    [&offset](uint32_t& chunkOffset) { chunkOffset += offset; });

      // Replace old co64 box with new one
      auto nodefactory =
          CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
      nodefactory->replaceNode(stcoBoxElement,
                               box::CChunkOffsetBox::SStcoBoxWriteConfig(stcoConfig));
    }
  }
}

void CIsobmffWriter::Pimpl::updateStscBox(STrakSampleEnhancerConfig& config,
                                          const uint32_t& samplesPerChunk,
                                          const bool& largeOffsets) {
  // An entry in the SampleToChunk box is created for every chunk of samples in the interleaved
  // samplestore that has a different amount of samples than the last entry stored in the vector
  // of entries in the box.  We determine a new chunk in the samplestore by checking the trackId
  // of the current sample. If it is different than the trackId from the function parameters,
  // then this means that we have a new chunk.
  if (samplesPerChunk != 0 &&
      (config.stscConfig.entries.empty() ||
       config.stscConfig.entries.back().samples_per_chunk != samplesPerChunk)) {
    box::CSampleToChunkBox::SStscEntry stscEntry;
    // NB: Currently we only support one sample entry per track. Thus the sample_description_index
    // is always 1
    stscEntry.sample_description_index = 1;

    if (!largeOffsets) {
      stscEntry.first_chunk = static_cast<uint32_t>(config.stcoConfig.chunkOffsets.size());
    } else {
      stscEntry.first_chunk = static_cast<uint32_t>(config.co64Config.chunkOffsets.size());
    }
    stscEntry.samples_per_chunk = samplesPerChunk;
    config.stscConfig.entries.push_back(stscEntry);
  }
}

void CIsobmffWriter::Pimpl::updateSttsBox(STrakSampleEnhancerConfig& config,
                                          box::CDecodingTimeToSampleBox::SSttsEntry& sttsEntry,
                                          uint32_t currentDuration) {
  // Check if duration of this sample is different than that of previous sample.
  // In this case create a new entry in the stts box
  if (sttsEntry.sampleCount > 0 && (currentDuration != sttsEntry.sampleDelta)) {
    config.sttsConfig.entries.push_back(sttsEntry);
    sttsEntry.sampleCount = 1;
  } else {
    sttsEntry.sampleCount++;
  }
  sttsEntry.sampleDelta = static_cast<uint32_t>(currentDuration);
}

void CIsobmffWriter::Pimpl::updateDurationsInTree(const MetaSampleVec& sampleMetaDataVec) {
  std::map<uint32_t, uint64_t> tracksDuration;

  for (auto& sample : sampleMetaDataVec) {
    tracksDuration[sample.trackId] += sample.duration;
  }

  auto it = std::max_element(
      tracksDuration.begin(), tracksDuration.end(),
      [](const std::pair<uint32_t, uint64_t>& p1, const std::pair<uint32_t, uint64_t>& p2) {
        return p1.second < p2.second;
      });
  auto longestTrack = it->second;
  uint64_t longestTrackTimescale = 0;

  auto moovBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(*m_tree, ilo::toFcc("moov"));
  ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
  BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

  auto mvhdBoxElements =
      findAllElementsWithFourccAndBoxType<box::CMovieHeaderBox>(*m_tree, ilo::toFcc("mvhd"));
  ILO_ASSERT(mvhdBoxElements.size() == 1, "one and only one mvhd box should be present");

  BoxElement& mvhdBoxElement = const_cast<BoxElement&>(mvhdBoxElements[0].get());
  auto mvhdBox = std::dynamic_pointer_cast<box::CMovieHeaderBox>(mvhdBoxElement.item);

  auto trakBoxElements =
      findAllElementsWithFourccAndBoxType<box::CContainerBox>(moovBoxElement, ilo::toFcc("trak"));
  ILO_ASSERT(trakBoxElements.size() >= 1, "one or more trak boxes should be present");

  auto nodefactory = CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();

  for (auto trakBoxElementRef : trakBoxElements) {
    BoxElement& trakBoxElement = const_cast<BoxElement&>(trakBoxElementRef.get());

    auto edtsBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("edts"));
    ILO_ASSERT(edtsBoxElements.size() <= 1, "zero or one edts box should be present");

    std::shared_ptr<box::CEditListBox> elstBox;
    if (edtsBoxElements.size() == 1) {
      BoxElement& edtsBoxElement = const_cast<BoxElement&>(edtsBoxElements[0].get());
      auto elstBoxElements = findAllElementsWithFourccAndBoxType<box::CEditListBox>(
          edtsBoxElement, ilo::toFcc("elst"));
      BoxElement& elstBoxElement = const_cast<BoxElement&>(elstBoxElements[0].get());
      elstBox = std::dynamic_pointer_cast<box::CEditListBox>(elstBoxElement.item);
    }

    auto tkhdBoxElements = findAllElementsWithFourccAndBoxType<box::CTrackHeaderBox>(
        trakBoxElement, ilo::toFcc("tkhd"));
    BoxElement& tkhdBoxElement = const_cast<BoxElement&>(tkhdBoxElements[0].get());
    auto tkhdBox = std::dynamic_pointer_cast<box::CTrackHeaderBox>(tkhdBoxElement.item);

    auto mdiaBoxElements =
        findAllElementsWithFourccAndBoxType<box::CContainerBox>(trakBoxElement, ilo::toFcc("mdia"));
    ILO_ASSERT(mdiaBoxElements.size() == 1,
               "one and only one mdia box should be present for each trak");

    auto mdhdBoxElements = findAllElementsWithFourccAndBoxType<box::CMediaHeaderBox>(
        trakBoxElement, ilo::toFcc("mdhd"));
    BoxElement& mdhdBoxElement = const_cast<BoxElement&>(mdhdBoxElements[0].get());
    auto mdhdBox = std::dynamic_pointer_cast<box::CMediaHeaderBox>(mdhdBoxElement.item);

    // Create tkhd config from existing box and set new duration
    auto tkhdConfig = createTkhdConfig(tkhdBox);

    if (elstBox) {
      for (auto& entry : elstBox->entries()) {
        tkhdConfig.duration += entry.segmentDuration;
      }
    } else {
      // Sample durations are already in track timesale and in this case they should be in the
      // timescale of the mvhd box
      tkhdConfig.duration = static_cast<uint64_t>(std::floor(
          tracksDuration.at(tkhdBox->trackID()) * mvhdBox->timescale() / mdhdBox->timescale()));
    }

    // Replace old tkhd box with the new one
    nodefactory->replaceNode(tkhdBoxElement, box::CTrackHeaderBox::STkhdBoxWriteConfig(tkhdConfig));

    // Create mdhd config from existing box and set new duration
    auto mdhdConfig = createMdhdConfig(mdhdBox);
    mdhdConfig.duration = tracksDuration[tkhdBox->trackID()];

    // Replace old mdhd box with the new one
    nodefactory->replaceNode(mdhdBoxElement, box::CMediaHeaderBox::SMdhdBoxWriteConfig(mdhdConfig));

    // Store the timescale of the longest track
    if (tracksDuration.at(tkhdBox->trackID()) == longestTrack) {
      if (elstBox) {
        longestTrack = tkhdConfig.duration;
        longestTrackTimescale = mvhdBox->timescale();
      } else {
        longestTrackTimescale = static_cast<uint64_t>(mdhdBox->timescale());
      }
    }
  }

  // Create mvhd config from existing box and set new duration
  auto mvhdConfig = createMvhdConfig(mvhdBox);
  mvhdConfig.duration = static_cast<uint64_t>(
      std::floor(longestTrack * mvhdBox->timescale() / longestTrackTimescale));

  // Replace old mdhd box with the new one
  nodefactory->replaceNode(mvhdBoxElement, box::CMovieHeaderBox::SMvhdBoxWriteConfig(mvhdConfig));
}

void CIsobmffWriter::Pimpl::updateSampleGroupsConfig(SSampleGroupsConfig& config,
                                                     const CMetaSample& metaSample) {
  config.sampleGroupInfoNew = metaSample.sampleGroupInfo;

  // if there is a change in the rollDistance, sampleGroup or sapType
  // compared to last sample, change is to be made on the rollGroup boxes
  if (config.sampleGroupInfoOld != config.sampleGroupInfoNew) {
    updateEntries(config);
  } else {
    incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
    incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
    incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
  }
}

void CIsobmffWriter::Pimpl::updateEntries(SSampleGroupsConfig& config) {
  if (config.sampleGroupInfoNew.type == SampleGroupType::roll) {
    updateRollDistances<CAudioRollRecoveryEntry>(config.rollConfig,
                                                 config.sampleGroupInfoNew.rollDistance);

    if (config.sampleGroupInfoOld.type == SampleGroupType::prol) {
      config.prolConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
    } else if (config.sampleGroupInfoOld.type == SampleGroupType::sap) {
      config.sapConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
    } else {
      incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
      incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
    }
  } else if (config.sampleGroupInfoNew.type == SampleGroupType::prol) {
    updateRollDistances<CAudioPreRollEntry>(config.prolConfig,
                                            config.sampleGroupInfoNew.rollDistance);

    if (config.sampleGroupInfoOld.type == SampleGroupType::roll) {
      config.rollConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
    } else if (config.sampleGroupInfoOld.type == SampleGroupType::sap) {
      config.sapConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
    } else {
      incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
      incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
    }
  } else if (config.sampleGroupInfoNew.type == SampleGroupType::sap) {
    updateSapType(config.sapConfig, config.sampleGroupInfoNew.sapType);

    if (config.sampleGroupInfoOld.type == SampleGroupType::roll) {
      config.rollConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
    } else if (config.sampleGroupInfoOld.type == SampleGroupType::prol) {
      config.prolConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
    } else {
      incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
      incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
    }
  } else {
    if (config.sampleGroupInfoOld.type == SampleGroupType::roll) {
      config.rollConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      config.sapConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
      config.prolConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
    } else if (config.sampleGroupInfoOld.type == SampleGroupType::prol) {
      config.prolConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      config.rollConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
      config.sapConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
    } else if (config.sampleGroupInfoOld.type == SampleGroupType::sap) {
      config.sapConfig.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
          box::CSampleToGroupBox::SSampleGroupEntry(1, config.groupDescriptionIndexStart));
      config.prolConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
      config.rollConfig.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
    } else {
      incrementSampleCount(config.prolConfig, config.groupDescriptionIndexStart);
      incrementSampleCount(config.rollConfig, config.groupDescriptionIndexStart);
      incrementSampleCount(config.sapConfig, config.groupDescriptionIndexStart);
    }
  }

  config.sampleGroupInfoOld = config.sampleGroupInfoNew;
}

void CIsobmffWriter::Pimpl::incrementSampleCount(SGroupingTypeSpecificConfig& config,
                                                 uint32_t groupDescriptionIndexStart) {
  if (!config.boxesConfig.sbgpConfig.sampleGroupEntries.empty()) {
    config.boxesConfig.sbgpConfig.sampleGroupEntries.back().sampleCount++;
  } else {
    box::CSampleToGroupBox::SSampleGroupEntry sampleGroupEntry(1, groupDescriptionIndexStart);
    config.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(sampleGroupEntry);
  }
}

template <class T>
void CIsobmffWriter::Pimpl::updateRollDistances(SGroupingTypeSpecificConfig& config,
                                                int16_t rollDistance) {
  box::CSampleGroupDescriptionBox::SSampleGroupDescriptionEntry sampleGroupDescriptionEntry;
  if (config.rollDistances[rollDistance] == 0) {
    config.rollDistances[rollDistance] = static_cast<uint32_t>(++config.lastGroupDescIndex);
    sampleGroupDescriptionEntry.sampleGroupEntry = std::make_shared<T>(rollDistance);
    config.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.push_back(
        sampleGroupDescriptionEntry);
  }
  config.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
      box::CSampleToGroupBox::SSampleGroupEntry(1, config.rollDistances[rollDistance]));
}

void CIsobmffWriter::Pimpl::updateSapType(SGroupingTypeSpecificConfig& config, uint8_t sapType) {
  box::CSampleGroupDescriptionBox::SSampleGroupDescriptionEntry sampleGroupDescriptionEntry;
  if (config.sapTypes[sapType] == 0) {
    config.sapTypes[sapType] = static_cast<uint32_t>(++config.lastGroupDescIndex);
    sampleGroupDescriptionEntry.sampleGroupEntry = std::make_shared<CSAPEntry>(sapType);
    config.boxesConfig.sgpdConfig.sampleGroupDescriptionEntries.push_back(
        sampleGroupDescriptionEntry);
  }
  config.boxesConfig.sbgpConfig.sampleGroupEntries.push_back(
      box::CSampleToGroupBox::SSampleGroupEntry(1, config.sapTypes[sapType]));
}

void CIsobmffWriter::Pimpl::cleanTempFiles() {
  if (m_tmpFileName.empty()) {
    return;
  }

  closeAllOutputs();

  // Try to delete the file. In case of an EACCES error (something blocks the deletion call)
  // try again a few times.
  uint32_t retryCount = 10;
  uint32_t sleepDuration = 100;  // ms
  for (uint32_t retries = 1; retries <= retryCount; ++retries) {
    auto res = remove(m_tmpFileName.c_str());
    if (res != 0) {
#if defined(WIN32) || defined(_WIN32)
      const uint32_t BUFFER_SIZE = 256;
      char buffer[BUFFER_SIZE] = {0};

      strerror_s(&buffer[0], BUFFER_SIZE, errno);
      std::string error(buffer);
      ILO_LOG_WARNING("Could not delete tempfile %s. Error is: %s", m_tmpFileName.c_str(),
                      error.c_str());
#else
      ILO_LOG_WARNING("Could not delete tempfile %s.", m_tmpFileName.c_str());
#endif
      if (errno != EACCES) {
        break;
      }
      ILO_LOG_WARNING("Retrying file deletion of %s ... (%d/%d)", m_tmpFileName.c_str(), retries,
                      retryCount);
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration));
    }
  }
  m_tmpFileName.clear();
}
}  // namespace isobmff
}  // namespace mmt
