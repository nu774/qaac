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

#pragma once

// System includes
#include <memory>

// External includes

// Internal includes
#include "mmtisobmff/writer/output.h"
#include "common/tracksampleinfo.h"
#include "tree/boxtree.h"
#include "writer/sample_store.h"
#include "writer/trak_sample_enhancer.h"
#include "writer/traf_samplegroups_enhancer.h"
#include "box/trunbox.h"
#include "box/mvhdbox.h"
#include "box/mdhdbox.h"
#include "box/tkhdbox.h"
#include "box/iodsbox.h"

namespace mmt {
namespace isobmff {
struct CIsobmffWriter::Pimpl {
  static const size_t MAX_CHUNK_SIZE_IN_BYTES = 200000000;

  struct SFragmentData {
    uint32_t trackId = 0;
    std::vector<CMetaSample> metaDataSamples;
  };

  struct SPimplConfig {
    std::unique_ptr<IIsobmffOutput> out = nullptr;
    std::unique_ptr<IIsobmffOutput> tmpOut = nullptr;
    std::unique_ptr<BoxTree> tree = nullptr;
    std::unique_ptr<CSampleStore> sampleStore = nullptr;
    uint64_t timeNowUtc = 0;
    bool hasFragments = false;
    bool forceTfdtV1 = false;
    bool writeSidx = false;
    bool writeIods = false;
    ESapType sapType = ESapType::SapTypeInvalid;
    uint64_t chunkSize = 0;
    std::string tmpFileName;  // Helps tmp file cleanup
  };

  struct SGroupingTypeSpecificConfig {
    SGroupingTypeSpecificConfig(const uint8_t& sgpdBoxVersion, const uint8_t& sbgpBoxVersion)
        : boxesConfig(sgpdBoxVersion, sbgpBoxVersion) {}

    std::map<int16_t, uint32_t> rollDistances;
    std::map<uint8_t, uint32_t> sapTypes;
    uint32_t sampleCountSum = 0;
    SSampleGroupsEnhancerConfig boxesConfig;
    uint32_t lastGroupDescIndex = 0;
  };

  struct SSampleGroupsConfig {
    SSampleGroupsConfig(const uint8_t& sgpdBoxVersion, const uint8_t& sbgpBoxVersion,
                        bool isFragmented = false)
        : rollConfig(sgpdBoxVersion, sbgpBoxVersion),
          prolConfig(sgpdBoxVersion, sbgpBoxVersion),
          sapConfig(sgpdBoxVersion, sbgpBoxVersion),
          groupDescriptionIndexStart(isFragmented ? 0x10000 : 0x0000) {
      fillDefaultConfig();
    }

    SSampleGroupsConfig& operator=(const SSampleGroupsConfig&) = delete;
    SSampleGroupsConfig(const SSampleGroupsConfig&) = delete;

    SSampleGroupInfo sampleGroupInfoOld;
    SSampleGroupInfo sampleGroupInfoNew;

    SGroupingTypeSpecificConfig rollConfig;
    SGroupingTypeSpecificConfig prolConfig;
    SGroupingTypeSpecificConfig sapConfig;
    const uint32_t groupDescriptionIndexStart;

   private:
    void fillDefaultConfig() {
      rollConfig.boxesConfig.sbgpConfig.groupingType = ilo::toFcc("roll");
      rollConfig.boxesConfig.sgpdConfig.groupingType = ilo::toFcc("roll");
      rollConfig.boxesConfig.sgpdConfig.defaultLength = 2;
      rollConfig.lastGroupDescIndex = groupDescriptionIndexStart;
      prolConfig.boxesConfig.sbgpConfig.groupingType = ilo::toFcc("prol");
      prolConfig.boxesConfig.sgpdConfig.groupingType = ilo::toFcc("prol");
      prolConfig.boxesConfig.sgpdConfig.defaultLength = 2;
      prolConfig.lastGroupDescIndex = groupDescriptionIndexStart;
      sapConfig.boxesConfig.sbgpConfig.groupingType = ilo::toFcc("sap ");
      sapConfig.boxesConfig.sgpdConfig.groupingType = ilo::toFcc("sap ");
      sapConfig.boxesConfig.sgpdConfig.defaultLength = 1;
      sapConfig.lastGroupDescIndex = groupDescriptionIndexStart;
    }
  };

  struct STrakEnhancersConfig {
    STrakEnhancersConfig(const uint8_t& sgpdBoxVersion, const uint8_t& sbgpBoxVersion)
        : sampleGroupsConfig(sgpdBoxVersion, sbgpBoxVersion) {}

    SSampleGroupsConfig sampleGroupsConfig;
    STrakSampleEnhancerConfig trakSampleEnhancerConfig;
  };

  Pimpl(SPimplConfig& config)
      : m_tree(std::move(config.tree)),
        m_timeNowUtc(config.timeNowUtc),
        m_sampleStore(std::move(config.sampleStore)),
        m_hasFragments(config.hasFragments),
        m_forceTfdtV1(config.forceTfdtV1),
        m_writeSidx(config.writeSidx),
        m_writeIods(config.writeIods),
        m_sapType(config.sapType),
        m_chunkSize(config.chunkSize),
        m_output(std::move(config.out)),
        m_tmpOutput(std::move(config.tmpOut)),
        m_tmpFileName(config.tmpFileName) {}

  ~Pimpl() { cleanTempFiles(); }

  const std::unique_ptr<IIsobmffOutput>& output() const;

  void closeCurrentOutput();
  void closeAllOutputs();

  // Helper function to fill generic moov boxes that are trak related and
  // special for fragmented/nonFragmented mp4 files
  void fillStaticMoovInfo();

  // Helper function to extract the info from the metaSamples and fill the
  // config for sample groups related boxes (Used for nonFragmented mp4 files)
  void fillTrakEnhancersConfigs(STrakEnhancersConfig& config,
                                const MetaSampleVec& sampleMetaDataVec, const uint32_t& trackId);

  // Helper function to create an mvex box
  void createMvexBox();

  // Helper function to create a styp box based on the content of the ftyp box.
  // If isLastSegment is set, then the 'lmsg' compattibility brand is added
  void createStypBox(ilo::ByteBuffer& stypBuff, const bool& isLastSegment);

  // Helper function to create an sidx box. This is only used for fragmented
  // file writing
  void createSidxBox(ilo::ByteBuffer& sidxBuff);

  // Helper function to add the sidx box and finish the fragmented file. This
  // is only used for fragmented file writing
  void addSidxBox(const size_t maxChunkSize = MAX_CHUNK_SIZE_IN_BYTES);

  // Creates e.g. ftyp + moov
  void createInitFragment(std::unique_ptr<IIsobmffOutput>&& output);

  // Creates fragments out of all available samples
  void createFragments(std::unique_ptr<IIsobmffOutput>&& output);

  // Creates moof + mdat
  void createFragment(const std::vector<CMetaSample>& metaDataSamples);

  // Finishes a non fragmented file by writing the tree and copying the samples from sample store.
  // maxChunkSize is the max number of bytes that are read at once from the sample store
  void finishNonFragmentedFile(const size_t maxChunkSize = MAX_CHUNK_SIZE_IN_BYTES);

  // Warning! Advanced use-case! Do not use for normal mp4 operation modes!
  // Function to overwrite the base media decode time
  void overwriteBaseMediaDecodeTime(uint32_t trackId, uint64_t newBmdtOffset);

  std::unique_ptr<BoxTree> m_tree = nullptr;
  std::vector<std::unique_ptr<BoxTree>> m_fragTrees;
  uint64_t m_timeNowUtc = 0;
  std::unique_ptr<CSampleStore> m_sampleStore = nullptr;
  bool m_hasFragments = false;
  bool m_forceTfdtV1 = false;
  bool m_writeSidx = false;
  bool m_writeIods = false;
  ESapType m_sapType = ESapType::SapTypeInvalid;
  uint64_t m_chunkSize = 0;
  uint32_t m_lastFragmentNumber = 1;
  uint32_t m_nextTrackId = 1;
  std::map<uint32_t, uint64_t> m_baseMediaDecodeTime;
  bool m_initWritten = false;
  bool m_closeCalled = false;
  bool m_memoryMp4SerializationCalled = false;
  std::map<uint32_t, SEditList> m_editListMap;
  std::map<uint32_t, std::vector<ilo::ByteBuffer>> m_userDataMap;
  std::vector<uint32_t> m_mp4aTrackIds;
  std::map<uint32_t /* trackID */, std::unique_ptr<SSampleGroupInfo>> m_defaultSampleGroupInfoMap;

 private:
  // Helper function to update nextTrackId in mvhd
  void updateNextTrackId();
  box::CTrackRunBox::STrunBoxWriteConfig createTrunConfig(
      const std::shared_ptr<box::CTrackRunBox>& trunBox);
  box::CMovieHeaderBox::SMvhdBoxWriteConfig createMvhdConfig(
      const std::shared_ptr<box::CMovieHeaderBox>& mvhdBox);
  box::CMediaHeaderBox::SMdhdBoxWriteConfig createMdhdConfig(
      const std::shared_ptr<box::CMediaHeaderBox>& mdhdBox);
  box::CTrackHeaderBox::STkhdBoxWriteConfig createTkhdConfig(
      const std::shared_ptr<box::CTrackHeaderBox>& tkhdBox);
  box::CObjectDescriptorBox::SIodsBoxWriteConfig createIodsConfig(
      const std::shared_ptr<box::CObjectDescriptorBox>& iodsBox);
  void updateTrunDataOffset(BoxTree::NodeType& subTree, const uint32_t& dataOffset);
  void updateChunkOffsets(
      const std::vector<std::reference_wrapper<const BoxElement>>& trakBoxElements,
      const uint32_t& offset);

  // Helper functions to fill the mentioned boxes.
  void updateStscBox(STrakSampleEnhancerConfig& config, const uint32_t& samplesPerChunk,
                     const bool& largeOffsets);
  void updateSttsBox(STrakSampleEnhancerConfig& config,
                     box::CDecodingTimeToSampleBox::SSttsEntry& sttsEntry,
                     uint32_t currentDuration);

  // Helper function to update the durations in following boxes: mvhd, tkhd, mdhd
  void updateDurationsInTree(const MetaSampleVec& sampleMetaDataVec);

  // Extracts the sample groups related info from the metadata and updates the roll groups
  // boxes config
  void updateSampleGroupsConfig(SSampleGroupsConfig& config, const CMetaSample& metaSample);

  // updates the sample entries in the sbgp and sgpd boxes
  void updateEntries(SSampleGroupsConfig& config);

  // Increment sample count
  void incrementSampleCount(SGroupingTypeSpecificConfig& groupingTypeSpecificConfig,
                            uint32_t groupDescriptionIndexStart);

  // Update sgpd and sbgp boxes with the new rollDistance
  template <class T>
  void updateRollDistances(SGroupingTypeSpecificConfig& config, const int16_t rollDistance);
  // Update sgpd and sbgp boxes with the new sapType
  void updateSapType(SGroupingTypeSpecificConfig& config, uint8_t sapType);

  // Function to clean up temp files. Only call in destructor!
  void cleanTempFiles();

  // Generate the sample flags from a sample
  uint32_t getFlagsFromSample(const CMetaSample& sample);

  std::unique_ptr<IIsobmffOutput> m_output = nullptr;
  std::unique_ptr<IIsobmffOutput> m_tmpOutput = nullptr;
  std::string m_tmpFileName;
};
}  // namespace isobmff
}  // namespace mmt
