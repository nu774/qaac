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
 * Content: common tools for sample conversion
 */

// System includes
#include <string>
#include <iostream>
#include <exception>
#include <vector>
#include <ctime>
#include <cmath>

// External includes
#include "ilo/memory.h"
#include "ilo/string_utils.h"

// Internal includes
#include "common/logging.h"
#include "mmtisobmff/helper/commonhelpertools.h"
#include "mmtisobmff/helper/printhelpertools.h"
#include "mmtisobmff/writer/trackwriter.h"
#include "common/bytebuffertools_extension.h"

namespace mmt {
namespace isobmff {
namespace tools {
std::deque<std::unique_ptr<CSample>> getAllSamples(
    std::unique_ptr<CGenericTrackReader>& trackReader) {
  std::deque<std::unique_ptr<CSample>> sampleDeque;
  CSample sample;
  trackReader->nextSample(sample);

  while (!sample.empty()) {
    sampleDeque.push_back(ilo::make_unique<CSample>(sample));
    trackReader->nextSample(sample);
  }

  return sampleDeque;
}

// expresses the time difference between UNIX time (1970) and UTC time (1904) in seconds
const uint64_t UNIX_TO_UTC = 2082758400 + 24 * 60 * 60;

uint64_t currentUTCTime() {
  // we expect this returns the seconds since 1970-01-01 (posix timestamp)
  std::time_t time = std::time(nullptr);
  return static_cast<uint64_t>(time) + UNIX_TO_UTC;
}

std::string UTCTimeToString(uint64_t seconds) {
  time_t t = static_cast<time_t>(seconds - UNIX_TO_UTC);

  // ISOBMFF timestamps are based on UTC epoch (00:00:00 1 January 1904)
  // C/C++ functions work with UNIX epoch (00:00:00 1 January 1970)
  // If we want to handle timestamps in between these two points we have to use a different approach
  if (t < 0) {
    return std::string();
  }

  std::stringstream ss;
  std::tm buf;
  char mbstr[100];

#if defined(WIN32) || defined(_WIN32)
  localtime_s(&buf, &t);
#else
  buf = *(localtime(&t));
#endif
  if (std::strftime(mbstr, sizeof(mbstr), "%c %Z", &buf)) {
    ss << mbstr;
  }

  return ss.str();
}

SSampleFlags valueToSampleFlags(uint32_t value) {
  SSampleFlags flags;
  flags.isLeading = static_cast<SSampleFlags::Leading>((value & 0xC000000) >> 26);
  flags.dependsON = static_cast<SSampleFlags::SampleDependsOn>((value & 0x3000000) >> 24);
  flags.isDependedOn = static_cast<SSampleFlags::SampleIsDependedOn>((value & 0xC00000) >> 22);
  flags.hasRedundancy = static_cast<SSampleFlags::SampleHasRedundancy>((value & 0x300000) >> 20);
  flags.paddingValue = static_cast<uint8_t>((value & 0xE0000) >> 17);
  flags.isNonSyncSample = ((value & 0x10000) >> 16) == 1;
  flags.degradationPriority = static_cast<uint16_t>(value & 0xFFFF);

  return flags;
}

uint32_t sampleFlagsToValue(const SSampleFlags& sampleFlags) {
  uint32_t flagsValue = ((static_cast<uint8_t>(sampleFlags.isLeading)) << 26) +
                        ((static_cast<uint8_t>(sampleFlags.dependsON)) << 24) +
                        ((static_cast<uint8_t>(sampleFlags.isDependedOn)) << 22) +
                        ((static_cast<uint8_t>(sampleFlags.hasRedundancy)) << 20) +
                        ((static_cast<uint8_t>(sampleFlags.paddingValue)) << 17) +
                        ((static_cast<uint8_t>(sampleFlags.isNonSyncSample)) << 16) +
                        static_cast<uint16_t>(sampleFlags.degradationPriority);
  return flagsValue;
}

void clearFragNumber(CSample& sample) {
  sample.fragmentNumber = 0;
}

void setFragNumber(CSample& sample, uint32_t fragNumber) {
  sample.fragmentNumber = fragNumber;
}

void clearFragNumber(SNaluSample& sample) {
  sample.sample.fragmentNumber = 0;
}

void setFragNumber(SNaluSample& sample, uint32_t fragNumber) {
  sample.sample.fragmentNumber = fragNumber;
}

bool isSyncSample(CSample& sample) {
  return sample.isSyncSample;
}

bool isSyncSample(SNaluSample& sample) {
  return sample.sample.isSyncSample;
}

template <typename Tsample>
void checkSyncSample(Tsample& sample, bool ignoreSyncSample) {
  bool synSample = isSyncSample(sample);
  if (ignoreSyncSample && !synSample) {
    ILO_LOG_WARNING(
        "Fragment does not start with a sync sample. User used ignoreSyncSample override");
  } else {
    ILO_ASSERT(isSyncSample(sample), "Fragment does not start with a SyncSample");
  }
}

template <typename Tsample>
uint64_t getDuration(const Tsample& t) {
  return t.sample.duration;
}

template <>
uint64_t getDuration(const CSample& t) {
  return t.duration;
}

template <typename Tsample, class Treader, class Twriter>
void copyAUs(const Treader& tReader, const Twriter& tWriter, const SCopyConfig& config) {
  // Get all samples in order. Each call fetches the next sample.
  Tsample sample;
  tReader->nextSample(sample);

  uint32_t fragNumber = 1;
  uint64_t currentDuration = 0;

  while (!sample.empty()) {
    if (!config.keepFragNumber) {
      // Defragment
      if (config.fragmentDuration == 0 && !config.fragmentEverySyncSample) {
        clearFragNumber(sample);
      }
      // Fragment by using SyncSample table
      else if (config.fragmentDuration == 0 && config.fragmentEverySyncSample) {
        if (isSyncSample(sample) && currentDuration > 0) {
          fragNumber++;
          currentDuration = 0;
        }
        setFragNumber(sample, fragNumber);
      }
      // Fragment by using fragment duration
      else {
        ILO_ASSERT(config.fragmentDuration > 0, "Fragment duration cannot be 0.");
        if ((currentDuration >= config.fragmentDuration)) {
          checkSyncSample<Tsample>(sample, config.ignoreSyncSample);
          fragNumber++;
          currentDuration = 0;
        }
        setFragNumber(sample, fragNumber);
      }
    }

    tWriter->addSample(sample);
    tReader->nextSample(sample);
    currentDuration += getDuration(sample);
  }
}

template <class Twriter>
void copyEditList(const Twriter& writer, const SCopyConfig& config) {
  if ((config.fragmentDuration > 0 ||
       (config.fragmentDuration == 0 && config.fragmentEverySyncSample)) &&
      config.trackInfo.editList.size() > 0) {
    ILO_LOG_WARNING(
        "Dropping edit list of trakId %d, since writing fragmenting "
        "mp4 files with edit lists are not supported",
        config.trackInfo.trackId);
  } else {
    for (auto entry : config.trackInfo.editList) {
      if (config.newMovieTimescale != config.oldMovieTimescale) {
        entry.segmentDuration = static_cast<uint64_t>(std::floor(
            entry.segmentDuration * config.newMovieTimescale / config.oldMovieTimescale));
      }
      writer->addEditListEntry(entry);
    }
  }
}

template <class Twriter>
void copyTrakUdta(const Twriter& writer, const SCopyConfig& config) {
  for (const auto& udta : config.trackInfo.userData) {
    writer->addUserData(udta);
  }
}

void fillBasicConfig(STrackConfig& tConfig, const SCopyConfig& config) {
  tConfig.mediaTimescale = config.trackInfo.timescale;
}

void fillAudioConfig(SBaseAudioConfig& aConfig, const SCopyConfig& config) {
  aConfig.language = config.trackInfo.language;
}

template <typename ConfigType>
void copyMpegh(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto mpeghTrackReader = reader.trackByIndex<CMpeghTrackReader>(config.trackInfo.trackIndex);

  ConfigType mpeghTrackConfig;
  fillBasicConfig(mpeghTrackConfig, config);
  fillAudioConfig(mpeghTrackConfig, config);
  mpeghTrackConfig.sampleRate = mpeghTrackReader->sampleRate();
  mpeghTrackConfig.configRecord = mpeghTrackReader->mhaDecoderConfigRecord();

  auto mpeghTrackWriter = writer.trackWriter<CMpeghTrackWriter>(mpeghTrackConfig);

  copyAUs<CSample>(mpeghTrackReader, mpeghTrackWriter, config);
  copyEditList(mpeghTrackWriter, config);
  copyTrakUdta(mpeghTrackWriter, config);
}

template <typename ConfigType>
void copyMp4a(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto mp4aTrackReader = reader.trackByIndex<CMp4aTrackReader>(config.trackInfo.trackIndex);

  ConfigType mp4aTrackConfig;
  fillBasicConfig(mp4aTrackConfig, config);
  fillAudioConfig(mp4aTrackConfig, config);
  mp4aTrackConfig.channelCount = mp4aTrackReader->channelCount();
  mp4aTrackConfig.sampleRate = mp4aTrackReader->sampleRate();
  mp4aTrackConfig.configRecord = mp4aTrackReader->mp4aDecoderConfigRecord();

  auto mp4aTrackWriter = writer.trackWriter<CMp4aTrackWriter>(mp4aTrackConfig);

  copyAUs<CSample>(mp4aTrackReader, mp4aTrackWriter, config);
  copyEditList(mp4aTrackWriter, config);
  copyTrakUdta(mp4aTrackWriter, config);
}

template <typename ConfigType>
void copyHevc(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto hevcTrackReader = reader.trackByIndex<CHevcTrackReader>(config.trackInfo.trackIndex);

  ConfigType hevcTrackConfig;
  fillBasicConfig(hevcTrackConfig, config);
  hevcTrackConfig.height = hevcTrackReader->height();
  hevcTrackConfig.width = hevcTrackReader->width();
  hevcTrackConfig.configRecord = hevcTrackReader->hevcDecoderConfigRecord();

  auto hevcTrackWriter = writer.trackWriter<CHevcTrackWriter>(hevcTrackConfig);

  copyAUs<SHevcSample>(hevcTrackReader, hevcTrackWriter, config);
  copyEditList(hevcTrackWriter, config);
  copyTrakUdta(hevcTrackWriter, config);
}

template <typename ConfigType>
void copyAvc(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto avcTrackReader = reader.trackByIndex<CAvcTrackReader>(config.trackInfo.trackIndex);

  ConfigType avcTrackConfig;
  fillBasicConfig(avcTrackConfig, config);
  avcTrackConfig.height = avcTrackReader->height();
  avcTrackConfig.width = avcTrackReader->width();
  avcTrackConfig.configRecord = avcTrackReader->avcDecoderConfigRecord();

  auto avcTrackWriter = writer.trackWriter<CAvcTrackWriter>(avcTrackConfig);

  copyAUs<SAvcSample>(avcTrackReader, avcTrackWriter, config);
  copyEditList(avcTrackWriter, config);
  copyTrakUdta(avcTrackWriter, config);
}

void copyJxs(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto jxsTrackReader = reader.trackByIndex<CJxsTrackReader>(config.trackInfo.trackIndex);

  SJxsTrackConfig jxsTrackConfig;
  fillBasicConfig(jxsTrackConfig, config);
  jxsTrackConfig.height = jxsTrackReader->height();
  jxsTrackConfig.width = jxsTrackReader->width();
  jxsTrackConfig.compressorName = jxsTrackReader->compressorName();
  jxsTrackConfig.codingName = jxsTrackReader->codingName();
  jxsTrackConfig.configRecord = jxsTrackReader->jxsDecoderConfigRecord();
  jxsTrackConfig.jxsExtraData =
      ilo::make_unique<SJpegxsExtraData>(jxsTrackReader->jpegxsExtraData());

  auto jxsTrackWriter = writer.trackWriter<CJxsTrackWriter>(jxsTrackConfig);

  copyAUs<CSample>(jxsTrackReader, jxsTrackWriter, config);
  copyEditList(jxsTrackWriter, config);
  copyTrakUdta(jxsTrackWriter, config);
}

template <typename ConfigType>
void copyVvc(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  auto vvcTrackReader = reader.trackByIndex<CVvcTrackReader>(config.trackInfo.trackIndex);

  ConfigType vvcTrackConfig;
  fillBasicConfig(vvcTrackConfig, config);
  vvcTrackConfig.height = vvcTrackReader->height();
  vvcTrackConfig.width = vvcTrackReader->width();
  vvcTrackConfig.configRecord = vvcTrackReader->vvcDecoderConfigRecord();

  auto vvcTrackWriter = writer.trackWriter<CVvcTrackWriter>(vvcTrackConfig);

  copyAUs<SVvcSample>(vvcTrackReader, vvcTrackWriter, config);
  copyEditList(vvcTrackWriter, config);
  copyTrakUdta(vvcTrackWriter, config);
}

void copyTrack(CIsobmffReader& reader, CIsobmffWriter& writer, const SCopyConfig& config) {
  if (config.trackInfo.codingName == ilo::toFcc("mhm1")) {
    copyMpegh<SMpeghMhm1TrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("mha1")) {
    copyMpegh<SMpeghMha1TrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("mp4a")) {
    copyMp4a<SMp4aTrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("avc1")) {
    copyAvc<SAvcTrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("hvc1")) {
    copyHevc<SHvc1TrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("hev1")) {
    copyHevc<SHev1TrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("jxsm")) {
    copyJxs(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("vvc1")) {
    copyVvc<SVvc1TrackConfig>(reader, writer, config);
    return;
  }
  if (config.trackInfo.codingName == ilo::toFcc("vvi1")) {
    copyVvc<SVvi1TrackConfig>(reader, writer, config);
    return;
  }

  throw std::runtime_error("Track type of " + ilo::toString(config.trackInfo.codingName) +
                           " is currently not supported!");
}

void fillTrackConfig(const SEasyTrackConfig& eConfig, STrackConfig& tConfig) {
  tConfig.trackID = eConfig.trackId;
  tConfig.mediaTimescale = eConfig.timescale;
  tConfig.defaultSampleGroup = eConfig.defaultSampleGroup;
}

void fillAudioConfig(const SEasyTrackConfig& eConfig, SBaseAudioConfig& tConfig) {
  fillTrackConfig(eConfig, tConfig);

  tConfig.sampleRate = eConfig.sampleRate;
  tConfig.language = eConfig.language;
}

void fillMpeghConfig(const SEasyTrackConfig& eConfig, SMpeghTrackConfig& tConfig) {
  fillAudioConfig(eConfig, tConfig);

  tConfig.profileAndLevelCompatibleSets = eConfig.compatibleProfileLevels;
}

void fillVideoConfig(const SEasyTrackConfig& eConfig, SBaseVideoConfig& tConfig) {
  fillTrackConfig(eConfig, tConfig);

  tConfig.width = eConfig.width;
  tConfig.height = eConfig.height;
}

void fillMp4aConfig(const SEasyTrackConfig& eConfig, SMp4aTrackConfig& tConfig) {
  fillAudioConfig(eConfig, tConfig);

  tConfig.channelCount = eConfig.channelCount;
}

template <typename TTrackConfig, typename TDcr>
void fillDcr(const SEasyTrackConfig& eConfig, TTrackConfig& tConfig) {
  if (!eConfig.decoderConfigRecord.empty()) {
    ilo::ByteBuffer::const_iterator dcrBeg = eConfig.decoderConfigRecord.begin();
    ilo::ByteBuffer::const_iterator dcrEnd = eConfig.decoderConfigRecord.end();
    tConfig.configRecord = ilo::make_unique<TDcr>(dcrBeg, dcrEnd);
  }
}

std::unique_ptr<ITrackWriter> createTrackWriter(CIsobmffWriter& writer,
                                                const SEasyTrackConfig& eConfig) {
  if (eConfig.codecType == ilo::toFcc("mhm1")) {
    SMpeghMhm1TrackConfig tConfig;
    fillMpeghConfig(eConfig, tConfig);
    fillDcr<SMpeghMhm1TrackConfig, config::CMhaDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CMpeghTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("mhm2")) {
    SMpeghMhm2TrackConfig tConfig;
    fillMpeghConfig(eConfig, tConfig);
    fillDcr<SMpeghMhm2TrackConfig, config::CMhaDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CMpeghTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("mha1")) {
    SMpeghMha1TrackConfig tConfig;
    fillMpeghConfig(eConfig, tConfig);
    fillDcr<SMpeghMha1TrackConfig, config::CMhaDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CMpeghTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("mp4a")) {
    SMp4aTrackConfig tConfig;
    fillMp4aConfig(eConfig, tConfig);
    fillDcr<SMp4aTrackConfig, config::CMp4aDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CMp4aTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("avc1")) {
    SAvcTrackConfig tConfig;
    fillVideoConfig(eConfig, tConfig);
    fillDcr<SAvcTrackConfig, config::CAvcDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CAvcTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("hvc1")) {
    SHvc1TrackConfig tConfig;
    fillVideoConfig(eConfig, tConfig);
    fillDcr<SHvc1TrackConfig, config::CHevcDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CHevcTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("hev1")) {
    SHev1TrackConfig tConfig;
    fillVideoConfig(eConfig, tConfig);
    fillDcr<SHev1TrackConfig, config::CHevcDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CHevcTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("vvc1")) {
    SVvc1TrackConfig tConfig;
    fillVideoConfig(eConfig, tConfig);
    fillDcr<SVvc1TrackConfig, config::CVvcDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CVvcTrackWriter>(tConfig);
  }

  if (eConfig.codecType == ilo::toFcc("vvi1")) {
    SVvi1TrackConfig tConfig;
    fillVideoConfig(eConfig, tConfig);
    fillDcr<SVvi1TrackConfig, config::CVvcDecoderConfigRecord>(eConfig, tConfig);
    return writer.trackWriter<CVvcTrackWriter>(tConfig);
  }

  throw std::runtime_error("Track type of " + ilo::toString(eConfig.codecType) +
                           " is currently not supported!");
}

EMp4Type getMp4TypeFromBuffer(const ilo::ByteBuffer& inputBuffer) {
  ilo::ByteBuffer::const_iterator iter = inputBuffer.begin();
  ilo::ByteBuffer::const_iterator end = inputBuffer.end();

  bool hasMoov = false;
  bool hasMoof = false;
  bool hasMdat = false;

  while (iter < end) {
    BoxSizeType boxHeader = tools::getBoxSizeAndType(iter, end);

    if (boxHeader.type == ilo::toFcc("moov")) {
      hasMoov = true;
    } else if (boxHeader.type == ilo::toFcc("moof")) {
      hasMoof = true;
    } else if (boxHeader.type == ilo::toFcc("mdat")) {
      hasMdat = true;
    }

    if (boxHeader.size == 0) {
      break;
    } else {
      iter += static_cast<size_t>(boxHeader.size);
    }
  }

  if (hasMoov && !hasMoof && !hasMdat) {
    return EMp4Type::initSegment;
  } else if (hasMoov && !hasMoof && hasMdat) {
    return EMp4Type::flatMp4;
  } else if (hasMoov && hasMoof && hasMdat) {
    return EMp4Type::fragmentedMp4;
  } else if (!hasMoov && hasMoof && hasMdat) {
    return EMp4Type::mediaSegment;
  } else {
    return EMp4Type::unknown;
  }
}
}  // namespace tools
}  // namespace isobmff
}  // namespace mmt
