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
 * Content: video tools for sample conversion
 */

// System includes
#include <vector>
#include <map>
#include <algorithm>
#include <string>

// External includes
#include "ilo/bytebuffertools.h"

// Internal includes
#include "common/logging.h"
#include "mmtisobmff/helper/videohelpertools.h"
#include "mmtisobmff/helper/commonhelpertools.h"
#include "mmtisobmff/reader/trackreader.h"

namespace mmt {
namespace isobmff {
namespace tools {

void parseVideoSampleNalus(SNaluSample& naluSample, uint32_t lengthSizeMinusOne) {
  naluSample.nalus.clear();
  ilo::ByteBuffer::const_iterator iter = naluSample.sample.rawData.begin();
  uint32_t naluLength = 0;

  while (static_cast<int64_t>(lengthSizeMinusOne + 1) <= naluSample.sample.rawData.end() - iter) {
    switch (lengthSizeMinusOne) {
      case 0:
        naluLength = ilo::readUint8(naluSample.sample.rawData, iter);
        break;
      case 1:
        naluLength = ilo::readUint16(naluSample.sample.rawData, iter);
        break;
      case 3:
        naluLength = ilo::readUint32(naluSample.sample.rawData, iter);
        break;
      default:
        ILO_ASSERT(false, "Nalu length type of %d is not supported", lengthSizeMinusOne);
    }
    ILO_ASSERT(naluLength > 0, "Nalu must have a length greater than zero");
    ILO_ASSERT(static_cast<int64_t>(naluLength) <= naluSample.sample.rawData.end() - iter,
               "Incorrect nalu length or malformed nalu");
    naluSample.addNalu(iter, iter + naluLength);
    iter += naluLength;
  }
  ILO_ASSERT(iter == naluSample.sample.rawData.end(),
             "nalus not parsed to the end - invalid video sample");
}

void parseVideoSampleNalus(SAvcSample& avcSample,
                           const config::CAvcDecoderConfigRecord& configRecord) {
  return parseVideoSampleNalus(avcSample, configRecord.lengthSizeMinusOne());
}

void parseVideoSampleNalus(SHevcSample& hevcSample,
                           const config::CHevcDecoderConfigRecord& configRecord) {
  return parseVideoSampleNalus(hevcSample, configRecord.lengthSizeMinusOne());
}

void parseVideoSampleNalus(SVvcSample& vvcSample,
                           const config::CVvcDecoderConfigRecord& configRecord) {
  return parseVideoSampleNalus(vvcSample, configRecord.lengthSizeMinusOne());
}

static const ilo::ByteBuffer startCodeFour = {0x00, 0x00, 0x00, 0x01};
static const ilo::ByteBuffer startCodeThree = {0x00, 0x00, 0x01};

template <class type>
void convertVideoSampleToAnnexBNalus(const SNaluSample& naluSample, SNaluSample& annexbNaluSample,
                                     uint8_t offset, type predicate) {
  ILO_ASSERT(naluSample.nalus.size() != 0, "Nalu sample does not contain any nalus");

  size_t finalSize = 0;
  for (const auto& nalu : naluSample.nalus) {
    finalSize += predicate(*(nalu.begin() + offset)).size();
    finalSize += nalu.size();
  }

  annexbNaluSample.clear();
  annexbNaluSample.sample.rawData.resize(finalSize);

  ilo::ByteBuffer::iterator iter = annexbNaluSample.sample.rawData.begin();
  for (const auto& nalu : naluSample.nalus) {
    ILO_ASSERT(nalu.size() > 0, "Invalid empty nalu found");

    const ilo::ByteBuffer& startCode = predicate(*(nalu.begin() + offset));
    auto begin = iter;

    std::copy(startCode.begin(), startCode.end(), iter);
    iter += startCode.size();
    std::copy(nalu.begin(), nalu.end(), iter);
    iter += nalu.size();

    annexbNaluSample.addNalu(begin, iter);
  }

  annexbNaluSample.sample.duration = naluSample.sample.duration;
  annexbNaluSample.sample.ctsOffset = naluSample.sample.ctsOffset;
  annexbNaluSample.sample.isSyncSample = naluSample.sample.isSyncSample;
  annexbNaluSample.sample.fragmentNumber = naluSample.sample.fragmentNumber;
  annexbNaluSample.sample.sampleGroupInfo = naluSample.sample.sampleGroupInfo;
}

void convertVideoSampleToAnnexBNalus(const SAvcSample& avcSample, SAvcSample& avcAnnexbSample) {
  convertVideoSampleToAnnexBNalus(avcSample, avcAnnexbSample, 0U, [](uint8_t firstByte) {
    switch (firstByte & 0x1F) {
      case 7:
      case 8:
        return startCodeFour;
      default:
        return startCodeThree;
    }
  });
}

void convertVideoSampleToAnnexBNalus(const SHevcSample& hevcSample, SHevcSample& hevcAnnexbSample) {
  convertVideoSampleToAnnexBNalus(hevcSample, hevcAnnexbSample, 0U, [](uint8_t firstByte) {
    switch ((firstByte & 0x7E) >> 1) {
      case 32:
      case 33:
      case 34:
        return startCodeFour;
      default:
        return startCodeThree;
    }
  });
}

void convertVideoSampleToAnnexBNalus(const SVvcSample& vvcSample, SVvcSample& vvcAnnexbSample) {
  convertVideoSampleToAnnexBNalus(vvcSample, vvcAnnexbSample, 1U, [](uint8_t secondByte) {
    // Nalu type parsing according to ISO/IEC 23090-3 - 7.3.1.2
    switch (secondByte >> 3) {
      case 12:  // OPI_NUT
      case 13:  // DCI_NUT
      case 14:  // VPS_NUT
      case 15:  // SPS_NUT
      case 16:  // PPS_NUT
      case 17:  // PREFIX_APS_NUT
      case 18:  // SUFFIX_APS_NUT
        return startCodeFour;
      default:
        return startCodeThree;
    }
  });
}

size_t requiredAnnexbNaluSampleSize(const config::CAvcDecoderConfigRecord& configRecord) {
  size_t sum = 0;

  for (const auto& sps : configRecord.sequenceParameterSets()) {
    sum += sps.size() + startCodeFour.size();
  }

  for (const auto& pps : configRecord.pictureParameterSets()) {
    sum += pps.size() + startCodeFour.size();
  }

  for (const auto& spsExt : configRecord.sequenceParameterExtSets()) {
    sum += spsExt.size() + startCodeFour.size();
  }

  return sum;
}

size_t requiredAnnexbNaluSampleSize(const config::CHevcDecoderConfigRecord& configRecord) {
  size_t sum = 0;

  for (const auto& nonVclNalus : configRecord.nonVclArrays()) {
    for (const auto& nonVclNalu : nonVclNalus.nalus) {
      sum += nonVclNalu.size() + startCodeFour.size();
    }
  }

  return sum;
}

size_t requiredAnnexbNaluSampleSize(const config::CVvcDecoderConfigRecord& configRecord) {
  size_t sum = 0;

  for (const auto& nonVclNalus : configRecord.nonVclArrays()) {
    for (const auto& nonVclNalu : nonVclNalus.nalus) {
      sum += nonVclNalu.size() + startCodeFour.size();
    }
  }

  return sum;
}

void populateAnnexB(const ilo::ByteBuffer& nonvcl, ilo::ByteBuffer::iterator& rawDataPosition,
                    SNaluSample& out) {
  std::copy(startCodeFour.begin(), startCodeFour.end(), rawDataPosition);
  std::copy(nonvcl.begin(), nonvcl.end(), rawDataPosition + startCodeFour.size());

  out.addNalu(rawDataPosition, rawDataPosition + startCodeFour.size() + nonvcl.size());
  rawDataPosition += startCodeFour.size() + nonvcl.size();
}

void convertNonVclNalusToAnnexBNalus(const config::CAvcDecoderConfigRecord& configRecord,
                                     SAvcSample& avcAnnexbSample) {
  avcAnnexbSample.nalus.clear();
  avcAnnexbSample.sample.rawData.clear();

  avcAnnexbSample.sample.rawData.resize(requiredAnnexbNaluSampleSize(configRecord));
  auto currentPosition = avcAnnexbSample.sample.rawData.begin();

  for (const auto& sps : configRecord.sequenceParameterSets()) {
    populateAnnexB(sps, currentPosition, avcAnnexbSample);
  }

  for (const auto& pps : configRecord.pictureParameterSets()) {
    populateAnnexB(pps, currentPosition, avcAnnexbSample);
  }

  for (const auto& spsExt : configRecord.sequenceParameterExtSets()) {
    populateAnnexB(spsExt, currentPosition, avcAnnexbSample);
  }
}

void convertNonVclNalusToAnnexBNalus(const config::CHevcDecoderConfigRecord& configRecord,
                                     SHevcSample& hevcAnnexbSample) {
  hevcAnnexbSample.nalus.clear();
  hevcAnnexbSample.sample.rawData.clear();

  hevcAnnexbSample.sample.rawData.resize(requiredAnnexbNaluSampleSize(configRecord));
  auto currentPosition = hevcAnnexbSample.sample.rawData.begin();

  for (const auto& nonVclNalus : configRecord.nonVclArrays()) {
    for (const auto& nonVclNalu : nonVclNalus.nalus) {
      populateAnnexB(nonVclNalu, currentPosition, hevcAnnexbSample);
    }
  }
}

void convertNonVclNalusToAnnexBNalus(const config::CVvcDecoderConfigRecord& configRecord,
                                     SVvcSample& vvcAnnexbSample) {
  vvcAnnexbSample.nalus.clear();
  vvcAnnexbSample.sample.rawData.clear();

  vvcAnnexbSample.sample.rawData.resize(requiredAnnexbNaluSampleSize(configRecord));
  auto currentPosition = vvcAnnexbSample.sample.rawData.begin();

  for (const auto& nonVclNalus : configRecord.nonVclArrays()) {
    for (const auto& nonVclNalu : nonVclNalus.nalus) {
      populateAnnexB(nonVclNalu, currentPosition, vvcAnnexbSample);
    }
  }
}

void fillCSampleMetaData(const SVideoNalus& videoNalus, CSample& sample) {
  const auto& nalusMetaData = videoNalus.getMetaData();
  sample.ctsOffset = nalusMetaData.ctsOffset;
  sample.duration = nalusMetaData.duration;
  sample.fragmentNumber = nalusMetaData.fragmentNumber;
  sample.isSyncSample = nalusMetaData.isSyncSample;
  sample.sampleGroupInfo = nalusMetaData.sampleGroupInfo;
}

uint32_t calculateStartCodeLength(const ilo::ByteBuffer& nalu) {
  if (std::search(nalu.begin(), nalu.end(), startCodeFour.begin(), startCodeFour.end()) !=
      nalu.end()) {
    return 4;
  } else if (std::search(nalu.begin(), nalu.end(), startCodeThree.begin(), startCodeThree.end()) !=
             nalu.end()) {
    return 3;
  }
  ILO_LOG_ERROR("No AnnexB startcode found, but nalus data struct reported AnnexB format");
  throw std::runtime_error(
      "No AnnexB startcode found, but nalus data struct reported AnnexB format");
}

size_t computeVideoSampleSize(const SVideoNalus& videoNalus, uint8_t lengthPrefixSize) {
  size_t totalSize = 0;
  size_t offset = 0;

  const auto& nalus = videoNalus.getNalus();
  for (const auto& nalu : nalus) {
    if (videoNalus.isAnnexB()) {
      offset = calculateStartCodeLength(nalu);
      ILO_ASSERT(nalu.size() > offset,
                 "Video Nalu has a malformed startcode/payload structure. "
                 "Startcode size is %d, payload size is %d",
                 offset, nalu.size());
    }

    totalSize += nalu.size() - offset;
    totalSize += lengthPrefixSize;
  }

  return totalSize;
}

void convertGeneralVideoNalusToVideoSample(const SVideoNalus& videoNalus, uint8_t lengthPrefixSize,
                                           SNaluSample& naluSample) {
  naluSample.clear();
  fillCSampleMetaData(videoNalus, naluSample.sample);
  naluSample.sample.rawData.resize(computeVideoSampleSize(videoNalus, lengthPrefixSize));

  ilo::ByteBuffer::iterator iter = naluSample.sample.rawData.begin();
  ilo::ByteBuffer::iterator end = naluSample.sample.rawData.end();

  const auto& nalus = videoNalus.getNalus();

  for (const auto& nalu : nalus) {
    size_t offset = 0;

    if (videoNalus.isAnnexB()) {
      offset = calculateStartCodeLength(nalu);
    }

    ILO_ASSERT(nalu.size() > offset,
               "Video Nalu has a malformed startcode/paload structure. "
               "Startcode size is %d, payload size is %d",
               offset, nalu.size());
    size_t naluSize = nalu.size() - offset;

    switch (lengthPrefixSize) {
      case 1:
        ILO_ASSERT(naluSize <= std::numeric_limits<uint8_t>::max(),
                   "Nalu size of %d is bigger than signaled lengthPrefixSize of %d", naluSize,
                   lengthPrefixSize);
        ilo::writeUint8(iter, end, static_cast<uint8_t>(naluSize));
        break;
      case 2:
        ILO_ASSERT(naluSize <= std::numeric_limits<uint16_t>::max(),
                   "Nalu size of %d is bigger than signaled lengthPrefixSize of %d", naluSize,
                   lengthPrefixSize);
        ilo::writeUint16(iter, end, static_cast<uint16_t>(naluSize));
        break;
      case 4:
        ILO_ASSERT(naluSize <= std::numeric_limits<uint32_t>::max(),
                   "Nalu size of %d is bigger than signaled lengthPrefixSize of %d", naluSize,
                   lengthPrefixSize);
        ilo::writeUint32(iter, end, static_cast<uint32_t>(naluSize));
        break;
      default:
        ILO_ASSERT(false, "Nalu length type of %d is not supported", lengthPrefixSize);
    }
    std::copy(nalu.begin() + offset, nalu.end(), iter);
    naluSample.addNalu(iter, iter + naluSize);
    iter += naluSize;
  }

  ILO_ASSERT(iter == naluSample.sample.rawData.end(),
             "Resulting video sample is smaller than source nalu data");
}

void convertAnnexbByteBufferToVideoSample(const ilo::ByteBuffer& annexbBuffer,
                                          const SVideoNalus::SMetaData& metaData,
                                          uint8_t lengthPrefixSize, SNaluSample& naluSample) {
  std::vector<ilo::ByteBuffer::const_iterator> naluBegin;
  auto iterator = annexbBuffer.begin();
  while (iterator != annexbBuffer.end()) {
    iterator = std::min(
        std::search(iterator, annexbBuffer.end(), startCodeThree.begin(), startCodeThree.end()),
        std::search(iterator, annexbBuffer.end(), startCodeFour.begin(), startCodeFour.end()));
    if (iterator != annexbBuffer.end()) {
      naluBegin.push_back(iterator);
      iterator += 3;
    }
  }

  SVideoNalus videoNalus(metaData, true);

  ILO_ASSERT(naluBegin.size() > 0, "AnnexB buffer did not contain any startcodes");

  for (size_t i = 0; i < naluBegin.size(); i++) {
    auto begin = naluBegin[i];
    auto end = (i == naluBegin.size() - 1) ? annexbBuffer.end() : naluBegin[i + 1];
    videoNalus.addNalu(ilo::ByteBuffer(begin, end));
  }

  convertGeneralVideoNalusToVideoSample(videoNalus, lengthPrefixSize, naluSample);
}

void convertAnnexbByteBufferToVideoSampleBuffer(const ilo::ByteBuffer& annexbBuffer,
                                                uint8_t lengthPrefixSize,
                                                ilo::ByteBuffer& sampleBuffer) {
  // We can leave the metadata empty since we are only interested in the buffer conversion
  SVideoNalus::SMetaData metaData;
  SNaluSample naluSample;
  convertAnnexbByteBufferToVideoSample(annexbBuffer, metaData, lengthPrefixSize, naluSample);

  // Will will move the buffer out, so the nalu vector is not valid anymore.
  naluSample.nalus.clear();

  // We don't want to copy the buffer
  sampleBuffer = std::move(naluSample.sample.rawData);
}

void fillNonVclNalusIntoConfigRecord(const SAvcNonVclNalus& nonVlcNalus,
                                     config::CAvcDecoderConfigRecord& configRecord) {
  config::CAvcDecoderConfigRecord::SAvcParamVector sps;
  config::CAvcDecoderConfigRecord::SAvcParamVector pps;
  config::CAvcDecoderConfigRecord::SAvcParamVector spsExt;

  const auto& nalus = nonVlcNalus.getNalus();
  for (const auto& nalu : nalus) {
    size_t offset = 0;

    if (nonVlcNalus.isAnnexB()) {
      offset = calculateStartCodeLength(nalu);
    }

    switch (nalu.at(offset) & 0x1F) {
      case 6:
        ILO_LOG_WARNING("AVC Nalu type of %d is not implemented", (nalu.at(offset) & 0x1F));
        break;
      case 7:
        sps.push_back(ilo::ByteBuffer(nalu.begin() + offset, nalu.end()));
        break;
      case 8:
        pps.push_back(ilo::ByteBuffer(nalu.begin() + offset, nalu.end()));
        break;
      case 13:
        spsExt.push_back(ilo::ByteBuffer(nalu.begin() + offset, nalu.end()));
        break;
      default:
        ILO_ASSERT(false, "AVC Nalu type of %d is not implemented", (nalu.at(offset) & 0x1F));
    }
  }

  if (sps.size()) {
    configRecord.setSequenceParameterSets(sps);
  }

  if (pps.size()) {
    configRecord.setPictureParameterSets(pps);
  }

  if (spsExt.size()) {
    configRecord.setSequenceParameterExtSets(spsExt);
  }
}

void fillNonVclNalusIntoConfigRecord(const SHevcNonVclNalus& nonVclNalus,
                                     config::CHevcDecoderConfigRecord& configRecord,
                                     bool allArrayComplete) {
  config::CHevcDecoderConfigRecord::NonVclArrays nonVclArrays;
  std::map<uint8_t, uint32_t> naluTypeToVectorIndex;

  const auto& nalus = nonVclNalus.getNalus();
  for (const auto& nalu : nalus) {
    size_t offset = 0;

    if (nonVclNalus.isAnnexB()) {
      offset = calculateStartCodeLength(nalu);
    }

    uint8_t naluType = static_cast<uint8_t>((nalu.at(offset) & 0x7E) >> 1);
    if (naluTypeToVectorIndex.find(naluType) == naluTypeToVectorIndex.end()) {
      config::CHevcDecoderConfigRecord::SHevcArray hevcArray;

      hevcArray.arrayCompleteness = allArrayComplete;
      hevcArray.naluType = naluType;
      nonVclArrays.push_back(hevcArray);
      naluTypeToVectorIndex[naluType] = static_cast<uint32_t>(nonVclArrays.size() - 1);
    }

    auto& hevcArray = nonVclArrays.at(naluTypeToVectorIndex[naluType]);
    hevcArray.nalus.push_back(ilo::ByteBuffer(nalu.begin() + offset, nalu.end()));
  }

  configRecord.setNonVclArrays(nonVclArrays);
}

void fillNonVclNalusIntoConfigRecord(const SVvcNonVclNalus& nonVclNalus,
                                     config::CVvcDecoderConfigRecord& configRecord,
                                     bool allArrayComplete) {
  config::CVvcDecoderConfigRecord::NonVclArrays nonVclArrays;
  std::map<uint8_t, uint32_t> naluTypeToVectorIndex;

  const auto& nalus = nonVclNalus.getNalus();
  for (const auto& nalu : nalus) {
    size_t offset = 0;

    if (nonVclNalus.isAnnexB()) {
      offset = calculateStartCodeLength(nalu);
    }

    // Nalu type parsing according to ISO/IEC 23090-3 - 7.3.1.2
    uint8_t naluType = static_cast<uint8_t>(nalu.at(offset + 1) >> 3);
    if (naluTypeToVectorIndex.find(naluType) == naluTypeToVectorIndex.end()) {
      config::CVvcDecoderConfigRecord::SVvcArray vvcArray;

      vvcArray.arrayCompleteness = allArrayComplete;
      vvcArray.naluType = naluType;
      nonVclArrays.push_back(vvcArray);
      naluTypeToVectorIndex[naluType] = static_cast<uint32_t>(nonVclArrays.size() - 1);
    }

    auto& vvcArray = nonVclArrays.at(naluTypeToVectorIndex[naluType]);
    vvcArray.nalus.emplace_back(nalu.begin() + offset, nalu.end());
  }

  configRecord.setNonVclArrays(nonVclArrays);
}
}  // namespace tools
}  // namespace isobmff
}  // namespace mmt
