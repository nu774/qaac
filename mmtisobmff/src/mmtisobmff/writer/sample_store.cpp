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
 * Content: store for the sample writer
 */

// System includes
#include <algorithm>
#include <map>
#include <utility>

// project includes
#include "sample_store.h"
#include "common/logging.h"

namespace mmt {
namespace isobmff {
MetaSampleVec CExternalAlignment::align(const MetaSampleVec& metaSamples, const bool&) {
  return metaSamples;
}

MetaSampleVec CTimeAligned::align(const MetaSampleVec& metaSamples, const bool& updateOffsets) {
  auto trackToTimeline = createTimelineMap(metaSamples);

  MetaSampleVec alignedMetaSampleVec;
  std::map<uint32_t, size_t> trackToStatusMap;

  size_t iteration = 1;

  while (alignedMetaSampleVec.size() != metaSamples.size()) {
    for (const auto& dataPoints : trackToTimeline) {
      while (trackToStatusMap[dataPoints.first] < dataPoints.second.size()) {
        const auto& dataPoint = dataPoints.second.at(trackToStatusMap[dataPoints.first]);
        if (static_cast<uint64_t>(dataPoint.decMedTime * 1000) > iteration * m_chunkSizeInMS) {
          break;
        }

        if (updateOffsets) {
          uint64_t newMetaSampleOffset = 0;
          if (alignedMetaSampleVec.size()) {
            auto& lastMetaSample = alignedMetaSampleVec.back();
            newMetaSampleOffset = lastMetaSample.offset + lastMetaSample.size;
          }

          alignedMetaSampleVec.push_back(metaSamples.at(dataPoint.index));
          alignedMetaSampleVec.back().offset = newMetaSampleOffset;
        } else {
          alignedMetaSampleVec.push_back(metaSamples.at(dataPoint.index));
        }

        ++trackToStatusMap[dataPoints.first];
      }
    }

    iteration++;
  }
  return alignedMetaSampleVec;
}

CTimeAligned::TimelineMap CTimeAligned::createTimelineMap(const MetaSampleVec& metaSamples) const {
  TimelineMap trackToTimeline;

  for (size_t i = 0; i < metaSamples.size(); ++i) {
    STimeLine timeline;
    timeline.index = i;
    if (trackToTimeline.find(metaSamples.at(i).trackId) == trackToTimeline.end()) {
      timeline.decMedTime = 0;
    } else {
      ILO_ASSERT(metaSamples[i].timeScale,
                 "MDAT sample aligning needs timescale information, "
                 "but timescale is %d",
                 metaSamples[i].timeScale);
      timeline.decMedTime = trackToTimeline.at(metaSamples.at(i).trackId).back().decMedTime +
                            ((double)metaSamples[i].duration / (double)metaSamples[i].timeScale);
    }
    trackToTimeline[metaSamples[i].trackId].push_back(timeline);
  }

  return trackToTimeline;
}

void CSampleStore::addSample(const CSample& sample, uint32_t trackId, uint32_t timeScale) {
  m_sampleMetaData.push_back(CMetaSample(
      m_sampleMetaData.empty() ? 0 : m_sampleMetaData.back().size + m_sampleMetaData.back().offset,
      sample.rawData.size(), sample.duration, sample.ctsOffset,
      0,  // Only applicable, when reading samples (for filling SSampleExtraInfo)
      sample.fragmentNumber, sample.isSyncSample, trackId, timeScale, sample.sampleGroupInfo));
  m_size += sample.rawData.size();
  m_sink->write(sample.rawData.begin(), sample.rawData.end());
}

MetaSampleVec CSampleStore::getSampleMetadata() const {
  return m_interleaver->align(m_sampleMetaData, true);
}

ilo::CUniqueBuffer CSampleStore::storedSamples(size_t maxBufferSize, uint32_t fragmentNumber) {
  ILO_ASSERT(m_sampleMetaData.size() != 0 && m_size != 0, "No samples to read from sample store");
  ILO_ASSERT(fragmentNumber >= m_lastFragNum,
             "Cannot request older fragments. User wanted %d, last access was to %d",
             fragmentNumber, m_lastFragNum);

  // Only do interleaving if data has changed, since the algorithm it quite expensive
  if (m_alignedMetaData.size() != m_sampleMetaData.size()) {
    m_alignedMetaData = m_interleaver->align(m_sampleMetaData, false);
  }

  // We still have data left (m_size != 0, but we are at the end of the
  // metadata vector => Data might be fragmented, but not all fragments
  // are read => Indicate with nullptr, that reading of this fragment is complete
  if (m_sampleIndex >= m_alignedMetaData.size() && m_lastFragNum <= fragmentNumber) {
    return nullptr;
  }

  uint64_t totalSize = 0;
  uint64_t lastOffset = 0;
  uint64_t lastSize = 0;
  std::vector<std::pair<uint64_t, uint64_t>> byteRanges;

  for (; m_sampleIndex < m_alignedMetaData.size(); ++m_sampleIndex) {
    // Only get samples from that particular "fragment". Plain file would
    // have always the same number
    if (m_alignedMetaData[m_sampleIndex].fragmentNumber < fragmentNumber) {
      continue;
    } else if (m_alignedMetaData[m_sampleIndex].fragmentNumber > fragmentNumber) {
      break;
    }

    // Only get as much samples as the limit allows us to fetch
    if (totalSize + m_alignedMetaData[m_sampleIndex].size > maxBufferSize && maxBufferSize != 0) {
      break;
    }

    // Try to get the copy ranges for the fileReader as continuous as possible
    const auto& currentSample = m_alignedMetaData[m_sampleIndex];
    if ((totalSize != 0) && (lastOffset + lastSize == currentSample.offset)) {
      byteRanges.back().second += currentSample.size;
    } else {
      byteRanges.push_back(std::make_pair(currentSample.offset, currentSample.size));
    }
    lastOffset = currentSample.offset;
    lastSize = currentSample.size;
    totalSize += m_alignedMetaData[m_sampleIndex].size;

    m_lastFragNum = m_alignedMetaData[m_sampleIndex].fragmentNumber;
  }

  ILO_ASSERT(totalSize != 0,
             "Not able to query samples from store. Maybe MaxChunkSize of %d bytes is too small "
             "to hold a single sample.",
             maxBufferSize);

  // Buffer needs to be re-aligned (interleaving does not match)
  ilo::CUniqueBuffer buffer = ilo::make_unique<ilo::ByteBuffer>(static_cast<size_t>(totalSize));
  size_t copiedSize = 0;

  // Get interleaving offsets
  for (const auto& byteRange : byteRanges) {
    auto readBuff =
        m_sink->read(static_cast<size_t>(byteRange.first), static_cast<size_t>(byteRange.second));
    std::copy(readBuff->begin(), readBuff->end(), buffer->begin() + copiedSize);
    copiedSize += readBuff->size();
  }

  ILO_ASSERT(m_size >= copiedSize, "Size mismatch in sample store");
  m_size -= copiedSize;

  return buffer;
}
}  // namespace isobmff
}  // namespace mmt
