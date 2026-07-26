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
 * Content: track writer class
 */

// System includes
#include <memory>
#include <cmath>
#include <type_traits>

// External includes
#include "ilo/memory.h"

// Internal includes
#include "mmtisobmff/writer/trackwriter.h"
#include "mmtisobmff/helper/commonhelpertools.h"
#include "mmtisobmff/helper/videohelpertools.h"
#include "common/logging.h"
#include "writer/trak_tree_enhancer.h"
#include "writer/mpegh_tree_enhancer.h"
#include "writer/mp4a_tree_enhancer.h"
#include "writer/avc_tree_enhancer.h"
#include "writer/hevc_tree_enhancer.h"
#include "writer/jxs_tree_enhancer.h"
#include "writer/vvc_tree_enhancer.h"
#include "writer/trak_sample_enhancer.h"
#include "writer/mediafragment_tree_builder.h"
#include "writer/traf_tree_enhancer.h"
#include "writer/traf_sample_enhancer.h"
#include "writer/writerpimpl.h"
#include "tree/boxtree.h"
#include "service/factory.h"
#include "service/servicesingleton.h"
#include "box/sampleentry.h"
#include "box/containerbox.h"
#include "box/stsdbox.h"

namespace mmt {
namespace isobmff {

/* ######---Basic Track Writer---###### */
struct CTrackWriter::SPimpl {
  SPimpl(const SBaseAudioConfig& config) {
    m_enhancerConfig.mdhdConfig.language = config.language;
    m_enhancerConfig.hdlrConfig.handlerType = ilo::toFcc("soun");
    m_enhancerConfig.hdlrConfig.name = "soun";
  };

  SPimpl(const SBaseVideoConfig& config) {
    m_enhancerConfig.hdlrConfig.handlerType = ilo::toFcc("vide");
    m_enhancerConfig.hdlrConfig.name = "VideoHandler";
    m_enhancerConfig.tkhdConfig.width = config.width;
    m_enhancerConfig.tkhdConfig.height = config.height;
  };

  std::reference_wrapper<BoxElement> createTrack() {
    auto wPimpl = wP.lock();
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();

    auto moovBoxElements = findAllElementsWithFourccAndBoxType<box::CContainerBox>(
        *(wPimpl->m_tree), ilo::toFcc("moov"));
    ILO_ASSERT(moovBoxElements.size() == 1, "one and only one moov box should be present");
    BoxElement& moovBoxElement = const_cast<BoxElement&>(moovBoxElements[0].get());

    auto trakBoxElement = nodefactory->createNode(
        moovBoxElement, box::CContainerBox::SContainerBoxWriteConfig(ilo::toFcc("trak")));
    CTrakTreeEnhancer{trakBoxElement, m_enhancerConfig};

    if (wPimpl->m_hasFragments) {
      auto stblBoxElements = findAllElementsWithFourccAndBoxType<box::CContainerBox>(
          trakBoxElement, ilo::toFcc("stbl"));
      ILO_ASSERT(stblBoxElements.size() == 1,
                 "one and only one stbl box should be present for each trak");
      BoxElement& stblBoxElement = const_cast<BoxElement&>(stblBoxElements[0].get());

      CTrakSampleEnhancer{stblBoxElement};
    }

    return trakBoxElement;
  }

  uint32_t m_trackId = 0;
  STrakTreeEnhancerConfig m_enhancerConfig{0};
  std::weak_ptr<CIsobmffWriter::Pimpl> wP;
};

template <typename TConfig>
CTrackWriter::CTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl, const TConfig& config)
    : m_pimpl(new SPimpl(config)) {
  ILO_ASSERT_WITH(config.codingName != ilo::toFcc("0000"), std::invalid_argument,
                  "mandatory codingName was not set");
  m_pimpl->wP = writerPimpl;

  auto wPimpl = writerPimpl.lock();
  if (wPimpl->m_writeIods) {
    ILO_ASSERT_WITH(config.codingName == ilo::toFcc("mp4a"), std::invalid_argument,
                    "the iods box can only be written for mp4a");
  }

  m_pimpl->m_enhancerConfig.tkhdConfig.creationTime = wPimpl->m_timeNowUtc;
  m_pimpl->m_enhancerConfig.tkhdConfig.modificationTime = wPimpl->m_timeNowUtc;

  if (config.trackID != 0) {
    m_pimpl->m_enhancerConfig.tkhdConfig.trackID = config.trackID;
    wPimpl->m_nextTrackId = config.trackID + 1;
  } else {
    m_pimpl->m_enhancerConfig.tkhdConfig.trackID = wPimpl->m_nextTrackId++;
  }

  m_pimpl->m_trackId = m_pimpl->m_enhancerConfig.tkhdConfig.trackID;
  m_pimpl->m_enhancerConfig.mdhdConfig.creationTime = wPimpl->m_timeNowUtc;
  m_pimpl->m_enhancerConfig.mdhdConfig.modificationTime = wPimpl->m_timeNowUtc;
  m_pimpl->m_enhancerConfig.mdhdConfig.timescale = config.mediaTimescale;
  m_pimpl->m_enhancerConfig.stsdConfig.entryCount = 1;
  m_pimpl->m_enhancerConfig.drefConfig.entryCount = 1;

  if (config.defaultSampleGroup.type != SampleGroupType::undefined &&
      config.defaultSampleGroup.type != SampleGroupType::none) {
    wPimpl->m_defaultSampleGroupInfoMap[m_pimpl->m_trackId] =
        ilo::make_unique<SSampleGroupInfo>(config.defaultSampleGroup);
  }
}

CTrackWriter::~CTrackWriter() = default;

void CTrackWriter::addSample(const CSample& sample) {
  auto wPimpl = m_pimpl->wP.lock();
  ILO_ASSERT(wPimpl != nullptr, "writer has not been initialized");

  if (!wPimpl->m_hasFragments) {
    ILO_ASSERT(sample.fragmentNumber == 0,
               "fragment number for non-fragmented mp4 file has to be 0");
  } else {
    ILO_ASSERT(wPimpl->m_lastFragmentNumber <= sample.fragmentNumber,
               "fragment number cannot decrease");
    wPimpl->m_lastFragmentNumber = sample.fragmentNumber;
  }

  wPimpl->m_sampleStore->addSample(sample, m_pimpl->m_trackId,
                                   m_pimpl->m_enhancerConfig.mdhdConfig.timescale);
}

void CTrackWriter::addEditListEntry(const SEdit& entry) {
  auto wPimpl = m_pimpl->wP.lock();
  ILO_ASSERT(wPimpl != nullptr, "writer has not been initialized");

  ILO_ASSERT(!wPimpl->m_hasFragments,
             "Writing edit lists is currently not supported for fragmented mp4 files");

  wPimpl->m_editListMap[m_pimpl->m_trackId].push_back(entry);
}

void CTrackWriter::addUserData(const ilo::ByteBuffer& data) {
  auto wPimpl = m_pimpl->wP.lock();
  ILO_ASSERT(wPimpl != nullptr, "writer has not been initialized");

  ILO_ASSERT(!wPimpl->m_hasFragments,
             "Writing user data is currently not supported for fragmented mp4 files");

  wPimpl->m_userDataMap[m_pimpl->m_trackId].push_back(data);
}

void CTrackWriter::overwriteBaseMediaDecodeTime(uint64_t newBmdtOffset) {
  auto wPimpl = m_pimpl->wP.lock();
  ILO_ASSERT(wPimpl != nullptr, "writer has not been initialized");
  ILO_ASSERT_WITH(wPimpl->m_hasFragments, std::invalid_argument,
                  "Can't overwrite base media decode time for plain mp4 file mode.");
  wPimpl->overwriteBaseMediaDecodeTime(m_pimpl->m_trackId, newBmdtOffset);
}

template <typename TEnhConfig>
void setupMpegh(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                std::reference_wrapper<BoxElement> trakBoxElement,
                const SMpeghTrackConfig& config) {
  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  TEnhConfig enhConfig;
  enhConfig.sampleEntryConfig.channelCount = 0;
  enhConfig.sampleEntryConfig.sampleRate = config.sampleRate;
  if (config.configRecord) {
    enhConfig.decoderConfig =
        ilo::make_unique<config::CMhaDecoderConfigRecord>(*config.configRecord);
  }
  enhConfig.profileAndLevelCompatibleSets = config.profileAndLevelCompatibleSets;

  CMpeghTreeEnhancer{stsdBoxElement, enhConfig};

  // Creates sample tables in moov and fill mvex if necessary
  auto wPimpl = writerPimpl.lock();
  wPimpl->fillStaticMoovInfo();
}

/* ######---MPEGH Track Writer---###### */
CMpeghTrackWriter::CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                     const SMpeghMhm1TrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();
  setupMpegh<SMhm1EnhancerConfig>(writerPimpl, trakBoxElement, config);
}

CMpeghTrackWriter::CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                     const SMpeghMhm2TrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();
  setupMpegh<SMhm2EnhancerConfig>(writerPimpl, trakBoxElement, config);
}

CMpeghTrackWriter::CMpeghTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                     const SMpeghMha1TrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  ILO_ASSERT(config.configRecord != nullptr, "configRecord is mandatory for Mha1");
  auto trakBoxElement = m_pimpl->createTrack();
  setupMpegh<SMha1EnhancerConfig>(writerPimpl, trakBoxElement, config);
}

CMpeghTrackWriter::~CMpeghTrackWriter() = default;

/* ######---MP4a Track Writer---###### */
CMp4aTrackWriter::CMp4aTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                   const SMp4aTrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();

  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  SMp4aEnhancerConfig mp4aEnhancerConfig;
  mp4aEnhancerConfig.mp4aConfig.channelCount = config.channelCount;
  mp4aEnhancerConfig.mp4aConfig.sampleRate = config.sampleRate;

  ILO_ASSERT(config.configRecord != nullptr, "configRecord is mandatory for mp4a");
  mp4aEnhancerConfig.decoderConfig =
      ilo::make_unique<config::CMp4aDecoderConfigRecord>(*config.configRecord);

  CMp4aTreeEnhancer{stsdBoxElement, mp4aEnhancerConfig};

  auto wPimpl = writerPimpl.lock();

  if (wPimpl->m_writeIods) {
    wPimpl->m_mp4aTrackIds.push_back(wPimpl->m_nextTrackId - 1);
  }

  wPimpl->fillStaticMoovInfo();
}

CMp4aTrackWriter::~CMp4aTrackWriter() = default;

/* ######---Avc Track Writer---###### */
CAvcTrackWriter::CAvcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                 const SAvcTrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();

  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  SAvcEnhancerConfig avc1EnhancerConfig;
  avc1EnhancerConfig.avc1Config.height = config.height;
  avc1EnhancerConfig.avc1Config.width = config.width;
  avc1EnhancerConfig.avc1Config.compressorName = config.compressorName;

  ILO_ASSERT(config.configRecord != nullptr, "configRecord is mandatory for Avc1");
  avc1EnhancerConfig.decoderConfig =
      ilo::make_unique<config::CAvcDecoderConfigRecord>(*config.configRecord);
  m_decoderConfigRecord = ilo::make_unique<config::CAvcDecoderConfigRecord>(*config.configRecord);

  CAvcTreeEnhancer{stsdBoxElement, avc1EnhancerConfig};

  // Creates sample tables in moov and fill mvex if necessary
  auto wPimpl = writerPimpl.lock();
  wPimpl->fillStaticMoovInfo();
}

CAvcTrackWriter::~CAvcTrackWriter() = default;

void CAvcTrackWriter::addSample(const CSample& sample) {
  CTrackWriter::addSample(sample);
}

void CAvcTrackWriter::addSample(const SAvcSample& sample) {
  addSample(sample.sample);
}

void CAvcTrackWriter::addSample(const SAvcNalus& nalus) {
  ILO_ASSERT(m_decoderConfigRecord, "Stored config record is a zero pointer");
  SAvcSample sample;
  tools::convertGeneralVideoNalusToVideoSample(
      nalus, m_decoderConfigRecord->lengthSizeMinusOne() + 1, sample);
  addSample(sample.sample);
}

/* ######---Hevc Track Writer---###### */
CHevcTrackWriter::CHevcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                   const SHevcTrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();

  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  SHevcEnhancerConfig enhancerConfig(config.codingName);
  enhancerConfig.sampleEntryConfig.height = config.height;
  enhancerConfig.sampleEntryConfig.width = config.width;
  enhancerConfig.sampleEntryConfig.compressorName = config.compressorName;

  ILO_ASSERT(config.configRecord != nullptr, "configRecord is mandatory for HEVC");
  enhancerConfig.decoderConfig =
      ilo::make_unique<config::CHevcDecoderConfigRecord>(*config.configRecord);
  m_decoderConfigRecord = ilo::make_unique<config::CHevcDecoderConfigRecord>(*config.configRecord);

  CHevcTreeEnhancer{stsdBoxElement, enhancerConfig};

  // Creates sample tables in moov and fill mvex if necessary
  auto wPimpl = writerPimpl.lock();
  wPimpl->fillStaticMoovInfo();
}

CHevcTrackWriter::~CHevcTrackWriter() {}

void CHevcTrackWriter::addSample(const CSample& sample) {
  CTrackWriter::addSample(sample);
}

void CHevcTrackWriter::addSample(const SHevcSample& sample) {
  addSample(sample.sample);
}

void CHevcTrackWriter::addSample(const SHevcNalus& nalus) {
  ILO_ASSERT(m_decoderConfigRecord, "Stored config record is a zero pointer");
  SHevcSample sample;
  tools::convertGeneralVideoNalusToVideoSample(
      nalus, m_decoderConfigRecord->lengthSizeMinusOne() + 1, sample);
  addSample(sample.sample);
}

/* ######--- Jxs Track Writer ---###### */
CJxsTrackWriter::CJxsTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                 const SJxsTrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();

  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  SJxsEnhancerConfig jxsEnhancerConfig;

  ILO_ASSERT_WITH(config.jxsExtraData != nullptr, std::invalid_argument,
                  "Mandatory JXS extra data is not contained the JxsEnhancerConfig.");
  ILO_ASSERT_WITH(
      config.jxsExtraData->colourInformations.size() > 0, std::invalid_argument,
      "At least 1 Colour Information Box needs to be written for the JXS Sample Entry.");
  jxsEnhancerConfig.jxsExtraData = ilo::make_unique<SJpegxsExtraData>(*config.jxsExtraData);

  ILO_ASSERT_WITH(config.configRecord != nullptr, std::invalid_argument,
                  "configRecord is mandatory for JXS");
  jxsEnhancerConfig.decoderConfig =
      ilo::make_unique<config::CJxsDecoderConfigRecord>(*config.configRecord);

  jxsEnhancerConfig.jxsmConfig.height = config.height;
  jxsEnhancerConfig.jxsmConfig.width = config.width;
  jxsEnhancerConfig.jxsmConfig.compressorName = config.compressorName;

  CJxsTreeEnhancer{stsdBoxElement, jxsEnhancerConfig};

  auto wPimpl = writerPimpl.lock();
  wPimpl->fillStaticMoovInfo();
}

CJxsTrackWriter::~CJxsTrackWriter() = default;

void CJxsTrackWriter::addSample(const CSample& sample) {
  CTrackWriter::addSample(sample);
}

/* ######---Vvc Track Writer---###### */
CVvcTrackWriter::CVvcTrackWriter(std::weak_ptr<CIsobmffWriter::Pimpl> writerPimpl,
                                 const SVvcTrackConfig& config)
    : CTrackWriter(writerPimpl, config) {
  auto trakBoxElement = m_pimpl->createTrack();

  auto stsdBoxElements = findAllElementsWithFourccAndBoxType<box::CSampleDescriptionBox>(
      trakBoxElement, ilo::toFcc("stsd"));
  ILO_ASSERT(stsdBoxElements.size() == 1,
             "one and only one stsd box should be present for each trak");
  BoxElement& stsdBoxElement = const_cast<BoxElement&>(stsdBoxElements[0].get());

  SVvcEnhancerConfig enhancerConfig(config.codingName);
  enhancerConfig.sampleEntryConfig.height = config.height;
  enhancerConfig.sampleEntryConfig.width = config.width;
  enhancerConfig.sampleEntryConfig.compressorName = config.compressorName;

  ILO_ASSERT(config.configRecord != nullptr, "configRecord is mandatory for VVC");
  enhancerConfig.decoderConfig =
      ilo::make_unique<config::CVvcDecoderConfigRecord>(*config.configRecord);
  m_decoderConfigRecord = ilo::make_unique<config::CVvcDecoderConfigRecord>(*config.configRecord);

  CVvcTreeEnhancer{stsdBoxElement, enhancerConfig};

  // Creates sample tables in moov and fill mvex if necessary
  auto wPimpl = writerPimpl.lock();
  wPimpl->fillStaticMoovInfo();
}

CVvcTrackWriter::~CVvcTrackWriter() {}

void CVvcTrackWriter::addSample(const CSample& sample) {
  CTrackWriter::addSample(sample);
}

void CVvcTrackWriter::addSample(const SVvcSample& sample) {
  addSample(sample.sample);
}

void CVvcTrackWriter::addSample(const SVvcNalus& nalus) {
  ILO_ASSERT(m_decoderConfigRecord, "Stored config record is a zero pointer");
  SVvcSample sample;
  tools::convertGeneralVideoNalusToVideoSample(
      nalus, m_decoderConfigRecord->lengthSizeMinusOne() + 1, sample);
  addSample(sample.sample);
}
}  // namespace isobmff
}  // namespace mmt
