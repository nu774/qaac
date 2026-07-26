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
 * Content: sample reader class
 */

// System includes
#include <cmath>

// External includes
#include "ilo/string_utils.h"

// Internal includes
#include "samplereader.h"
#include "common/logging.h"
#include "pimpl.h"
#include "box/trunbox.h"
#include "box/tfhdbox.h"

namespace mmt {
namespace isobmff {
CSampleReader::CSampleReader(std::unique_ptr<IIsobmffInput>&& input,
                             const CTrackSampleInfo& trackSampleInfo)
    : m_input(std::move(input)),
      m_trackSampleInfo(trackSampleInfo),
      m_currentSampleNrToRead(0),
      m_maxSampleSize(0) {
  for (const auto& metaSample : m_trackSampleInfo) {
    m_maxSampleSize = std::max(metaSample.size, m_maxSampleSize);
  }
}

uint64_t CSampleReader::maxSampleSize() {
  return m_maxSampleSize;
}

SSampleExtraInfo CSampleReader::nextSample(CSample& sample, bool preallocate) {
  SSampleExtraInfo sExtraInfo;
  sample.clear();
  if (m_currentSampleNrToRead >= m_trackSampleInfo.size()) {
    return sExtraInfo;
  }

  CMetaSample currentMetadataSample = m_trackSampleInfo[m_currentSampleNrToRead];

  sample.duration = currentMetadataSample.duration;
  sample.ctsOffset = currentMetadataSample.ctsOffset;
  sample.isSyncSample = currentMetadataSample.isSyncSample;
  sample.fragmentNumber = currentMetadataSample.fragmentNumber;
  sample.sampleGroupInfo = currentMetadataSample.sampleGroupInfo;

  if (preallocate && sample.rawData.capacity() < static_cast<size_t>(maxSampleSize())) {
    sample.rawData.reserve(static_cast<size_t>(maxSampleSize()));
  }

  ILO_ASSERT(currentMetadataSample.size > 0, "Metadata sample has a size of 0");
  sample.rawData.resize(static_cast<size_t>(currentMetadataSample.size));

  m_input->seek(currentMetadataSample.offset, SeekingOrigin::beg);
  auto readCount = m_input->read(sample.rawData.begin(), sample.rawData.end());
  sample.rawData.resize(readCount);
  ILO_ASSERT_WITH(sample.rawData.size() == static_cast<size_t>(currentMetadataSample.size),
                  std::length_error, "sample truncated");

  m_currentSampleNrToRead++;

  if (currentMetadataSample.dtsValue + currentMetadataSample.ctsOffset < 0) {
    sExtraInfo.timestamp = CIsoTimestamp();
    ILO_LOG_ERROR("PTS issue. CTS offset of %d and DTS value of %d result in negative PTS.",
                  currentMetadataSample.ctsOffset, currentMetadataSample.dtsValue);
  } else {
    sExtraInfo.timestamp =
        CIsoTimestamp(currentMetadataSample.timeScale,
                      currentMetadataSample.dtsValue + currentMetadataSample.ctsOffset,
                      currentMetadataSample.dtsValue);
  }
  return sExtraInfo;
}

SSampleExtraInfo CSampleReader::sampleByIndex(size_t sampleIndex, CSample& sample,
                                              bool preallocate) {
  m_currentSampleNrToRead = sampleIndex;
  return nextSample(sample, preallocate);
}

SSampleExtraInfo CSampleReader::sampleByTimestamp(const SSeekConfig& seekConfig, CSample& sample,
                                                  bool preallocate) {
  m_currentSampleNrToRead = sampleIndexForTimestamp(seekConfig);
  return nextSample(sample, preallocate);
}

SSampleExtraInfo CSampleReader::resolveTimestamp(const SSeekConfig& seekConfig) const {
  SSampleExtraInfo sExtraInfo;
  auto targetFrameIndex = sampleIndexForTimestamp(seekConfig);
  if (targetFrameIndex >= m_trackSampleInfo.size()) {
    return sExtraInfo;
  }
  CMetaSample currentMetadataSample = m_trackSampleInfo[targetFrameIndex];
  if (currentMetadataSample.dtsValue + currentMetadataSample.ctsOffset < 0) {
    sExtraInfo.timestamp = CIsoTimestamp();
    ILO_LOG_ERROR("PTS issue. CTS offset of %d and DTS value of %d result in negative PTS.",
                  currentMetadataSample.ctsOffset, currentMetadataSample.dtsValue);
  } else {
    sExtraInfo.timestamp =
        CIsoTimestamp(currentMetadataSample.timeScale,
                      currentMetadataSample.dtsValue + currentMetadataSample.ctsOffset,
                      currentMetadataSample.dtsValue);
  }
  return sExtraInfo;
}

std::size_t CSampleReader::sampleIndexForTimestamp(const SSeekConfig& seekConfig) const {
  ILO_ASSERT(seekConfig.seekMode != ESampleSeekMode::invalid,
             "Invalid seek mode specified by user");
  ILO_ASSERT(seekConfig.seekPoint.isValid(), "Invalid (empty) seekpoint found.");

  uint64_t accDuration = 0;
  size_t frameIndex = 0;

  size_t syncSampleIndex = 0;
  size_t syncSampleIndexNMinusOne = 0;

  bool foundUserSeekPosition = false;
  size_t userSeekPositionIndex = 0;

  double userSeekTime = seekConfig.seekPoint.duration() / (double)seekConfig.seekPoint.timescale();
  double currentTime = 0;

  for (const auto& metaSample : m_trackSampleInfo) {
    if (metaSample.isSyncSample) {
      syncSampleIndexNMinusOne = syncSampleIndex;
      syncSampleIndex = frameIndex;
      if (foundUserSeekPosition) {
        break;
      }
    }

    // Check if we reached user time
    currentTime = accDuration / (double)metaSample.timeScale;
    if (currentTime >= userSeekTime && !foundUserSeekPosition) {
      userSeekPositionIndex = frameIndex;
      foundUserSeekPosition = true;
    }

    accDuration += metaSample.duration;
    frameIndex++;
  }

  // Position not found. Try to use what we have (first or last position)
  if (!foundUserSeekPosition) {
    userSeekPositionIndex = frameIndex;
  }

  // Evaluate the mode and what fits better

  size_t targetFrameIndex = 0;
  switch (seekConfig.seekMode) {
    case ESampleSeekMode::nearestSyncSample:
      if (std::abs(static_cast<int64_t>(syncSampleIndex) -
                   static_cast<int64_t>(userSeekPositionIndex)) <=
          std::abs(static_cast<int64_t>(userSeekPositionIndex) -
                   static_cast<int64_t>(syncSampleIndexNMinusOne))) {
        targetFrameIndex = syncSampleIndex;
      } else {
        targetFrameIndex = syncSampleIndexNMinusOne;
      }
      break;
    case ESampleSeekMode::nextSyncSampleGreater:
      if (userSeekPositionIndex <= syncSampleIndexNMinusOne) {
        targetFrameIndex = syncSampleIndexNMinusOne;
      } else {
        targetFrameIndex = syncSampleIndex;
      }
      break;
    case ESampleSeekMode::lastSyncSampleSmaller:
      if (userSeekPositionIndex >= syncSampleIndex) {
        targetFrameIndex = syncSampleIndex;
      } else {
        targetFrameIndex = syncSampleIndexNMinusOne;
      }
      break;
    default:
      ILO_ASSERT(false, "Invalid sample seek mode specified");
      break;
  }

  return targetFrameIndex;
}
}  // namespace isobmff
}  // namespace mmt
