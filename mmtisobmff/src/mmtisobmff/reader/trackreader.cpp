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
 * Content: track reader class
 */

// External includes
#include "ilo/memory.h"

// Internal includes
#include "mmtisobmff/reader/trackreader.h"
#include "mmtisobmff/helper/videohelpertools.h"
#include "common/logging.h"
#include "tree/boxtree.h"
#include "pimpl.h"
#include "reader/samplereader.h"
#include "box/stsdbox.h"
#include "box/decoderconfigurationbox.h"
#include "box/mfhdbox.h"
#include "box/trunbox.h"
#include "box/tfhdbox.h"
#include "box/mdatbox.h"
#include "box/tkhdbox.h"
#include "box/containerbox.h"
#include "box/decoderconfigurationprovider.h"
#include "box/jpvibox.h"
#include "box/jxplbox.h"
#include "box/colrbox.h"
#include "box/mhapbox.h"

namespace mmt {
namespace isobmff {
struct CGenericTrackReader::Pimpl {
  Pimpl(std::unique_ptr<CSampleReader>&& sampleReader,
        std::shared_ptr<IDecoderConfigurationProvider> decoderConfigProvider,
        std::shared_ptr<box::CBox> genericSampleEntry)
      : m_sampleReader(std::move(sampleReader)),
        m_decoderConfigProvider(decoderConfigProvider),
        m_genericSampleEntry(genericSampleEntry) {}

  std::unique_ptr<IIsobmffInput> m_input;
  std::unique_ptr<CSampleReader> m_sampleReader;
  std::shared_ptr<IDecoderConfigurationProvider> m_decoderConfigProvider;
  std::shared_ptr<box::CBox> m_genericSampleEntry;
};

static std::shared_ptr<box::CBox> getSampleEntry(const BoxElement& currentTrackElement) {
  std::reference_wrapper<const BoxElement> stsdElement =
      findFirstElementWithFourccAndBoxType<box::CSampleDescriptionBox>(currentTrackElement,
                                                                       ilo::toFcc("stsd"));
  ILO_ASSERT(stsdElement.get().childCount() == 1, "Only a single sample entry is supported");
  return std::dynamic_pointer_cast<box::CBox>(stsdElement.get()[0].item);
}

const BoxElement& getCurrentTrackElement(const BoxTree& tree, size_t tracknumber) {
  auto traks = findAllElementsWithFourccAndBoxType<box::CContainerBox>(tree, ilo::toFcc("trak"));
  return traks.at(tracknumber).get();
}

std::unique_ptr<CSampleReader> createSampleReader(const BoxElement& currentTrackElement,
                                                  std::shared_ptr<CIsobmffReader::Pimpl> rpimpl) {
  auto tkhd =
      findFirstBoxWithFourccAndType<box::CTrackHeaderBox>(currentTrackElement, ilo::toFcc("tkhd"));
  ILO_ASSERT(tkhd != nullptr, "no track header found in iso container");
  uint32_t currentTrackID = tkhd->trackID();

  ILO_ASSERT(rpimpl->trackIdToTrackSampleInfo().count(currentTrackID) >= 1,
             "Selected track with id %d does not contain any samples.", currentTrackID);

  std::unique_ptr<CSampleReader> sampleReader;
  sampleReader = std::unique_ptr<CSampleReader>(new CSampleReader(
      rpimpl->input()->clone(), rpimpl->trackIdToTrackSampleInfo().at(currentTrackID)));
  ILO_ASSERT(sampleReader != nullptr, "Error: Sample reader could not be initialized!");
  return sampleReader;
}

CGenericTrackReader::CGenericTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                         size_t tracknumber) {
  auto rpimpl = reader_pimpl.lock();
  ILO_ASSERT(rpimpl != nullptr, "Error: Reader expired");

  const BoxElement& currentTrackElement = getCurrentTrackElement(rpimpl->tree(), tracknumber);
  auto sampleReader = createSampleReader(currentTrackElement, rpimpl);
  auto genericSampleEntry = getSampleEntry(currentTrackElement);
  ILO_ASSERT(genericSampleEntry != nullptr, "Failed to get the generic sample entry");
  auto decoderConfigProvider =
      findFirstBoxWithType<IDecoderConfigurationProvider>(currentTrackElement);

  p = std::unique_ptr<Pimpl>(
      new Pimpl(std::move(sampleReader), decoderConfigProvider, genericSampleEntry));
}

CGenericTrackReader::~CGenericTrackReader() = default;

ilo::ByteBuffer CGenericTrackReader::decoderConfigRecord() const {
  return p->m_decoderConfigProvider != nullptr ? p->m_decoderConfigProvider->decoderConfiguration()
                                               : ilo::ByteBuffer();
}

SSampleExtraInfo CGenericTrackReader::nextSample(CSample& sample, bool preallocate) const {
  return p->m_sampleReader->nextSample(sample, preallocate);
}

SSampleExtraInfo CGenericTrackReader::sampleByIndex(size_t sampleIndex, CSample& sample,
                                                    bool preallocate) const {
  return p->m_sampleReader->sampleByIndex(sampleIndex, sample, preallocate);
}

SSampleExtraInfo CGenericTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                        CSample& sample, bool preallocate) const {
  return p->m_sampleReader->sampleByTimestamp(seekConfig, sample, preallocate);
}

SSampleExtraInfo CGenericTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return p->m_sampleReader->resolveTimestamp(seekConfig);
}

ilo::Fourcc CGenericTrackReader::codingName() const {
  return p->m_genericSampleEntry->type();
}

//-------------------

struct CGenericAudioTrackReader::PimplAudio {
  PimplAudio(std::shared_ptr<box::CAudioSampleEntry> audioSampleEntry)
      : m_audioSampleEntry(audioSampleEntry) {}

  std::shared_ptr<box::CAudioSampleEntry> m_audioSampleEntry;
};

CGenericAudioTrackReader::CGenericAudioTrackReader(
    std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
    : CGenericTrackReader(reader_pimpl, tracknumber) {
  auto audioSampleEntry =
      std::dynamic_pointer_cast<box::CAudioSampleEntry>(p->m_genericSampleEntry);
  ILO_ASSERT(audioSampleEntry != nullptr,
             "Error: Generic Audio Track reader could not access audio sample entry!");

  pa = std::unique_ptr<CGenericAudioTrackReader::PimplAudio>(
      new CGenericAudioTrackReader::PimplAudio(audioSampleEntry));
}

CGenericAudioTrackReader::~CGenericAudioTrackReader() {}

uint16_t CGenericAudioTrackReader::channelCount() const {
  return pa->m_audioSampleEntry->channelCount();
}

uint16_t CGenericAudioTrackReader::sampleSize() const {
  return pa->m_audioSampleEntry->sampleSize();
}

uint32_t CGenericAudioTrackReader::sampleRate() const {
  return pa->m_audioSampleEntry->sampleRate();
}

//------------------------------------

struct CGenericVideoTrackReader::PimplVideo {
  PimplVideo(std::shared_ptr<box::CVisualSampleEntry> visualSampleEntry)
      : m_visualSampleEntry(visualSampleEntry) {}

  std::shared_ptr<box::CVisualSampleEntry> m_visualSampleEntry;
};

CGenericVideoTrackReader::CGenericVideoTrackReader(
    std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
    : CGenericTrackReader(reader_pimpl, tracknumber) {
  auto visualSampleEntry =
      std::dynamic_pointer_cast<box::CVisualSampleEntry>(p->m_genericSampleEntry);
  ILO_ASSERT(visualSampleEntry != nullptr,
             "Generic video track reader could not access visual sample entry!");

  pv = std::unique_ptr<CGenericVideoTrackReader::PimplVideo>(
      new CGenericVideoTrackReader::PimplVideo(visualSampleEntry));
}

CGenericVideoTrackReader::~CGenericVideoTrackReader() {}

uint16_t CGenericVideoTrackReader::width() const {
  return pv->m_visualSampleEntry->width();
}

uint16_t CGenericVideoTrackReader::height() const {
  return pv->m_visualSampleEntry->height();
}

double CGenericVideoTrackReader::horizontalResolutionDPI() const {
  return pv->m_visualSampleEntry->horizresolution() / static_cast<double>(1 << 16);
}

double CGenericVideoTrackReader::verticalResolutionDPI() const {
  return pv->m_visualSampleEntry->vertresolution() / static_cast<double>(1 << 16);
}

uint16_t CGenericVideoTrackReader::frameCount() const {
  return pv->m_visualSampleEntry->framecount();
}

std::string CGenericVideoTrackReader::compressorName() const {
  return pv->m_visualSampleEntry->compressorname();
}

uint16_t CGenericVideoTrackReader::depth() const {
  return pv->m_visualSampleEntry->depth();
}

//-----------------------------------

struct CMpeghTrackReader::PimplMpegh {
  PimplMpegh(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericAudioTrackReader(reader_pimpl, tracknumber) {}

  CGenericAudioTrackReader m_genericAudioTrackReader;
  std::vector<uint8_t> m_profileAndLevelCompatibleSets;
};

CMpeghTrackReader::CMpeghTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                     size_t tracknumber) {
  pmpegh = std::unique_ptr<CMpeghTrackReader::PimplMpegh>(
      new CMpeghTrackReader::PimplMpegh(reader_pimpl, tracknumber));

  auto rpimpl = reader_pimpl.lock();
  const BoxElement& currentTrackElement = getCurrentTrackElement(rpimpl->tree(), tracknumber);
  auto mhaPbox = findAllBoxesWithFourccAndType<box::CMhaProfileLevelCompatibilitySetBox>(
      currentTrackElement, ilo::toFcc("mhaP"));

  if (!mhaPbox.empty()) {
    pmpegh->m_profileAndLevelCompatibleSets = mhaPbox[0]->profileAndLevelCompatibleSets();
  }
}

CMpeghTrackReader::~CMpeghTrackReader() = default;

SSampleExtraInfo CMpeghTrackReader::nextSample(CSample& sample, bool preallocate) const {
  return pmpegh->m_genericAudioTrackReader.nextSample(sample, preallocate);
}

SSampleExtraInfo CMpeghTrackReader::sampleByIndex(size_t sampleIndex, CSample& sample,
                                                  bool preallocate) const {
  return pmpegh->m_genericAudioTrackReader.sampleByIndex(sampleIndex, sample, preallocate);
}

SSampleExtraInfo CMpeghTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                      CSample& sample, bool preallocate) const {
  return pmpegh->m_genericAudioTrackReader.sampleByTimestamp(seekConfig, sample, preallocate);
}

SSampleExtraInfo CMpeghTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return pmpegh->m_genericAudioTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CMpeghTrackReader::codingName() const {
  return pmpegh->m_genericAudioTrackReader.codingName();
}

uint32_t CMpeghTrackReader::sampleRate() const {
  return pmpegh->m_genericAudioTrackReader.sampleRate();
}

std::unique_ptr<config::CMhaDecoderConfigRecord> CMpeghTrackReader::mhaDecoderConfigRecord() const {
  std::unique_ptr<config::CMhaDecoderConfigRecord> result;

  auto cfgRecordBlob = pmpegh->m_genericAudioTrackReader.decoderConfigRecord();
  if (cfgRecordBlob.size() == 0) {
    return result;
  }

  ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
  return std::unique_ptr<config::CMhaDecoderConfigRecord>(
      new config::CMhaDecoderConfigRecord(dataBegin, cfgRecordBlob.end()));
}

std::vector<uint8_t> CMpeghTrackReader::profileAndLevelCompatibleSets() const {
  return pmpegh->m_profileAndLevelCompatibleSets;
}

//-----------------------------------

struct CMp4aTrackReader::PimplMp4a {
  PimplMp4a(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericAudioTrackReader(reader_pimpl, tracknumber) {}

  CGenericAudioTrackReader m_genericAudioTrackReader;
};

CMp4aTrackReader::CMp4aTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                   size_t tracknumber) {
  pmp4a = std::unique_ptr<CMp4aTrackReader::PimplMp4a>(
      new CMp4aTrackReader::PimplMp4a(reader_pimpl, tracknumber));
}

CMp4aTrackReader::~CMp4aTrackReader() {}

SSampleExtraInfo CMp4aTrackReader::nextSample(CSample& sample, bool preallocate) const {
  return pmp4a->m_genericAudioTrackReader.nextSample(sample, preallocate);
}

SSampleExtraInfo CMp4aTrackReader::sampleByIndex(size_t sampleIndex, CSample& sample,
                                                 bool preallocate) const {
  return pmp4a->m_genericAudioTrackReader.sampleByIndex(sampleIndex, sample, preallocate);
}

SSampleExtraInfo CMp4aTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig, CSample& sample,
                                                     bool preallocate) const {
  return pmp4a->m_genericAudioTrackReader.sampleByTimestamp(seekConfig, sample, preallocate);
}

SSampleExtraInfo CMp4aTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return pmp4a->m_genericAudioTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CMp4aTrackReader::codingName() const {
  return pmp4a->m_genericAudioTrackReader.codingName();
}

uint32_t CMp4aTrackReader::sampleRate() const {
  return pmp4a->m_genericAudioTrackReader.sampleRate();
}

uint16_t CMp4aTrackReader::channelCount() const {
  return pmp4a->m_genericAudioTrackReader.channelCount();
}

std::unique_ptr<config::CMp4aDecoderConfigRecord> CMp4aTrackReader::mp4aDecoderConfigRecord()
    const {
  std::unique_ptr<config::CMp4aDecoderConfigRecord> result;

  auto cfgRecordBlob = pmp4a->m_genericAudioTrackReader.decoderConfigRecord();
  if (cfgRecordBlob.size() == 0) {
    return result;
  }

  ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
  return std::unique_ptr<config::CMp4aDecoderConfigRecord>(
      new config::CMp4aDecoderConfigRecord(dataBegin, cfgRecordBlob.end()));
}
//--------------------------------------------------------------------------

struct CAvcTrackReader::PimplAvc {
  PimplAvc(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericVideoTrackReader(reader_pimpl, tracknumber) {}

  CGenericVideoTrackReader m_genericVideoTrackReader;
  std::unique_ptr<config::CAvcDecoderConfigRecord> m_avcConfigRecord;
};

CAvcTrackReader::CAvcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                 size_t tracknumber) {
  pavc = std::unique_ptr<CAvcTrackReader::PimplAvc>(
      new CAvcTrackReader::PimplAvc(reader_pimpl, tracknumber));

  auto cfgRecordBlob = pavc->m_genericVideoTrackReader.decoderConfigRecord();
  if (cfgRecordBlob.size() != 0) {
    ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
    pavc->m_avcConfigRecord = std::unique_ptr<config::CAvcDecoderConfigRecord>(
        new config::CAvcDecoderConfigRecord(dataBegin, cfgRecordBlob.end()));
  }
}

CAvcTrackReader::~CAvcTrackReader() = default;

SSampleExtraInfo CAvcTrackReader::nextSample(SAvcSample& avcSample, bool preallocate) const {
  avcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pavc->m_genericVideoTrackReader.nextSample(avcSample.sample, preallocate);
  if (avcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(avcSample, *pavc->m_avcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CAvcTrackReader::sampleByIndex(size_t sampleIndex, SAvcSample& avcSample,
                                                bool preallocate) const {
  avcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pavc->m_genericVideoTrackReader.sampleByIndex(sampleIndex, avcSample.sample, preallocate);
  if (avcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(avcSample, *pavc->m_avcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CAvcTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                    SAvcSample& avcSample, bool preallocate) const {
  avcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pavc->m_genericVideoTrackReader.sampleByTimestamp(seekConfig, avcSample.sample, preallocate);
  if (avcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(avcSample, *pavc->m_avcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CAvcTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return pavc->m_genericVideoTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CAvcTrackReader::codingName() const {
  return pavc->m_genericVideoTrackReader.codingName();
}

uint16_t CAvcTrackReader::width() const {
  return pavc->m_genericVideoTrackReader.width();
}

uint16_t CAvcTrackReader::height() const {
  return pavc->m_genericVideoTrackReader.height();
}

std::string CAvcTrackReader::compressorName() const {
  return pavc->m_genericVideoTrackReader.compressorName();
}

uint16_t CAvcTrackReader::depth() const {
  return pavc->m_genericVideoTrackReader.depth();
}

std::unique_ptr<config::CAvcDecoderConfigRecord> CAvcTrackReader::avcDecoderConfigRecord() const {
  return std::unique_ptr<config::CAvcDecoderConfigRecord>(
      new config::CAvcDecoderConfigRecord(*pavc->m_avcConfigRecord));
}

//--------------------------------------------------------------------------

struct CHevcTrackReader::PimplHevc {
  PimplHevc(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericVideoTrackReader(reader_pimpl, tracknumber) {}

  CGenericVideoTrackReader m_genericVideoTrackReader;
  std::unique_ptr<config::CHevcDecoderConfigRecord> m_hevcConfigRecord;
};

CHevcTrackReader::CHevcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                   size_t tracknumber) {
  phevc = std::unique_ptr<CHevcTrackReader::PimplHevc>(
      new CHevcTrackReader::PimplHevc(reader_pimpl, tracknumber));

  auto cfgRecordBlob = phevc->m_genericVideoTrackReader.decoderConfigRecord();
  if (cfgRecordBlob.size() != 0) {
    ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
    phevc->m_hevcConfigRecord = std::unique_ptr<config::CHevcDecoderConfigRecord>(
        new config::CHevcDecoderConfigRecord(dataBegin, cfgRecordBlob.end()));
  }
}

CHevcTrackReader::~CHevcTrackReader() = default;

SSampleExtraInfo CHevcTrackReader::nextSample(SHevcSample& hevcSample, bool preallocate) const {
  hevcSample.clear();
  SSampleExtraInfo sExtraInfo =
      phevc->m_genericVideoTrackReader.nextSample(hevcSample.sample, preallocate);
  if (hevcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(hevcSample, *phevc->m_hevcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CHevcTrackReader::sampleByIndex(size_t sampleIndex, SHevcSample& hevcSample,
                                                 bool preallocate) const {
  hevcSample.clear();
  SSampleExtraInfo sExtraInfo =
      phevc->m_genericVideoTrackReader.sampleByIndex(sampleIndex, hevcSample.sample, preallocate);
  if (hevcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(hevcSample, *phevc->m_hevcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CHevcTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                     SHevcSample& hevcSample,
                                                     bool preallocate) const {
  hevcSample.clear();
  SSampleExtraInfo sExtraInfo = phevc->m_genericVideoTrackReader.sampleByTimestamp(
      seekConfig, hevcSample.sample, preallocate);
  if (hevcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(hevcSample, *phevc->m_hevcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CHevcTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return phevc->m_genericVideoTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CHevcTrackReader::codingName() const {
  return phevc->m_genericVideoTrackReader.codingName();
}

uint16_t CHevcTrackReader::width() const {
  return phevc->m_genericVideoTrackReader.width();
}

uint16_t CHevcTrackReader::height() const {
  return phevc->m_genericVideoTrackReader.height();
}

std::string CHevcTrackReader::compressorName() const {
  return phevc->m_genericVideoTrackReader.compressorName();
}

uint16_t CHevcTrackReader::depth() const {
  return phevc->m_genericVideoTrackReader.depth();
}

std::unique_ptr<config::CHevcDecoderConfigRecord> CHevcTrackReader::hevcDecoderConfigRecord()
    const {
  return std::unique_ptr<config::CHevcDecoderConfigRecord>(
      new config::CHevcDecoderConfigRecord(*phevc->m_hevcConfigRecord));
}

//--------------------------------------------------------------------------

struct CJxsTrackReader::PimplJxs {
  PimplJxs(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericVideoTrackReader(reader_pimpl, tracknumber) {
    auto rpimpl = reader_pimpl.lock();
    const BoxElement& currentTrackElement = getCurrentTrackElement(rpimpl->tree(), tracknumber);
    auto jpegVideoInfoList = findAllBoxesWithFourccAndType<box::CJPEGXSVideoInformationBox>(
        currentTrackElement, ilo::toFcc("jpvi"));
    auto profileAndLevelList = findAllBoxesWithFourccAndType<box::CJXPLProfileandLevelBox>(
        currentTrackElement, ilo::toFcc("jxpl"));
    auto colorinfoList = findAllBoxesWithFourccAndType<box::CColourInformationBox>(
        currentTrackElement, ilo::toFcc("colr"));

    ILO_ASSERT(
        jpegVideoInfoList.size() <= 1,
        "There are more than one JPEGXSVideoInformationBox in the track with the tracknumber %d.",
        tracknumber);
    ILO_ASSERT(
        profileAndLevelList.size() <= 1,
        "There are more than one JPEGXSProfileandLevelBox in the track with the tracknumber %d.",
        tracknumber);

    if (jpegVideoInfoList.size() > 0) {
      m_jpegxsExtraData.brat = jpegVideoInfoList[0]->brat();
      m_jpegxsExtraData.frat = jpegVideoInfoList[0]->frat();
      m_jpegxsExtraData.schar = jpegVideoInfoList[0]->schar();
      m_jpegxsExtraData.tcod = jpegVideoInfoList[0]->tcod();
    }

    if (profileAndLevelList.size() > 0) {
      m_jpegxsExtraData.plev = profileAndLevelList[0]->plev();
      m_jpegxsExtraData.ppih = profileAndLevelList[0]->ppih();
    }

    for (const auto& colorInfo : colorinfoList) {
      SColourInformation colorInformation;
      colorInformation.colourType = colorInfo->colourType();
      if (colorInfo->hasColourPrimaries()) {
        colorInformation.colourPrimaries = colorInfo->colourPrimaries();
      }
      if (colorInfo->hasFullRangeFlag()) {
        colorInformation.fullRangeFlag = colorInfo->fullRangeFlag();
      }
      if (colorInfo->hasIccProfile()) {
        colorInformation.iccProfile = colorInfo->iccProfile();
      }
      if (colorInfo->hasMatrixCoefficients()) {
        colorInformation.matrixCoefficients = colorInfo->matrixCoefficients();
      }
      if (colorInfo->hasTransferCharacteristics()) {
        colorInformation.transferCharacteristics = colorInfo->transferCharacteristics();
      }
      m_jpegxsExtraData.colourInformations.push_back(colorInformation);
    }
  }

  SJpegxsExtraData m_jpegxsExtraData;
  CGenericVideoTrackReader m_genericVideoTrackReader;
  std::unique_ptr<config::CJxsDecoderConfigRecord> m_jxsConfigRecord;
};

CJxsTrackReader::CJxsTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                 size_t tracknumber) {
  pjxs = ilo::make_unique<CJxsTrackReader::PimplJxs>(reader_pimpl, tracknumber);

  auto cfgRecordBlob = pjxs->m_genericVideoTrackReader.decoderConfigRecord();
  if (cfgRecordBlob.size() != 0) {
    ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
    pjxs->m_jxsConfigRecord =
        ilo::make_unique<config::CJxsDecoderConfigRecord>(dataBegin, cfgRecordBlob.end());
  }
}

CJxsTrackReader::~CJxsTrackReader() = default;

SSampleExtraInfo CJxsTrackReader::nextSample(CSample& jxsSample, bool preallocate) const {
  return pjxs->m_genericVideoTrackReader.nextSample(jxsSample, preallocate);
}

SSampleExtraInfo CJxsTrackReader::sampleByIndex(size_t sampleIndex, CSample& jxsSample,
                                                bool preallocate) const {
  return pjxs->m_genericVideoTrackReader.sampleByIndex(sampleIndex, jxsSample, preallocate);
}

SSampleExtraInfo CJxsTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                    CSample& jxsSample, bool preallocate) const {
  return pjxs->m_genericVideoTrackReader.sampleByTimestamp(seekConfig, jxsSample, preallocate);
}

SSampleExtraInfo CJxsTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return pjxs->m_genericVideoTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CJxsTrackReader::codingName() const {
  return pjxs->m_genericVideoTrackReader.codingName();
}

uint16_t CJxsTrackReader::width() const {
  return pjxs->m_genericVideoTrackReader.width();
}

uint16_t CJxsTrackReader::height() const {
  return pjxs->m_genericVideoTrackReader.height();
}

std::string CJxsTrackReader::compressorName() const {
  return pjxs->m_genericVideoTrackReader.compressorName();
}

uint16_t CJxsTrackReader::depth() const {
  return pjxs->m_genericVideoTrackReader.depth();
}

SJpegxsExtraData CJxsTrackReader::jpegxsExtraData() const {
  return pjxs->m_jpegxsExtraData;
}

std::unique_ptr<config::CJxsDecoderConfigRecord> CJxsTrackReader::jxsDecoderConfigRecord() const {
  return std::unique_ptr<config::CJxsDecoderConfigRecord>(
      new config::CJxsDecoderConfigRecord(*pjxs->m_jxsConfigRecord));
}

//--------------------------------------------------------------------------

struct CVvcTrackReader::PimplVvc {
  PimplVvc(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl, size_t tracknumber)
      : m_genericVideoTrackReader(reader_pimpl, tracknumber) {}

  CGenericVideoTrackReader m_genericVideoTrackReader;
  std::unique_ptr<config::CVvcDecoderConfigRecord> m_vvcConfigRecord;
};

CVvcTrackReader::CVvcTrackReader(std::weak_ptr<CIsobmffReader::Pimpl> reader_pimpl,
                                 size_t tracknumber) {
  pvvc = ilo::make_unique<CVvcTrackReader::PimplVvc>(reader_pimpl, tracknumber);

  auto cfgRecordBlob = pvvc->m_genericVideoTrackReader.decoderConfigRecord();
  if (!cfgRecordBlob.empty()) {
    ilo::ByteBuffer::const_iterator dataBegin = cfgRecordBlob.cbegin();
    pvvc->m_vvcConfigRecord =
        ilo::make_unique<config::CVvcDecoderConfigRecord>(dataBegin, cfgRecordBlob.end());
  }
}

CVvcTrackReader::~CVvcTrackReader() = default;

SSampleExtraInfo CVvcTrackReader::nextSample(SVvcSample& vvcSample, bool preallocate) const {
  vvcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pvvc->m_genericVideoTrackReader.nextSample(vvcSample.sample, preallocate);
  if (vvcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(vvcSample, *pvvc->m_vvcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CVvcTrackReader::sampleByIndex(size_t sampleIndex, SVvcSample& vvcSample,
                                                bool preallocate) const {
  vvcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pvvc->m_genericVideoTrackReader.sampleByIndex(sampleIndex, vvcSample.sample, preallocate);
  if (vvcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(vvcSample, *pvvc->m_vvcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CVvcTrackReader::sampleByTimestamp(const SSeekConfig& seekConfig,
                                                    SVvcSample& vvcSample, bool preallocate) const {
  vvcSample.clear();
  SSampleExtraInfo sExtraInfo =
      pvvc->m_genericVideoTrackReader.sampleByTimestamp(seekConfig, vvcSample.sample, preallocate);
  if (vvcSample.sample.rawData.size() != 0) {
    tools::parseVideoSampleNalus(vvcSample, *pvvc->m_vvcConfigRecord);
  }
  return sExtraInfo;
}

SSampleExtraInfo CVvcTrackReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  return pvvc->m_genericVideoTrackReader.resolveTimestamp(seekConfig);
}

ilo::Fourcc CVvcTrackReader::codingName() const {
  return pvvc->m_genericVideoTrackReader.codingName();
}

uint16_t CVvcTrackReader::width() const {
  return pvvc->m_genericVideoTrackReader.width();
}

uint16_t CVvcTrackReader::height() const {
  return pvvc->m_genericVideoTrackReader.height();
}

std::string CVvcTrackReader::compressorName() const {
  return pvvc->m_genericVideoTrackReader.compressorName();
}

uint16_t CVvcTrackReader::depth() const {
  return pvvc->m_genericVideoTrackReader.depth();
}

std::unique_ptr<config::CVvcDecoderConfigRecord> CVvcTrackReader::vvcDecoderConfigRecord() const {
  return ilo::make_unique<config::CVvcDecoderConfigRecord>(*pvvc->m_vvcConfigRecord);
}
}  // namespace isobmff
}  // namespace mmt
