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

#pragma once

// System includes
#include <vector>
#include <memory>

// external includes
#include "ilo/common_types.h"
#include "ilo/memory.h"

// project includes
#include "mmtisobmff/writer/output.h"
#include "mmtisobmff/types.h"
#include "common/tracksampleinfo.h"

namespace mmt {
namespace isobmff {
using MetaSampleVec = std::vector<CMetaSample>;

/*## Sample Sink Implementations ##*/

struct ISampleSink {
  virtual ~ISampleSink() {}
  virtual void write(const ilo::ByteBuffer::const_iterator& inBegin,
                     const ilo::ByteBuffer::const_iterator& inEnd) = 0;
  virtual ilo::CUniqueBuffer read(size_t offset = 0, size_t size = 0) = 0;
};

struct CFileSampleSink : public ISampleSink, public CIsobmffFileOutput {
  CFileSampleSink(std::string filename) : CIsobmffFileOutput(filename, true) {}

  void write(const ilo::ByteBuffer::const_iterator& inBegin,
             const ilo::ByteBuffer::const_iterator& inEnd) {
    CIsobmffFileOutput::write(inBegin, inEnd);
  }

  ilo::CUniqueBuffer read(size_t offset = 0, size_t size = 0) {
    return CIsobmffFileOutput::read(offset, size);
  }
};

struct CMemorySampleSink : public ISampleSink, public CIsobmffMemoryOutput {
  void write(const ilo::ByteBuffer::const_iterator& inBegin,
             const ilo::ByteBuffer::const_iterator& inEnd) {
    CIsobmffMemoryOutput::write(inBegin, inEnd);
  }

  ilo::CUniqueBuffer read(size_t offset = 0, size_t size = 0) {
    return CIsobmffMemoryOutput::read(offset, size);
  }
};

/*## Sample Interleaver Implementations ##*/

struct ISampleInterleaver {
  virtual ~ISampleInterleaver() {}

  virtual MetaSampleVec align(const MetaSampleVec& metaSamples, const bool& updateOffsets) = 0;
};

struct CExternalAlignment : public ISampleInterleaver {
  MetaSampleVec align(const MetaSampleVec& metaSamples, const bool& updateOffsets) override;
};

struct CTimeAligned : public ISampleInterleaver {
  struct STimeLine {
    size_t index = 0;
    double decMedTime = 0.0;
  };

  using TimelineMap = std::map<uint32_t, std::vector<STimeLine>>;

  CTimeAligned(const uint64_t& chunkSizeInMs) : m_chunkSizeInMS(chunkSizeInMs) {}

  MetaSampleVec align(const MetaSampleVec& metaSamples, const bool& updateOffsets) override;

 private:
  TimelineMap createTimelineMap(const MetaSampleVec& metaSamples) const;

 private:
  uint64_t m_chunkSizeInMS = 0;
};

/*## Sample Store Implementations ##*/

struct ISampleStore {
  virtual ~ISampleStore() {}

  virtual void addSample(const CSample& sample, uint32_t trackId, uint32_t timeScale) = 0;

 protected:
  std::unique_ptr<ISampleSink> m_sink = nullptr;
  std::unique_ptr<ISampleInterleaver> m_interleaver = nullptr;
  MetaSampleVec m_sampleMetaData;
  MetaSampleVec m_alignedMetaData;
  size_t m_sampleIndex = 0;
  uint32_t m_lastFragNum = 0;
};

struct CSampleStore : ISampleStore {
  CSampleStore(std::unique_ptr<ISampleSink>&& sink) {
    m_sink = std::move(sink);
    m_interleaver = ilo::make_unique<CExternalAlignment>();
    m_size = 0;
  }

  void addSample(const CSample& sample, uint32_t trackId, uint32_t timeScale);

  MetaSampleVec getSampleMetadata() const;
  ilo::CUniqueBuffer storedSamples(size_t maxBufferSize, uint32_t fragmentNumber = 0);

  // returns the size of the samples in the store that are not yet read.
  size_t getStoreSize() const { return m_size; }

 private:
  size_t m_size;
};

struct CInterleavingSampleStore : public CSampleStore {
  CInterleavingSampleStore(std::unique_ptr<ISampleSink>&& sink,
                           std::unique_ptr<ISampleInterleaver>&& interleaver)
      : CSampleStore(std::move(sink)) {
    m_interleaver = std::move(interleaver);
  }
};
}  // namespace isobmff
}  // namespace mmt
