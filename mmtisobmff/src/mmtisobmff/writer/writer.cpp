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
 * Content: mmtisobmff writer class(es)
 */

// System includes

// External includes
#include "ilo/memory.h"
#include "ilo/file_utils.h"

// Internal includes
#include "mmtisobmff/writer/writer.h"
#include "service/servicesingleton.h"
#include "writer/writerpimpl.h"
#include "writer/initsegment_tree_builder.h"
#include "writer/trak_userdata_enhancer.h"
#include "writer/config_verifier.h"

namespace mmt {
namespace isobmff {
SMovieConfig::SMovieConfig(SMovieConfig&& otherConf) {
  majorBrand = std::move(otherConf.majorBrand);
  compatibleBrands = std::move(otherConf.compatibleBrands);
  currentTimeInUtc = std::move(otherConf.currentTimeInUtc);
  forceTfdtBoxV1 = std::move(otherConf.forceTfdtBoxV1);
  movieTimeScale = std::move(otherConf.movieTimeScale);
  sidxConfig = std::move(otherConf.sidxConfig);
  iodsConfig = std::move(otherConf.iodsConfig);
  userData = std::move(otherConf.userData);
}

CIsobmffWriter::CIsobmffWriter() {
  setupServicesOnce();
}

CIsobmffWriter::~CIsobmffWriter() {
  close();
}

void CIsobmffWriter::createMediaFragments() {
  ILO_LOG_ERROR("createMediaFragments can only be called from CIsobmffFragFileWriter");
  throw std::runtime_error("createMediaFragments can only be called from CIsobmffFragFileWriter");
}

void CIsobmffWriter::createInitFileSegment(const std::string&) {
  ILO_LOG_ERROR("createInitFileSegment can only be called from CIsobmffFragFileSegWriter");
  throw std::runtime_error(
      "createInitFileSegment can only be called from CIsobmffFragFileSegWriter");
}

void CIsobmffWriter::createMediaFileSegment(const std::string&, const bool&) {
  ILO_LOG_ERROR("createMediaFileSegment can only be called from CIsobmffFragFileSegWriter");
  throw std::runtime_error(
      "createMediaFileSegment can only be called from CIsobmffFragFileSegWriter");
}

ilo::CUniqueBuffer CIsobmffWriter::createInitSegment() {
  ILO_LOG_ERROR("createInitSegment can only be called from CIsobmffFragMemoryWriter");
  throw std::runtime_error("createInitSegment can only be called from CIsobmffFragMemoryWriter");
}

ilo::CUniqueBuffer CIsobmffWriter::createMediaMemSegment(const bool&, const bool&) {
  ILO_LOG_ERROR("createMediaMemSegment can only be called from CIsobmffFragMemoryWriter");
  throw std::runtime_error(
      "createMediaMemSegment can only be called from CIsobmffFragMemoryWriter");
}

ilo::CUniqueBuffer CIsobmffWriter::serialize() {
  ILO_LOG_ERROR("serialize can only be called from CIsobmffMemoryWriter");
  throw std::runtime_error("serialize can only be called from CIsobmffMemoryWriter");
}

void CIsobmffWriter::close() {
  if (p) {
    p->closeAllOutputs();
  }
}

/* ######---BaseFragWriter---###### */

CIsobmffBaseFragWriter::CIsobmffBaseFragWriter(std::unique_ptr<IIsobmffOutput>&& output,
                                               const SMovieConfig& config) {
  const uint64_t CHUNK_SIZE_IN_MS = 1000;

  CMovieConfigVerifier{config};

  uint64_t timeNowUtc = config.currentTimeInUtc;

  if (timeNowUtc == 0) {
    timeNowUtc = tools::currentUTCTime();
  }

  SInitSegmentConfig initConfig;
  initConfig.ftypConfig.compatibleBrands = config.compatibleBrands;
  initConfig.ftypConfig.majorBrand = config.majorBrand;
  initConfig.mvhdConfig.creationTime = timeNowUtc;
  initConfig.mvhdConfig.modificationTime = timeNowUtc;
  initConfig.mvhdConfig.nextTrackID = 1;
  initConfig.mvhdConfig.timescale = config.movieTimeScale;
  CInitSegmentTreeBuilder initSegBuilder(initConfig);
  auto tree = initSegBuilder.build();

  if (config.iodsConfig) {
    box::CObjectDescriptorBox::SIodsBoxWriteConfig iodsConfig;
    iodsConfig.audioProfileLevelIndication = config.iodsConfig->audioProfileLevelIndication;
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
    nodefactory->createNode((*tree)[1], iodsConfig);
  }

  if (config.userData.size() > 0) {
    CTrakUserDataEnhancer{(*tree)[1], config.userData};
  }

  auto sink = ilo::make_unique<CMemorySampleSink>();
  auto interleaver = ilo::make_unique<CTimeAligned>(CHUNK_SIZE_IN_MS);
  auto sampleStore =
      ilo::make_unique<CInterleavingSampleStore>(std::move(sink), std::move(interleaver));

  // Fill pimpl config struct
  Pimpl::SPimplConfig pimplConfig;
  pimplConfig.out = std::move(output);
  pimplConfig.tree = std::move(tree);
  pimplConfig.sampleStore = std::move(sampleStore);
  pimplConfig.timeNowUtc = timeNowUtc;
  pimplConfig.hasFragments = true;
  pimplConfig.forceTfdtV1 = config.forceTfdtBoxV1;
  pimplConfig.writeSidx = config.sidxConfig != nullptr;
  pimplConfig.writeIods = config.iodsConfig != nullptr;
  pimplConfig.chunkSize = CHUNK_SIZE_IN_MS;

  if (pimplConfig.writeSidx) {
    std::string tmpFileName = ilo::getUniqueTmpFilename();
    pimplConfig.tmpOut = ilo::make_unique<CIsobmffFileOutput>(tmpFileName, true);
    pimplConfig.sapType = config.sidxConfig->sapType;
    pimplConfig.tmpFileName = tmpFileName;
  }

  p = ilo::make_unique<Pimpl>(pimplConfig);
}

CIsobmffBaseFragWriter::~CIsobmffBaseFragWriter() {}

/* ######---FragFileWriter---###### */

CIsobmffFragFileWriter::CIsobmffFragFileWriter(const SOutputConfig& outConf,
                                               const SMovieConfig& config)
    : CIsobmffBaseFragWriter(ilo::make_unique<CIsobmffFileOutput>(outConf.outputUri), config) {}

CIsobmffFragFileWriter::~CIsobmffFragFileWriter() {
  try {
    // If the user did not close the library, let's try it here.
    // If this fails, we cannot recover.
    close();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Caught exception during destructor call. This was the original message: %s",
                  e.what());
  }
}

void CIsobmffFragFileWriter::createMediaFragments() {
  if (!p->m_initWritten) {
    p->createInitFragment(nullptr);
    p->m_initWritten = true;
  }

  p->createFragments(nullptr);
}

void CIsobmffFragFileWriter::close() {
  if (p->m_writeSidx && !p->m_closeCalled) {
    p->m_closeCalled = true;
    p->addSidxBox();
  }
  CIsobmffWriter::close();
}

/* ######---FragFileSegWriter---###### */

CIsobmffFragFileSegWriter::CIsobmffFragFileSegWriter(const SMovieConfig& config)
    : CIsobmffBaseFragWriter(nullptr, config) {}

void CIsobmffFragFileSegWriter::createInitFileSegment(const std::string& segOutputUri) {
  auto segOut = ilo::make_unique<CIsobmffFileOutput>(segOutputUri);
  p->createInitFragment(std::move(segOut));
  p->closeCurrentOutput();
}

void CIsobmffFragFileSegWriter::createMediaFileSegment(const std::string& segOutputUri,
                                                       const bool& isLastSegment) {
  auto segOut = ilo::make_unique<CIsobmffFileOutput>(segOutputUri);

  // Create styp box from ftyp box data.
  // If isLastSegment is true, the compatibilty brand 'lmsg' is added.
  ilo::ByteBuffer stypBuff;
  p->createStypBox(stypBuff, isLastSegment);

  // Write styp box at beginning of segment
  segOut->write(stypBuff.begin(), stypBuff.end());

  p->createFragments(std::move(segOut));
  p->closeCurrentOutput();
  p->m_fragTrees.clear();
}

/* ######---FragMemoryWriter---###### */

CIsobmffFragMemoryWriter::CIsobmffFragMemoryWriter(const SMovieConfig& config)
    : CIsobmffBaseFragWriter(ilo::make_unique<CIsobmffMemoryOutput>(), config) {}

ilo::CUniqueBuffer CIsobmffFragMemoryWriter::createInitSegment() {
  // Use new buffer to write init fragment to
  p->createInitFragment(ilo::make_unique<CIsobmffMemoryOutput>());

  return p->output()->read();
}

ilo::CUniqueBuffer CIsobmffFragMemoryWriter::createMediaMemSegment(const bool& useStyp,
                                                                   const bool& isLastSegment) {
  // Use new buffer to write media fragment to
  auto segOut = ilo::make_unique<CIsobmffMemoryOutput>();

  if (useStyp) {
    // Create styp box from ftyp box data.
    // If isLastSegment is true, the compatibilty brand 'lmsg' is added.
    ilo::ByteBuffer stypBuff;
    p->createStypBox(stypBuff, isLastSegment);

    // Write styp box at beginning of segment
    segOut->write(stypBuff.begin(), stypBuff.end());
  }

  p->createFragments(std::move(segOut));
  p->m_fragTrees.clear();

  return p->output()->read();
}

/* ######---BaseWriter---###### */

CIsobmffBaseWriter::CIsobmffBaseWriter(const std::string& outUri, const std::string& tmpUri,
                                       const SMovieConfig& config, const bool memoryWriting) {
  const uint64_t CHUNK_SIZE_IN_MS = 1000;

  ILO_ASSERT(config.sidxConfig == nullptr, "Sidx box writing is only done for fragmented files");
  CMovieConfigVerifier{config};

  uint64_t timeNowUtc = config.currentTimeInUtc;

  if (timeNowUtc == 0) {
    timeNowUtc = tools::currentUTCTime();
  }

  SInitSegmentConfig initConfig;
  initConfig.ftypConfig.compatibleBrands = config.compatibleBrands;
  initConfig.ftypConfig.majorBrand = config.majorBrand;
  initConfig.mvhdConfig.creationTime = timeNowUtc;
  initConfig.mvhdConfig.modificationTime = timeNowUtc;
  initConfig.mvhdConfig.nextTrackID = 1;
  initConfig.mvhdConfig.timescale = config.movieTimeScale;
  CInitSegmentTreeBuilder initSegBuilder(initConfig);
  auto tree = initSegBuilder.build();

  if (config.iodsConfig) {
    box::CObjectDescriptorBox::SIodsBoxWriteConfig iodsConfig;
    iodsConfig.audioProfileLevelIndication = config.iodsConfig->audioProfileLevelIndication;
    auto nodefactory =
        CServiceLocatorSingleton::instance().lock()->getService<INodeFactory>().lock();
    nodefactory->createNode((*tree)[1], iodsConfig);
  }

  if (config.userData.size() > 0) {
    CTrakUserDataEnhancer{(*tree)[1], config.userData};
  }

  std::unique_ptr<IIsobmffOutput> output;
  std::unique_ptr<ISampleSink> sink;
  std::string tmpFileName;

  if (memoryWriting) {
    ILO_ASSERT(outUri.empty() && tmpUri.empty(),
               "Writing plain mp4 data to memory output does not work in combination with file "
               "output paths.");
    output = ilo::make_unique<CIsobmffMemoryOutput>();
    sink = ilo::make_unique<CMemorySampleSink>();
  } else {
    // if user did not specify an output tmpfilename, we need to generate a unique one
    tmpFileName = (tmpUri == "") ? ilo::getUniqueTmpFilename() : tmpUri;

    output = ilo::make_unique<CIsobmffFileOutput>(outUri);
    sink = ilo::make_unique<CFileSampleSink>(tmpFileName);
  }

  auto interleaver = ilo::make_unique<CTimeAligned>(CHUNK_SIZE_IN_MS);
  auto sampleStore =
      ilo::make_unique<CInterleavingSampleStore>(std::move(sink), std::move(interleaver));

  // Fill pimpl config struct
  Pimpl::SPimplConfig pimplConfig;
  pimplConfig.out = std::move(output);
  pimplConfig.tree = std::move(tree);
  pimplConfig.sampleStore = std::move(sampleStore);
  pimplConfig.timeNowUtc = timeNowUtc;
  pimplConfig.hasFragments = false;
  pimplConfig.forceTfdtV1 = config.forceTfdtBoxV1;
  pimplConfig.writeIods = config.iodsConfig != nullptr;
  pimplConfig.chunkSize = CHUNK_SIZE_IN_MS;
  pimplConfig.tmpFileName = tmpFileName;
  p = ilo::make_unique<Pimpl>(pimplConfig);
}

/* ######---FileWriter---###### */

CIsobmffFileWriter::CIsobmffFileWriter(const SOutputConfig& outConf, const SMovieConfig& config)
    : CIsobmffBaseWriter(outConf.outputUri, outConf.tmpUri, config, false) {}

CIsobmffFileWriter::~CIsobmffFileWriter() {
  try {
    // If the user did not close the library, let's try it here.
    // If this fails, we cannot recover.
    close();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Caught exception during destructor call. This was the original message: %s",
                  e.what());
  }

  // Needed for C-Interface. When close() is throwing the c-interface still calls delete.
  p->m_closeCalled = true;
  CIsobmffWriter::close();
}

void CIsobmffFileWriter::close() {
  if (!p->m_closeCalled) {
    p->m_closeCalled = true;
    p->finishNonFragmentedFile();
  }

  CIsobmffWriter::close();
}

/* ######---MemoryWriter---###### */

CIsobmffMemoryWriter::CIsobmffMemoryWriter(const SMovieConfig& config)
    : CIsobmffBaseWriter("", "", config, true) {}

CIsobmffMemoryWriter::~CIsobmffMemoryWriter() {
  try {
    // If the user did not close the library, let's try it here.
    // If this fails, we cannot recover.
    close();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Caught exception during destructor call. This was the original message: %s",
                  e.what());
  }

  // Needed for C-Interface. When close() is throwing the c-interface still calls delete.
  p->m_closeCalled = true;
  CIsobmffWriter::close();
}

ilo::CUniqueBuffer CIsobmffMemoryWriter::serialize() {
  if (!p->m_closeCalled && !p->m_memoryMp4SerializationCalled) {
    p->m_memoryMp4SerializationCalled = true;
    p->finishNonFragmentedFile();
    return p->output()->read();
  }
  ILO_LOG_ERROR(
      "CIsobmffMemoryWriter::serialize() cannot be called multiple times or after "
      "CIsobmffMemoryWriter::close().");
  return nullptr;
}

void CIsobmffMemoryWriter::close() {
  p->m_closeCalled = true;
  CIsobmffWriter::close();
}
}  // namespace isobmff
}  // namespace mmt
