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
 * Content: C interface for ISO BMFF library
 */

// System includes
#include <string>
#include <iostream>
#include <vector>
#include <exception>
#include <cmath>

// Internal includes
#include "common/logging.h"
#include "mmtisobmff/mmtisobmff_c.h"
#include "mmtisobmff/types.h"
#include "mmtisobmff/reader/input.h"
#include "mmtisobmff/reader/reader.h"
#include "mmtisobmff/reader/trackreader.h"
#include "mmtisobmff/configdescriptor/mha_decoderconfigrecord.h"
#include "mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h"
#include "mmtisobmff/writer/output.h"
#include "mmtisobmff/writer/writer.h"
#include "mmtisobmff/writer/trackwriter.h"
#include "mmtisobmff/logging.h"
#include "mmtisobmff/specificboxinfo.h"

using namespace ilo;
using namespace mmt::isobmff;

/* private structs for reading interface */
struct GenericTrackReader {
  std::unique_ptr<CGenericTrackReader> genericTrackReader = nullptr;
};

struct LoudnessInfo {
  ilo::ByteBuffer ludtInitInfo;
  ilo::ByteBuffer ludtFragInfo;
};

/* public structs for reading interface (hidden) */
struct ISOBMFF_Reader {
  std::unique_ptr<CIsobmffReader> isobmffReader = nullptr;
  std::vector<std::unique_ptr<TrackReader>> trackVec;
  std::unique_ptr<SDrcInfo> drcInfo = nullptr;
  std::unique_ptr<SIodsInfo> iodsInfo = nullptr;
  CMovieInfo movieInfo;
};

struct TrackReader {
  ISOBMFF_Reader* isobmff = nullptr;
  uint32_t trackIndex = 0;
  GenericTrackReader trackReader;
  CTrackInfo trackInfo;
  ilo::ByteBuffer dsc;
  LoudnessInfo loudnessInfo;
  std::vector<ilo::ByteBuffer> userData;
  std::vector<uint8_t> mpeghProfileAndLevelCompatibleSets;
};

/* public structs for writing interface (hidden) */
struct SidxConfig {
  ESapType sapType = ESapType::SapTypeInvalid;
};

struct IodsConfig {
  uint8_t audioProfileLevelIndication =
      0xFF;  // 0xFF means "no audio capability required" as described in ISO/IEC 14496-3
};

struct MovieConfig {
  ilo::Fourcc majorBrand = ilo::toFcc("0000");
  std::vector<ilo::Fourcc> compatibleBrands;
  uint32_t movieTimeScale = 600;
  uint64_t currentTimeInUTC = 0;
  bool force64bitMDT = false;
  std::unique_ptr<SidxConfig> sidxConfig = nullptr;
  std::unique_ptr<IodsConfig> iodsConfig = nullptr;
  std::vector<ByteBuffer> userData;
};

struct EditListEntry {
  //! duration of the edit in ticks based on movie time scale @see CMovieInfo
  uint64_t segmentDuration = 0;
  //! start time within the media of the edit in media time scale @see CTrackInfo
  int64_t mediaTime = 0;
  //! relative rate at which to play the media in the edit (0 specifies a dwell, should be 1
  //! otherwise)
  float mediaRate = 1.0;
};

struct ISOBMFF_Writer {
  std::shared_ptr<CIsobmffWriter> isobmffWriter = nullptr;
  std::unique_ptr<ilo::ByteBuffer> buffer = nullptr;
  uint32_t usedMovieTimescale = 0;
  bool isFragmentedWriter = false;
};

struct TrackConfig {
  Codec codec = Codec::undefined;
  uint32_t trackTimeScale = 0;
  uint32_t trackId = 0;
  uint32_t sampleRate = 0;
  uint16_t channelCount = 0;
  ilo::IsoLang language = ilo::toIsoLang("und");
  std::unique_ptr<ilo::ByteBuffer> dsc = nullptr;
  SSampleGroupInfo defaultSampleGroup;
  bool enableMpeghMultiStream = false;
  std::vector<uint8_t> mpegHPLcompatibleSets;
};

struct TrackWriter {
  std::unique_ptr<ITrackWriter> trackWriter = nullptr;
  uint32_t usedMovieTimescale = 0;
  bool isFragmentedWriter = false;
};

struct MpeghDecoderConfigRecord {
  uint8_t m_configurationVersion = 0;
  uint8_t m_mpegh3daProfileLevelIndication = 0;
  uint8_t m_referenceChannelLayout = 0;
  ilo::ByteBuffer m_mpegh3daConfig;
};

struct Mp4aDecoderConfigRecord {
  uint32_t m_maxBitrate = 0;
  uint32_t m_avgBitrate = 0;
  uint32_t m_bufferSizeDB = 0;
  ilo::ByteBuffer m_asc;
};

struct MpeghMultiStreamConfig {
  // This (currently) empty config is used to detect,
  // if user wants to enable multi stream support.
  // Optional configs about addional multistream boxes can
  // also be placed here.
};

/* public structs for common interface (hidden) */
struct Sample : CSample {};

/* ------------Private methods------------------- */

Codec_C convertCodecEnum(Codec codec) {
  switch (codec) {
    case Codec::mp4a:
      return Codec_Mp4a;
    case Codec::mpegh_mha:
      return Codec_Mpegh_Mha;
    case Codec::mpegh_mhm:
      return Codec_Mpegh_Mhm;
    case Codec::mp4v:
      return Codec_Mp4v;
    case Codec::avc:
      return Codec_Avc;
    case Codec::hevc:
      return Codec_Hevc;
    case Codec::jxs:
      return Codec_Jxs;
    case Codec::vvc:
      return Codec_Vvc;
    case Codec::undefined:
    default:
      return Codec_Undefined;
  }
}

Codec convertCodecCEnum(Codec_C codec) {
  switch (codec) {
    case Codec_C::Codec_Mp4a:
      return Codec::mp4a;
    case Codec_C::Codec_Mpegh_Mha:
      return Codec::mpegh_mha;
    case Codec_C::Codec_Mpegh_Mhm:
      return Codec::mpegh_mhm;
    case Codec_C::Codec_Mp4v:
      return Codec::mp4v;
    case Codec_C::Codec_Avc:
      return Codec::avc;
    case Codec_C::Codec_Hevc:
      return Codec::hevc;
    case Codec_C::Codec_Jxs:
      return Codec::jxs;
    case Codec_C::Codec_Vvc:
      return Codec::vvc;
    case Codec_C::Codec_Undefined:
    default:
      return Codec::undefined;
  }
}

TrackType_C convertTrackTypeEnum(TrackType trackType) {
  switch (trackType) {
    case TrackType::audio:
      return TrackType_Audio;
    case TrackType::video:
      return TrackType_Video;
    case TrackType::hint:
      return TrackType_Hint;
    case TrackType::undefined:
    default:
      return TrackType_Undefined;
  }
}

SampleGroup_C convertSampleGroupEnum(SampleGroupType sampleGroup) {
  switch (sampleGroup) {
    case SampleGroupType::none:
      return SampleGroup_None;
    case SampleGroupType::roll:
      return SampleGroup_Roll;
    case SampleGroupType::prol:
      return SampleGroup_Prol;
    case SampleGroupType::sap:
      return SampleGroup_Sap;
    default:
      return SampleGroup_Undefined;
  }
}

SampleGroupType convertSampleGroupCEnum(SampleGroup_C sampleGroup) {
  switch (sampleGroup) {
    case SampleGroup_None:
      return SampleGroupType::none;
    case SampleGroup_Roll:
      return SampleGroupType::roll;
    case SampleGroup_Prol:
      return SampleGroupType::prol;
    case SampleGroup_Sap:
      return SampleGroupType::sap;
    default:
      return SampleGroupType::undefined;
  }
}

ESapType convertSapTypeCEnum(SapType_C sapType) {
  switch (sapType) {
    case SapType_C::SapType1:
      return ESapType::SapType1;
    case SapType_C::SapType2:
      return ESapType::SapType2;
    case SapType_C::SapType3:
      return ESapType::SapType3;
    case SapType_C::SapType4:
      return ESapType::SapType4;
    case SapType_C::SapType5:
      return ESapType::SapType5;
    case SapType_C::SapType6:
      return ESapType::SapType6;
    default:
      return ESapType::SapTypeInvalid;
  }
}

ESapType convertSapTypeFromUint8ToESapType(uint8_t sapType) {
  switch (sapType) {
    case 1:
      return ESapType::SapType1;
    case 2:
      return ESapType::SapType2;
    case 3:
      return ESapType::SapType3;
    case 4:
      return ESapType::SapType4;
    case 5:
      return ESapType::SapType5;
    case 6:
      return ESapType::SapType6;
    default:
      return ESapType::SapTypeInvalid;
  }
}

LogLevel convertLogLevelCEnum(LogLevel_C logLevel) {
  switch (logLevel) {
    case LogLevel_C::LogLevelStandard:
      return LogLevel::standard;
    case LogLevel_C::LogLevelVerbose:
      return LogLevel::verbose;
    default:
      throw std::invalid_argument("Invalid log level given.");
  }
}

void fillIsobmffReadInstance(ISOBMFF_Reader** isobmff) {
  (*isobmff)->movieInfo = (*isobmff)->isobmffReader->movieInfo();
  (*isobmff)->drcInfo = (*isobmff)->isobmffReader->specificBoxInfo<SDrcInfo>();
  (*isobmff)->iodsInfo = (*isobmff)->isobmffReader->specificBoxInfo<SIodsInfo>();
}

SMovieConfig isobmffHelper_createMovieConfig(const MovieConfig& config) {
  SMovieConfig movConfig;
  movConfig.majorBrand = config.majorBrand;
  movConfig.compatibleBrands = config.compatibleBrands;
  movConfig.currentTimeInUtc = config.currentTimeInUTC;
  movConfig.forceTfdtBoxV1 = config.force64bitMDT;
  movConfig.movieTimeScale = config.movieTimeScale;

  if (config.sidxConfig) {
    SSidxConfig sidxConfig;
    sidxConfig.sapType = config.sidxConfig->sapType;
    movConfig.sidxConfig = ilo::make_unique<SSidxConfig>(sidxConfig);
  }

  if (config.iodsConfig) {
    SIodsConfig iodsConfig;
    iodsConfig.audioProfileLevelIndication = config.iodsConfig->audioProfileLevelIndication;
    movConfig.iodsConfig = ilo::make_unique<SIodsConfig>(iodsConfig);
  }

  movConfig.userData = config.userData;

  return movConfig;
}

/* ------------Public logging methods------------------- */

ISOBMFF_ERR isobmff_redirectLog(const char* fileUri) {
  try {
    redirectLoggingToFile(fileUri, RedirectMode::overwrite);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("logging redirect failed: %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_redirectLogAppend(const char* fileUri) {
  try {
    redirectLoggingToFile(fileUri, RedirectMode::append);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("logging redirect failed %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_redirectLogToConsole() {
  try {
    redirectLoggingToConsole();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("logging redirect failed %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_redirectLogToSystemLogger() {
  try {
    redirectLoggingToSystemLogger();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("logging redirect failed %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_disableLogging() {
  try {
    disableLogging();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Disabling the logging system failed %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setLogLevel(const LogLevel_C logLevel) {
  try {
    setLogLevel(convertLogLevelCEnum(logLevel));
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Setting the log level failed: %s", e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

/* ------------Public reading methods------------------- */

ISOBMFF_ERR isobmff_createFileReader(ISOBMFF_Reader** isobmff, const char* fileUri) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff reader handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (fileUri == nullptr) {
    ILO_LOG_ERROR("File uri can't be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_create can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Reader();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_Reader instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create a file input from the given uri and use it to create the reader */
    (*isobmff)->isobmffReader = std::unique_ptr<CIsobmffReader>(
        new CIsobmffReader(std::unique_ptr<IIsobmffInput>(new CIsobmffFileInput(fileUri))));
    fillIsobmffReadInstance(isobmff);
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMemoryReader(ISOBMFF_Reader** isobmff, const uint8_t* dataBuffer,
                                       const uint64_t size) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff reader handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (dataBuffer == nullptr) {
    ILO_LOG_ERROR("Data buffer can't be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_create can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Reader();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_Reader instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create a memory input from the given data buffer and use it to create the reader */
    auto data = std::make_shared<ilo::ByteBuffer>(dataBuffer, dataBuffer + size);
    (*isobmff)->isobmffReader = std::unique_ptr<CIsobmffReader>(
        new CIsobmffReader(std::unique_ptr<IIsobmffInput>(new CIsobmffMemoryInput(data))));
    fillIsobmffReadInstance(isobmff);
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroy(ISOBMFF_Reader* isobmff) {
  if (isobmff != nullptr) {
    /* Build up extra vector since isobmff->trackVec will be
       altered when calling isobmff_destroyTrack(...) */
    std::vector<TrackReader*> destructTrackVec;

    try {
      for (size_t i = 0; i < isobmff->trackVec.size(); ++i) {
        destructTrackVec.push_back(isobmff->trackVec[i].get());
      }
    } catch (const std::exception& e) {
      ILO_LOG_ERROR(e.what());
      return ISOBMFF_LIB_ERR;
    }

    /* Delete and unregister tracks */
    for (const auto& track : destructTrackVec) {
      isobmff_destroyTrack(track);
    }
    delete isobmff;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackCount(ISOBMFF_Reader* isobmff, uint32_t* trackCount) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR("isobmff_getTrackCount has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (trackCount == nullptr) {
    ILO_LOG_ERROR("NumTracks is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    *trackCount = static_cast<uint32_t>(isobmff->isobmffReader->trackCount());
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTimeScale(ISOBMFF_Reader* isobmff, uint32_t* timeScale) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR("isobmff_getTimeScale has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (timeScale == nullptr) {
    ILO_LOG_ERROR("TimeScale is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *timeScale = static_cast<uint32_t>(isobmff->movieInfo.timeScale);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getMovieUserDataEntryCount(ISOBMFF_Reader* isobmff, uint32_t* count) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_getMovieUserDataEntryCount has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (count == nullptr) {
    ILO_LOG_ERROR("Count is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *count = static_cast<uint32_t>(isobmff->movieInfo.userData.size());
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getMovieUserDataEntryByIndex(ISOBMFF_Reader* isobmff, const uint32_t index,
                                                 uint8_t** data, uint32_t* size) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_getMovieUserDataEntryByIndex has been called without calling isobmff_create "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*data != nullptr) {
    ILO_LOG_ERROR("Data points already to allocated data");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    if (isobmff->movieInfo.userData.empty()) {
      *data = nullptr;
      *size = 0;
    } else {
      ILO_ASSERT(index < isobmff->movieInfo.userData.size(), "User data index is invalid");
      *data = isobmff->movieInfo.userData[index].data();
      *size = static_cast<uint32_t>(isobmff->movieInfo.userData[index].size());
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_PARAM_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getIodsAudioProfileLevelIndication(ISOBMFF_Reader* isobmff,
                                                       uint8_t* audioProfileLevelIndication,
                                                       uint8_t* isValid) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_getIodsAudioProfileLevelIndication has been called without calling isobmff_create "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (audioProfileLevelIndication == nullptr) {
    ILO_LOG_ERROR("AudioProfileLevelIndication is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isValid == nullptr) {
    ILO_LOG_ERROR("IsValid is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    std::unique_ptr<SIodsInfo> iodsInfo = isobmff->isobmffReader->specificBoxInfo<SIodsInfo>();
    ILO_ASSERT(iodsInfo != nullptr, "Failed to retrieve iods box information.");

    if (iodsInfo->iodsInfoAvailable()) {
      *audioProfileLevelIndication = iodsInfo->audioProfileLevelIndication();
      *isValid = 1;
    } else {
      *audioProfileLevelIndication = 0;
      *isValid = 0;
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_PARAM_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getMajorBrand(ISOBMFF_Reader* isobmff, char** majorBrand, uint32_t* brandSize) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR("isobmff_getMajorBrand has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (majorBrand == nullptr) {
    ILO_LOG_ERROR("majorBrand is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*majorBrand != nullptr) {
    ILO_LOG_ERROR("majorBrand points already to allocated memory");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (brandSize == nullptr) {
    ILO_LOG_ERROR("brandSize is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *majorBrand = isobmff->movieInfo.majorBrand.data();
  *brandSize = static_cast<uint32_t>(isobmff->movieInfo.majorBrand.size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getNrOfCompatibleBrands(ISOBMFF_Reader* isobmff,
                                            uint32_t* nrOfCompatibleBrands) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_getNrOfCompatibleBrands has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (nrOfCompatibleBrands == nullptr) {
    ILO_LOG_ERROR("nrOfCompatibleBrands is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *nrOfCompatibleBrands = static_cast<uint32_t>(isobmff->movieInfo.compatibleBrands.size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getCompatibleBrand(ISOBMFF_Reader* isobmff, const uint32_t brandIndex,
                                       char** compatibleBrand, uint32_t* brandSize) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_getCompatibleBrand has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (compatibleBrand == nullptr) {
    ILO_LOG_ERROR("majorBrand is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*compatibleBrand != nullptr) {
    ILO_LOG_ERROR("compatibleBrand points already to allocated memory");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (brandSize == nullptr) {
    ILO_LOG_ERROR("brandSize is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isobmff->movieInfo.compatibleBrands.size() <= brandIndex) {
    ILO_LOG_ERROR("brandIndex is out of bounds");
    *compatibleBrand = nullptr;
    *brandSize = 0;
    return ISOBMFF_PARAM_ERR;
  }

  *compatibleBrand = isobmff->movieInfo.compatibleBrands[brandIndex].data();
  *brandSize = static_cast<uint32_t>(isobmff->movieInfo.compatibleBrands[brandIndex].size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrack(ISOBMFF_Reader* isobmff, TrackReader** track,
                             const uint32_t trackIndex) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR("isobmff_getTrack has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  CTrackInfoVec trackInfoVect;

  try {
    trackInfoVect = isobmff->isobmffReader->trackInfos();

    if (trackIndex >= trackInfoVect.size()) {
      ILO_LOG_ERROR("Track index %u does not exist", trackIndex);
      return ISOBMFF_PARAM_ERR;
    }

    std::unique_ptr<TrackReader> pTrack = ilo::make_unique<TrackReader>();

    pTrack->trackIndex = trackIndex;
    pTrack->isobmff = isobmff;
    pTrack->trackInfo = trackInfoVect[trackIndex];

    // If the codec is MPEG-H get metadata that is only available from the MPEG-H specific track
    // reader
    if (pTrack->trackInfo.codec == Codec::mpegh_mha ||
        pTrack->trackInfo.codec == Codec::mpegh_mhm) {
      auto tmpTrack =
          pTrack->isobmff->isobmffReader->trackByIndex<CMpeghTrackReader>(pTrack->trackIndex);
      pTrack->mpeghProfileAndLevelCompatibleSets = tmpTrack->profileAndLevelCompatibleSets();
    }

    /* Create a new track reader of type CGenericAudioTrackReader or CGenericTrackReader */
    if (pTrack->trackInfo.type == TrackType::audio) {
      pTrack->trackReader.genericTrackReader =
          pTrack->isobmff->isobmffReader->trackByIndex<CGenericAudioTrackReader>(
              pTrack->trackIndex);
    } else {
      pTrack->trackReader.genericTrackReader =
          pTrack->isobmff->isobmffReader->trackByIndex<CGenericTrackReader>(pTrack->trackIndex);
    }
    isobmff->trackVec.push_back(std::move(pTrack));
    *track = isobmff->trackVec.back().get();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyTrack(TrackReader* track) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  track->trackReader.genericTrackReader = nullptr;

  /* Get isobmff handle, since track will be deleted */
  ISOBMFF_Reader* isobmff = track->isobmff;

  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_create(...) needs to be called, "
        "before using isobmff_destroyTrack(...) function");
    return ISOBMFF_NOT_INIT_ERR;
  }

  isobmff->trackVec.erase(std::remove_if(isobmff->trackVec.begin(), isobmff->trackVec.end(),
                                         [&track](const std::unique_ptr<TrackReader>& currTrack) {
                                           return (currTrack.get() == track);
                                         }),
                          isobmff->trackVec.end());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackId(TrackReader* track, uint32_t* trackId) {
  if (track == nullptr || trackId == nullptr) {
    ILO_LOG_ERROR("Track or trackId is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *trackId = track->trackInfo.trackId;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackType(TrackReader* track, TrackType_C* trackType) {
  if (track == nullptr || trackType == nullptr) {
    ILO_LOG_ERROR("Track or trackType is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *trackType = convertTrackTypeEnum(track->trackInfo.type);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackCodec(TrackReader* track, Codec_C* codec) {
  if (track == nullptr || codec == nullptr) {
    ILO_LOG_ERROR("Track or codec is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *codec = convertCodecEnum(track->trackInfo.codec);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackHandler(TrackReader* track, char** handler, uint32_t* size) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (handler == nullptr) {
    ILO_LOG_ERROR("Handler is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*handler != nullptr) {
    ILO_LOG_ERROR("Handler points already to allocated memory");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.handler.size() > 4) {
    ILO_LOG_ERROR("Handler is too long");
    return ISOBMFF_LIB_ERR;
  }

  *handler = track->trackInfo.handler.data();
  *size = static_cast<uint32_t>(track->trackInfo.handler.size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackCodingName(TrackReader* track, char** codingName, uint32_t* size) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (codingName == nullptr) {
    ILO_LOG_ERROR("CodingName is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*codingName != nullptr) {
    ILO_LOG_ERROR("CodingName points already to allocated memory");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.codingName.size() > 4) {
    ILO_LOG_ERROR("Coding name is too long");
    return ISOBMFF_LIB_ERR;
  }

  *codingName = track->trackInfo.codingName.data();
  *size = static_cast<uint32_t>(track->trackInfo.codingName.size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackDuration(TrackReader* track, uint64_t* trackDuration) {
  if (track == nullptr || trackDuration == nullptr) {
    ILO_LOG_ERROR("Track or trackDuration is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *trackDuration = track->trackInfo.duration;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackTimeScale(TrackReader* track, uint32_t* trackTimeScale) {
  if (track == nullptr || trackTimeScale == nullptr) {
    ILO_LOG_ERROR("Track or trackTimeScale is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *trackTimeScale = track->trackInfo.timescale;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getMaxSampleSize(TrackReader* track, uint64_t* maxSampleSize) {
  if (track == nullptr || maxSampleSize == nullptr) {
    ILO_LOG_ERROR("Track or maxSampleSize is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *maxSampleSize = track->trackInfo.maxSampleSize;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleCount(TrackReader* track, uint64_t* sampleCount) {
  if (track == nullptr || sampleCount == nullptr) {
    ILO_LOG_ERROR("Track or sampleCount is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *sampleCount = (uint64_t)track->trackInfo.sampleCount;
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getEditListEntryCount(TrackReader* track, uint32_t* editListEntryCount) {
  if (nullptr == track) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (nullptr == editListEntryCount) {
    ILO_LOG_ERROR("Edit list entry count is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *editListEntryCount = static_cast<uint32_t>(track->trackInfo.editList.size());

  return ISOBMFF_OK;
}

template <typename T>
static bool verifyEditListParameters(TrackReader* track, uint32_t editListEntryIndex, T* outValue) {
  if (nullptr == track) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return false;
  }

  if (nullptr == outValue) {
    ILO_LOG_ERROR("Output variable is a zero pointer");
    return false;
  }

  if (editListEntryIndex >= track->trackInfo.editList.size()) {
    ILO_LOG_ERROR("Edit list entry index is out of range");
    return false;
  }

  return true;
}

ISOBMFF_ERR isobmff_getEditListEntrySegmentDuration(TrackReader* track,
                                                    const uint32_t editListEntryIndex,
                                                    uint64_t* segmentDuration) {
  if (!verifyEditListParameters(track, editListEntryIndex, segmentDuration)) {
    return ISOBMFF_PARAM_ERR;
  }

  *segmentDuration = track->trackInfo.editList[editListEntryIndex].segmentDuration;
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getEditListEntryMediaTime(TrackReader* track, const uint32_t editListEntryIndex,
                                              int64_t* mediaTime) {
  if (!verifyEditListParameters(track, editListEntryIndex, mediaTime)) {
    return ISOBMFF_PARAM_ERR;
  }
  *mediaTime = track->trackInfo.editList[editListEntryIndex].mediaTime;
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getEditListEntryMediaRate(TrackReader* track, const uint32_t editListEntryIndex,
                                              float* mediaRate) {
  if (!verifyEditListParameters(track, editListEntryIndex, mediaRate)) {
    return ISOBMFF_PARAM_ERR;
  }

  *mediaRate = track->trackInfo.editList[editListEntryIndex].mediaRate;
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackUserDataEntryCount(TrackReader* trackReader, uint32_t* count) {
  if (trackReader == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (count == nullptr) {
    ILO_LOG_ERROR("Output variable is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *count = static_cast<uint32_t>(trackReader->trackInfo.userData.size());
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackUserDataEntryByIndex(TrackReader* trackReader, const uint32_t index,
                                                 uint8_t** data, uint32_t* size) {
  if (trackReader == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*data != nullptr) {
    ILO_LOG_ERROR("Data points already to allocated data");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    trackReader->userData = trackReader->trackInfo.userData;
    if (trackReader->userData.empty()) {
      *data = nullptr;
      *size = 0;
    } else {
      ILO_ASSERT(index < trackReader->trackInfo.userData.size(), "User data index is invalid");
      *data = trackReader->userData[index].data();
      *size = static_cast<uint32_t>(trackReader->userData[index].size());
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_PARAM_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getInitLudtData(TrackReader* track, uint8_t** data, uint32_t* size) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*data != nullptr) {
    ILO_LOG_ERROR("Data points already to allocated data");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    ILO_ASSERT(track->isobmff->drcInfo != nullptr, "Drc Info object is not existing");
    track->loudnessInfo.ludtInitInfo = track->isobmff->drcInfo->globalLudtData(track->trackIndex);
    if (track->loudnessInfo.ludtInitInfo.empty()) {
      *data = nullptr;
      *size = 0;
    } else {
      *data = track->loudnessInfo.ludtInitInfo.data();
      *size = static_cast<uint32_t>(track->loudnessInfo.ludtInitInfo.size());
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_hasLudtUpdates(TrackReader* track, uint8_t* hasLudtUpdates) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (hasLudtUpdates == nullptr) {
    ILO_LOG_ERROR("hasLudtUpdates is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    ILO_ASSERT(track->isobmff->drcInfo != nullptr, "Drc Info object is not existing");
    auto hasUpdates = track->isobmff->drcInfo->trackHasLudtUpdates(track->trackIndex);
    if (hasUpdates) {
      *hasLudtUpdates = 1;
    } else {
      *hasLudtUpdates = 0;
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getFragmentLudtData(TrackReader* track, uint32_t fragmentNum, uint8_t** data,
                                        uint32_t* size) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*data != nullptr) {
    ILO_LOG_ERROR("Data points already to allocated data");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    ILO_ASSERT(track->isobmff->drcInfo != nullptr, "Drc Info object is not existing");
    track->loudnessInfo.ludtFragInfo =
        track->isobmff->drcInfo->fragmentLudtData(track->trackIndex, fragmentNum);
    if (track->loudnessInfo.ludtFragInfo.empty()) {
      *data = nullptr;
      *size = 0;
    } else {
      *data = track->loudnessInfo.ludtFragInfo.data();
      *size = static_cast<uint32_t>(track->loudnessInfo.ludtFragInfo.size());
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getDecoderSpecificConfig(TrackReader* track, uint8_t** data, uint32_t* size) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*data != nullptr) {
    ILO_LOG_ERROR("Data points already to allocated data");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackReader.genericTrackReader != nullptr) {
    try {
      switch (track->trackInfo.codec) {
        case Codec::mp4v:
        case Codec::avc:
        case Codec::hevc:
        case Codec::jxs:
        case Codec::vvc:
          ILO_LOG_ERROR("Getting decoder specific config for codec %s is not implemented",
                        ilo::toString(track->trackInfo.codingName).c_str());
          return ISOBMFF_NOT_IMPL_ERR;
        case Codec::mpegh_mha:
        case Codec::mpegh_mhm: {
          ilo::ByteBuffer dcrBlob = track->trackReader.genericTrackReader->decoderConfigRecord();
          ilo::ByteBuffer::const_iterator dcrBlobBegin = dcrBlob.begin();

          if (dcrBlob.size() == 0) {
            *data = nullptr;
            *size = 0;
          } else {
            config::CMhaDecoderConfigRecord mhaDcr(dcrBlobBegin, dcrBlob.end());
            track->dsc = mhaDcr.mpegh3daConfig();
            *data = track->dsc.data();
            *size = static_cast<uint32_t>(track->dsc.size());
          }
          break;
        }
        case Codec::mp4a: {
          ilo::ByteBuffer dcrBlob = track->trackReader.genericTrackReader->decoderConfigRecord();
          ilo::ByteBuffer::const_iterator dcrBlobBegin = dcrBlob.begin();

          if (dcrBlob.size() == 0) {
            *data = nullptr;
            *size = 0;
          } else {
            config::CMp4aDecoderConfigRecord mp4aDcr(dcrBlobBegin, dcrBlob.end());
            track->dsc = mp4aDcr.asc();
            *data = track->dsc.data();
            *size = static_cast<uint32_t>(track->dsc.size());
          }
          break;
        }
        case Codec::undefined:
        default:
          ILO_LOG_ERROR("Unknown codec found");
          return ISOBMFF_PARAM_ERR;
      }
    } catch (const std::exception& e) {
      ILO_LOG_ERROR(e.what());
      return ISOBMFF_LIB_ERR;
    }
  } else {
    ILO_LOG_ERROR("Cannot get decoder config record, because no specific track reader was created");
    return ISOBMFF_NOT_INIT_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getTrackLanguage(TrackReader* track, char** trackLanguage,
                                     uint32_t* trackLanguageSize) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (trackLanguage == nullptr) {
    ILO_LOG_ERROR("TrackLanguage is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (*trackLanguage != nullptr) {
    ILO_LOG_ERROR("TrackLanguage points already to allocated memory");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (trackLanguageSize == nullptr) {
    ILO_LOG_ERROR("TrackLanguageSize is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.language.size() == 0) {
    *trackLanguage = nullptr;
    *trackLanguageSize = 0;
    return ISOBMFF_OK;
  }

  *trackLanguage = track->trackInfo.language.data();
  *trackLanguageSize = static_cast<uint32_t>(track->trackInfo.language.size());

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getAudioSampleRate(TrackReader* track, uint32_t* audioSamplerate) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (audioSamplerate == nullptr) {
    ILO_LOG_ERROR("AudioSamplerate is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.type != TrackType::audio) {
    *audioSamplerate = 0;
    return ISOBMFF_OK;
  }

  try {
    auto audiotrackReader =
        dynamic_cast<CGenericAudioTrackReader*>(track->trackReader.genericTrackReader.get());
    if (audiotrackReader == nullptr) {
      *audioSamplerate = 0;
      ILO_LOG_ERROR(
          "isobmff_getAudioSampleRate failed, because trackReader could not access audio specific "
          "data fields.");
      return ISOBMFF_LIB_ERR;
    }

    *audioSamplerate = audiotrackReader->sampleRate();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getAudioChannelCount(TrackReader* track, uint32_t* audioChannelCount) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (audioChannelCount == nullptr) {
    ILO_LOG_ERROR("AudioChannelCount is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.type != TrackType::audio) {
    *audioChannelCount = 0;
    return ISOBMFF_OK;
  }

  try {
    auto audiotrackReader =
        dynamic_cast<CGenericAudioTrackReader*>(track->trackReader.genericTrackReader.get());
    if (audiotrackReader == nullptr) {
      *audioChannelCount = 0;
      ILO_LOG_ERROR(
          "isobmff_getAudioChannelCount failed, because trackReader could not access audio "
          "specific data fields.");
      return ISOBMFF_LIB_ERR;
    }

    *audioChannelCount = audiotrackReader->channelCount();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getDcrMp4aAudioMaxBitrate(TrackReader* track, uint32_t* mp4aAudioMaxBitrate) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (mp4aAudioMaxBitrate == nullptr) {
    ILO_LOG_ERROR("Mp4aAudioMaxBitrate is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.codec != Codec::mp4a) {
    *mp4aAudioMaxBitrate = 0;
    return ISOBMFF_OK;
  }

  try {
    ilo::ByteBuffer dcrBlob = track->trackReader.genericTrackReader->decoderConfigRecord();
    ilo::ByteBuffer::const_iterator dcrBlobBegin = dcrBlob.begin();

    config::CMp4aDecoderConfigRecord mp4aDcr(dcrBlobBegin, dcrBlob.end());
    *mp4aAudioMaxBitrate = mp4aDcr.maxBitrate();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getDcrMp4aAudioAvgBitrate(TrackReader* track, uint32_t* mp4aAudioAvgBitrate) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (mp4aAudioAvgBitrate == nullptr) {
    ILO_LOG_ERROR("Mp4aAudioAvgBitrate is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.codec != Codec::mp4a) {
    *mp4aAudioAvgBitrate = 0;
    return ISOBMFF_OK;
  }

  try {
    ilo::ByteBuffer dcrBlob = track->trackReader.genericTrackReader->decoderConfigRecord();
    ilo::ByteBuffer::const_iterator dcrBlobBegin = dcrBlob.begin();

    config::CMp4aDecoderConfigRecord mp4aDcr(dcrBlobBegin, dcrBlob.end());
    *mp4aAudioAvgBitrate = mp4aDcr.avgBitrate();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getDcrMp4aAudioBufferSizeDb(TrackReader* track,
                                                uint32_t* mp4aAudioBufferSizeDb) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (mp4aAudioBufferSizeDb == nullptr) {
    ILO_LOG_ERROR("Mp4aAudioBufferSizeDb is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (track->trackInfo.codec != Codec::mp4a) {
    *mp4aAudioBufferSizeDb = 0;
    return ISOBMFF_OK;
  }

  try {
    ilo::ByteBuffer dcrBlob = track->trackReader.genericTrackReader->decoderConfigRecord();
    ilo::ByteBuffer::const_iterator dcrBlobBegin = dcrBlob.begin();

    config::CMp4aDecoderConfigRecord mp4aDcr(dcrBlobBegin, dcrBlob.end());
    *mp4aAudioBufferSizeDb = mp4aDcr.bufferSizeDB();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getNextSample(TrackReader* track, Sample* sample) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track or codec a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (sample == nullptr) {
    ILO_LOG_ERROR("isobmff_createSample must be called first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  ISOBMFF_ERR err = isobmff_resetSample(sample);
  if (err != ISOBMFF_OK) {
    return err;
  }

  try {
    if (sample->rawData.capacity() < track->trackInfo.maxSampleSize) {
      ILO_LOG_WARNING(
          "Pre-Allocated sample size is smaller than "
          "maxSampleSize! Re-allocation might happen");
    }

    track->trackReader.genericTrackReader->nextSample(*sample);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleByIndex(TrackReader* track, Sample* sample, const size_t index) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track or codec a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (sample == nullptr) {
    ILO_LOG_ERROR("isobmff_createSample must be called first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  ISOBMFF_ERR err = isobmff_resetSample(sample);
  if (err != ISOBMFF_OK) {
    return err;
  }

  try {
    if (sample->rawData.capacity() < track->trackInfo.maxSampleSize) {
      ILO_LOG_WARNING(
          "Pre-Allocated sample size is smaller than "
          "maxSampleSize! Re-allocation might happen");
    }

    track->trackReader.genericTrackReader->sampleByIndex(index, *sample);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

/* ------------Public writing methods------------------- */

ISOBMFF_ERR isobmff_createIodsConfig(IodsConfig** config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid iods config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the iods config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createIodsConfig can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *config = new (std::nothrow) IodsConfig();

  if (*config == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new IodsConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setAudioProfileLevelIndication(IodsConfig* config,
                                                   const uint8_t audioProfileLevelIndication) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setAudioProfileLevelIndication has been called without calling "
        "isobmff_createIodsConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->audioProfileLevelIndication = audioProfileLevelIndication;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyIodsConfig(IodsConfig* config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("IodsConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete config;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createSidxsConfig(SidxConfig** config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid sidx config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the sidx config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createSidxConfig can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *config = new (std::nothrow) SidxConfig();

  if (*config == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new SidxConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setStreamAccessPointType(SidxConfig* config, const SapType_C sapType) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setStreamAccessPointType has been called without calling "
        "isobmff_createSidxsConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->sapType = convertSapTypeCEnum(sapType);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroySidxConfig(SidxConfig* config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("SidxConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete config;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMovieConfig(MovieConfig** config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid movie config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the movie config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createMovieConfig can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *config = new (std::nothrow) MovieConfig();

  if (*config == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new MovieConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setMajorBrand(MovieConfig* config, const char* majorBrand,
                                  const uint32_t size) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setMajorBrand has been called without calling isobmff_createMovieConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (size != 4) {
    ILO_LOG_ERROR("majorBrand should be 4 characters long");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    config->majorBrand = ilo::toFcc(std::string(majorBrand, size));
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addCompatibleBrand(MovieConfig* config, const char* compatibleBrand,
                                       const uint32_t size) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addCompatibleBrand has been called without calling isobmff_createMovieConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (size != 4) {
    ILO_LOG_ERROR("compatibleBrand should be 4 characters long");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    config->compatibleBrands.push_back(ilo::toFcc(std::string(compatibleBrand, size)));
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setTimeScale(MovieConfig* config, const uint32_t timeScale) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setTimeScale has been called without calling isobmff_createMovieConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->movieTimeScale = timeScale;
  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setcurrentTimeInUtc(MovieConfig* config, const uint64_t currentTimeInUTC) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setcurrentTimeInUtc has been called without calling isobmff_createMovieConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->currentTimeInUTC = currentTimeInUTC;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_force64bitMediaDecodeTime(MovieConfig* config, const uint8_t use64bitMDT) {
  if (config == nullptr) {
    ILO_LOG_ERROR("isobmff_createMovieConfig must be called first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (use64bitMDT > 1) {
    ILO_LOG_ERROR("the 64-bit MDT flag must have a value of 0 or 1. Assuming a value of 1");
  }

  config->force64bitMDT = (use64bitMDT >= 1);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setIodsConfig(MovieConfig* movieConfig, const IodsConfig* iodsConfig) {
  if (movieConfig == nullptr) {
    ILO_LOG_ERROR("isobmff_createMovieConfig must be called first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (iodsConfig == nullptr) {
    ILO_LOG_ERROR("isobmff_createIodsConfig must be called first");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    movieConfig->iodsConfig = ilo::make_unique<IodsConfig>(*iodsConfig);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSidxConfig(MovieConfig* movieConfig, const SidxConfig* sidxConfig) {
  if (movieConfig == nullptr) {
    ILO_LOG_ERROR("isobmff_createMovieConfig must be called first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (sidxConfig == nullptr) {
    ILO_LOG_ERROR("isobmff_createSidxConfig must be called first");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    movieConfig->sidxConfig = ilo::make_unique<SidxConfig>(*sidxConfig);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addMovieUserDataEntry(MovieConfig* movieConfig, const uint8_t* data,
                                          const uint32_t size) {
  if (movieConfig == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addMovieUserDataEntry has been called without calling isobmff_createMovieConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("user data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size < 8) {
    ILO_LOG_ERROR(
        "user given buffer needs to have at least a size of 8 bytes (size and type fields)");
    return ISOBMFF_PARAM_ERR;
  }

  uint32_t tmpSize = static_cast<uint32_t>(data[3] | data[2] << 8 | data[1] << 16 | data[0] << 24);

  if (tmpSize != size) {
    ILO_LOG_ERROR(
        "sizes mismatch between what is given to the user and what has been found in the buffer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    ilo::ByteBuffer buffer(data, data + size);
    movieConfig->userData.push_back(buffer);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyMovieConfig(MovieConfig* config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("MovieConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete config;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createFragFileWriter(ISOBMFF_Writer** isobmff, const MovieConfig* config,
                                         const char* outFileUri) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff writer handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (outFileUri == nullptr) {
    ILO_LOG_ERROR("File Uri can't be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_createFragFileWriter can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Writer();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_FragFileW instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create the fragmented file writer */
    CIsobmffFragFileWriter::SOutputConfig outConfig;
    outConfig.outputUri = outFileUri;
    auto movConfig = isobmffHelper_createMovieConfig(*config);

    (*isobmff)->isobmffWriter = ilo::make_unique<CIsobmffFragFileWriter>(outConfig, movConfig);
    (*isobmff)->usedMovieTimescale = config->movieTimeScale;
    (*isobmff)->isFragmentedWriter = true;
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_newMediaSegment(ISOBMFF_Writer* isobmff) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_newMediaSegment has been called without calling isobmff_createFragFileWriter "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (isobmff->isobmffWriter == nullptr) {
    ILO_LOG_ERROR("The isobmff writer handle has not been initiated correctly.");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    isobmff->isobmffWriter->createMediaFragments();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_newInitFileSegment(ISOBMFF_Writer* isobmff, const char* outFileUri) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_newInitFileSegment has been called without calling "
        "isobmff_createFragFileSegWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (outFileUri == nullptr) {
    ILO_LOG_ERROR("outFileUri is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isobmff->isobmffWriter == nullptr) {
    ILO_LOG_ERROR("The isobmff writer handle has not been initiated correctly.");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    isobmff->isobmffWriter->createInitFileSegment(outFileUri);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createFragFileSegWriter(ISOBMFF_Writer** isobmff, const MovieConfig* config) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff writer handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("MovieConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_createFragFileSegWriter can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Writer();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_FragFileSegW instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create the fragmented file writer */
    auto movConfig = isobmffHelper_createMovieConfig(*config);

    (*isobmff)->isobmffWriter = ilo::make_unique<CIsobmffFragFileSegWriter>(movConfig);
    (*isobmff)->usedMovieTimescale = config->movieTimeScale;
    (*isobmff)->isFragmentedWriter = true;
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_newMediaFileSegment(ISOBMFF_Writer* isobmff, const char* outFileUri,
                                        const uint8_t isLastSegment) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_newMediaFileSegment has been called without calling "
        "isobmff_createFragFileSegWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (outFileUri == nullptr) {
    ILO_LOG_ERROR("outFileUri is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isobmff->isobmffWriter == nullptr) {
    ILO_LOG_ERROR("The isobmff writer handle has not been initiated correctly.");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (isLastSegment > 1) {
    ILO_LOG_ERROR("The isLastSegment flag must have a value of 0 or 1. Assuming a value of 1.");
  }

  bool isLastSeg = (isLastSegment >= 1);

  try {
    isobmff->isobmffWriter->createMediaFileSegment(outFileUri, isLastSeg);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createFragMemoryWriter(ISOBMFF_Writer** isobmff, const MovieConfig* config) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff writer handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("MovieConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_createFragMemoryWriter can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Writer();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_FragMemW instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create the fragmented memory writer */
    auto movConfig = isobmffHelper_createMovieConfig(*config);

    (*isobmff)->isobmffWriter = ilo::make_unique<CIsobmffFragMemoryWriter>(movConfig);
    (*isobmff)->usedMovieTimescale = config->movieTimeScale;
    (*isobmff)->isFragmentedWriter = true;
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_newInitMemorySegment(ISOBMFF_Writer* isobmff, uint8_t** data, uint64_t* size) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_newInitMemorySegment has been called without calling "
        "isobmff_createFragMemoryWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    isobmff->buffer = isobmff->isobmffWriter->createInitSegment();
    *data = isobmff->buffer->data();
    *size = static_cast<uint64_t>(isobmff->buffer->size());
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_newMediaMemorySegment(ISOBMFF_Writer* isobmff, uint8_t** data, uint64_t* size,
                                          const uint8_t isLastSegment) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_newMediaMemorySegment has been called without calling "
        "isobmff_createFragMemoryWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isLastSegment > 1) {
    ILO_LOG_ERROR("The isLastSegment flag must have a value of 0 or 1. Assuming a value of 1.");
  }

  bool isLastSeg = (isLastSegment >= 1);

  try {
    isobmff->buffer = isobmff->isobmffWriter->createMediaMemSegment(true, isLastSeg);

    *data = isobmff->buffer->data();
    *size = static_cast<uint64_t>(isobmff->buffer->size());
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createFileWriter(ISOBMFF_Writer** isobmff, MovieConfig* config,
                                     const char* outFileUri, const char* tmpFileUri) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff writer handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (outFileUri == nullptr) {
    ILO_LOG_ERROR("File Uri can't be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_createFileWriter can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Writer();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_FileWriter instance");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    /* Create the non-fragmented file writer */
    CIsobmffFileWriter::SOutputConfig outConfig;
    outConfig.outputUri = outFileUri;
    if (tmpFileUri != nullptr) {
      outConfig.tmpUri = tmpFileUri;
    }
    auto movConfig = isobmffHelper_createMovieConfig(*config);

    (*isobmff)->isobmffWriter = ilo::make_unique<CIsobmffFileWriter>(outConfig, movConfig);
    (*isobmff)->usedMovieTimescale = config->movieTimeScale;
    (*isobmff)->isFragmentedWriter = false;
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMemoryWriter(ISOBMFF_Writer** isobmff, MovieConfig* config) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff writer handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if we are already initialized */
  if (*isobmff != nullptr) {
    ILO_LOG_ERROR("isobmff_createFileWriter can't be called twice on the same handle");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *isobmff = new (std::nothrow) ISOBMFF_Writer();

  if (*isobmff == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for ISOBMFF_MemoryWriter instance");
    return ISOBMFF_MEMORY_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("Config can't be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    /* Create the non-fragmented memory writer */
    auto movConfig = isobmffHelper_createMovieConfig(*config);

    (*isobmff)->isobmffWriter = ilo::make_unique<CIsobmffMemoryWriter>(movConfig);
    (*isobmff)->usedMovieTimescale = config->movieTimeScale;
    (*isobmff)->isFragmentedWriter = false;
  } catch (const std::exception& e) {
    if (*isobmff != nullptr) {
      delete *isobmff;
      *isobmff = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_serializeToMemoryBuffer(ISOBMFF_Writer* isobmff, uint8_t** data,
                                            uint64_t* size) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_serializeToMemoryBuffer has been called without calling "
        "isobmff_createMemoryWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    isobmff->buffer = isobmff->isobmffWriter->serialize();

    if (isobmff->buffer) {
      *data = isobmff->buffer->data();
      *size = static_cast<uint64_t>(isobmff->buffer->size());
    } else {
      *data = nullptr;
      *size = 0;
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  if (*size == 0) {
    ILO_LOG_ERROR(
        "Serialized mp4 buffer is empty. Please make sure to call isobmff_serializeToMemoryBuffer "
        "only once.");
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyWriter(ISOBMFF_Writer* isobmff) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("ISOBMFF_Writer is not initialized");
    return ISOBMFF_NOT_INIT_ERR;
  }

  try {
    isobmff->isobmffWriter->close();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    delete isobmff;
    return ISOBMFF_LIB_ERR;
  }

  delete isobmff;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMpeghDecoderConfigRecord(MpeghDecoderConfigRecord** mpeghDcr) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR("Invalid mpegh decoder config record handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the decoder config record already points to data */
  if (*mpeghDcr != nullptr) {
    ILO_LOG_ERROR("isobmff_createMpeghDecoderConfigRecord can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *mpeghDcr = new (std::nothrow) MpeghDecoderConfigRecord();

  if (*mpeghDcr == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new MpeghDecoderConfigRecord");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setDcrConfigurationVersion(MpeghDecoderConfigRecord* mpeghDcr,
                                               const uint8_t configurationVersion) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrConfigurationVersion has been called without calling "
        "isobmff_createMpeghDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mpeghDcr->m_configurationVersion = configurationVersion;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setDcrProfileLevelIndication(MpeghDecoderConfigRecord* mpeghDcr,
                                                 const uint8_t profileLevelIndication) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrProfileLevelIndication has been called without calling "
        "isobmff_createMpeghDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mpeghDcr->m_mpegh3daProfileLevelIndication = profileLevelIndication;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setDcrReferenceChnlLayout(MpeghDecoderConfigRecord* mpeghDcr,
                                              const uint8_t referenceChannelLayout) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrReferenceChnlLayout has been called without calling "
        "isobmff_createMpeghDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mpeghDcr->m_referenceChannelLayout = referenceChannelLayout;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setDcrMpegh3daConfig(MpeghDecoderConfigRecord* mpeghDcr, const uint8_t* data,
                                         const uint64_t size) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrMpegh3daConfig has been called without calling "
        "isobmff_createMpeghDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("MPEG-H 3da config data cannot be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    mpeghDcr->m_mpegh3daConfig = ilo::ByteBuffer(data, data + size);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyMpeghDecoderConfigRecord(MpeghDecoderConfigRecord* mpeghDcr) {
  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR("MpeghDecoderConfigRecord is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete mpeghDcr;

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR
isobmff_createMp4aDecoderConfigRecord(Mp4aDecoderConfigRecord** mp4aDcr) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR("Invalid mp4a decoder config record handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the decoder config record already points to data */
  if (*mp4aDcr != nullptr) {
    ILO_LOG_ERROR("isobmff_createMp4aDecoderConfigRecord can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *mp4aDcr = new (std::nothrow) Mp4aDecoderConfigRecord();

  if (*mp4aDcr == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new MpeghDecoderConfigRecord");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrMaxBitrate(Mp4aDecoderConfigRecord* mp4aDcr,
                                                    const uint32_t maxBitrate) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrMaxBitrate has been called without calling "
        "isobmff_createMp4aDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mp4aDcr->m_maxBitrate = maxBitrate;

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrAvgBitrate(Mp4aDecoderConfigRecord* mp4aDcr,
                                                    const uint32_t avgBitrate) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrAvgBitrate has been called without calling "
        "isobmff_createMp4aDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mp4aDcr->m_avgBitrate = avgBitrate;

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrBufferSizeDB(Mp4aDecoderConfigRecord* mp4aDcr,
                                                      const uint32_t bufferSizeDB) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrBufferSizeDB has been called without calling "
        "isobmff_createMp4aDecoderConfigRecord first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  mp4aDcr->m_bufferSizeDB = bufferSizeDB;

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR isobmff_setDcrAsc(Mp4aDecoderConfigRecord* mp4aDcr, const uint8_t* data,
                                             const uint64_t size) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDcrAsc has been called without calling isobmff_createMp4aDecoderConfigRecord "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("ASC data cannot be a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    mp4aDcr->m_asc = ilo::ByteBuffer(data, data + size);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

MMTISOBMFF_DLL ISOBMFF_ERR
isobmff_destroyMp4aDecoderConfigRecord(Mp4aDecoderConfigRecord* mp4aDcr) {
  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR("Mp4aDecoderConfigRecord is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete mp4aDcr;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createEditListEntry(EditListEntry** editListEntry) {
  if (editListEntry == nullptr) {
    ILO_LOG_ERROR("Invalid edit list entry handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the edit list entry already points to data */
  if (*editListEntry != nullptr) {
    ILO_LOG_ERROR(
        "isobmff_createEditListEntry can't be called twice on the same edit list instance");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *editListEntry = new (std::nothrow) EditListEntry();

  if (*editListEntry == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new EditListEntry");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setEditListEntrySegmentDuration(EditListEntry* editListEntry,
                                                    const uint64_t segmentDuration) {
  if (editListEntry == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setEditListEntrySegmentDuration has been called without calling "
        "isobmff_createEditListEntry first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  editListEntry->segmentDuration = segmentDuration;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setEditListEntryMediaTime(EditListEntry* editListEntry,
                                              const int64_t mediaTime) {
  if (editListEntry == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setEditListEntryMediaTime has been called without calling "
        "isobmff_createEditListEntry first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  editListEntry->mediaTime = mediaTime;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setEditListEntryMediaRate(EditListEntry* editListEntry, const float mediaRate) {
  if (editListEntry == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setEditListEntryMediaRate has been called without calling "
        "isobmff_createEditListEntry first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  editListEntry->mediaRate = mediaRate;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyEditListEntry(EditListEntry* editListEntry) {
  if (editListEntry == nullptr) {
    ILO_LOG_ERROR("EditListEntry is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete editListEntry;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMpeghMultiStreamConfig(MpeghMultiStreamConfig** mpeghMsc) {
  if (mpeghMsc == nullptr) {
    ILO_LOG_ERROR("Invalid MPEG-H multistream config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the edit list entry already points to data */
  if (*mpeghMsc != nullptr) {
    ILO_LOG_ERROR(
        "isobmff_createMpeghMultiStreamConfig can't be called twice on the same multistream config "
        "instance");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *mpeghMsc = new (std::nothrow) MpeghMultiStreamConfig();

  if (*mpeghMsc == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new MpeghMultiStreamConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyMpeghMultiStreamConfig(MpeghMultiStreamConfig* mpeghMsc) {
  if (mpeghMsc == nullptr) {
    ILO_LOG_ERROR("MpeghMultiStreamConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete mpeghMsc;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createTrackConfig(TrackConfig** config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid track config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the track config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createTrackConfig can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *config = new (std::nothrow) TrackConfig();

  if (*config == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new TrackConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setTrackCodec(TrackConfig* config, const Codec_C codec) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setTrackCodec has been called without calling isobmff_createTrackConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->codec = convertCodecCEnum(codec);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setTrackTimeScale(TrackConfig* config, const uint32_t trackTimeScale) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setTrackTimeScale has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->trackTimeScale = trackTimeScale;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setTrackId(TrackConfig* config, const uint32_t trackId) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setTrackId has been called without calling isobmff_createTrackConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->trackId = trackId;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setAudioSampleRate(TrackConfig* config, const uint32_t sampleRate) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setAudioSampleRate has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->sampleRate = sampleRate;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setAudioChannelCount(TrackConfig* config, const uint16_t channelCount) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setAudioChannelCount has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->channelCount = channelCount;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setAudioTrackLanguage(TrackConfig* config, const char* language,
                                          const uint32_t size) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setAudioTrackLanguage has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (size != 3) {
    ILO_LOG_ERROR("language should be 3 characters long");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    config->language = ilo::toIsoLang(std::string(language, size));
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setMpeghDecoderConfigRecord(TrackConfig* config,
                                                const MpeghDecoderConfigRecord* mpeghDcr) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setMpeghDecoderConfigRecord has been called without calling "
        "isobmff_createTrackConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (mpeghDcr == nullptr) {
    ILO_LOG_ERROR("mpeghDcr is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    auto configRecord = ilo::make_unique<config::CMhaDecoderConfigRecord>();
    configRecord->setConfigurationVersion(mpeghDcr->m_configurationVersion);
    configRecord->setMpegh3daProfileLevelIndication(mpeghDcr->m_mpegh3daProfileLevelIndication);
    configRecord->setReferenceChannelLayout(mpeghDcr->m_referenceChannelLayout);
    configRecord->setMpegh3daConfig(mpeghDcr->m_mpegh3daConfig);

    ilo::ByteBuffer buffer(static_cast<size_t>(configRecord->size()));

    ByteBuffer::iterator begin = buffer.begin();
    configRecord->write(buffer, begin);
    config->dsc = ilo::make_unique<ilo::ByteBuffer>(buffer.begin(), buffer.end());
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setMp4aDecoderConfigRecord(TrackConfig* config,
                                               const Mp4aDecoderConfigRecord* mp4aDcr) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setMp4aDecoderConfigRecord has been called without calling "
        "isobmff_createTrackConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (mp4aDcr == nullptr) {
    ILO_LOG_ERROR("mp4aDcr is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    config::CMp4aDecoderConfigRecord::SConfig configRecordConfig;
    configRecordConfig.asc = mp4aDcr->m_asc;
    configRecordConfig.maxBitrate = mp4aDcr->m_maxBitrate;
    configRecordConfig.avgBitrate = mp4aDcr->m_avgBitrate;
    configRecordConfig.bufferSizeDB = mp4aDcr->m_bufferSizeDB;

    auto configRecord = ilo::make_unique<config::CMp4aDecoderConfigRecord>(configRecordConfig);

    ilo::ByteBuffer buffer(static_cast<size_t>(configRecord->size()));

    ByteBuffer::iterator begin = buffer.begin();
    configRecord->write(buffer, begin);
    config->dsc = ilo::make_unique<ilo::ByteBuffer>(buffer.begin(), buffer.end());
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setDefaultSampleGroup(TrackConfig* config, const SampleGroup_C sampleGroupType,
                                          const int16_t rollDistance) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setDefaultSampleGroup has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  config->defaultSampleGroup.rollDistance = rollDistance;
  config->defaultSampleGroup.type = convertSampleGroupCEnum(sampleGroupType);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setMpeghMultiStreamConfig(TrackConfig* config,
                                              const MpeghMultiStreamConfig* mpeghMsc) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setMpeghMultiStreamConfig has been called without calling "
        "isobmff_createTrackConfig first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (mpeghMsc == nullptr) {
    ILO_LOG_ERROR("mpeghMsc is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  // Hint: If we support optional multistream boxes later, we need to configure/fill
  //       the appropriate structures and replace/delete the boolean.
  config->enableMpeghMultiStream = true;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addMpeghPLcompatibleSet(TrackConfig* config, const uint8_t PLcompatibleSet) {
  if (config == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addMpeghPLcompatibleSet has been called without calling isobmff_createTrackConfig "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  try {
    config->mpegHPLcompatibleSets.push_back(PLcompatibleSet);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyTrackConfig(TrackConfig* config) {
  if (config == nullptr) {
    ILO_LOG_ERROR("TrackConfig is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete config;

  return ISOBMFF_OK;
}

void internal_fillBasicConfig(STrackConfig& tConfig, const TrackConfig* utConfig) {
  tConfig.mediaTimescale = utConfig->trackTimeScale;
  tConfig.trackID = utConfig->trackId;
  tConfig.defaultSampleGroup = utConfig->defaultSampleGroup;
}

void internal_fillAudioConfig(SBaseAudioConfig& aConfig, const TrackConfig* uaConfig) {
  aConfig.language = uaConfig->language;
  aConfig.sampleRate = uaConfig->sampleRate;
}

void internal_fillMpeghConfig(SMpeghTrackConfig& mpeghConfig, const TrackConfig* umpeghConfig) {
  internal_fillBasicConfig(mpeghConfig, umpeghConfig);
  internal_fillAudioConfig(mpeghConfig, umpeghConfig);

  if (umpeghConfig->channelCount != 0) {
    ILO_LOG_WARNING("Ignoring invalid channel count of value %d. It can only be 0 for MPEG-H. ",
                    umpeghConfig->channelCount);
  }

  if (umpeghConfig->dsc && umpeghConfig->dsc->size() != 0) {
    ilo::ByteBuffer::const_iterator dataBegin = umpeghConfig->dsc->cbegin();
    mpeghConfig.configRecord =
        ilo::make_unique<config::CMhaDecoderConfigRecord>(dataBegin, umpeghConfig->dsc->end());
  }

  mpeghConfig.profileAndLevelCompatibleSets = umpeghConfig->mpegHPLcompatibleSets;
}

ISOBMFF_ERR isobmff_createTrackWriter(ISOBMFF_Writer* isobmff, TrackWriter** track,
                                      const TrackConfig* config) {
  if (isobmff == nullptr) {
    ILO_LOG_ERROR("isobmff is a zero pointer");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (track == nullptr) {
    ILO_LOG_ERROR("track is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  /* Check if the track already points to data */
  if (*track != nullptr) {
    ILO_LOG_ERROR("isobmff_createTrack can't be called twice on the track");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("Config is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *track = new (std::nothrow) TrackWriter();

  if (*track == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new TrackWriter");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    switch (config->codec) {
      case Codec::mpegh_mhm: {
        if (config->enableMpeghMultiStream) {
          SMpeghMhm2TrackConfig mpeghConfig;
          internal_fillMpeghConfig(mpeghConfig, config);

          (*track)->trackWriter =
              isobmff->isobmffWriter->trackWriter<CMpeghTrackWriter>(mpeghConfig);
        } else {
          SMpeghMhm1TrackConfig mpeghConfig;
          internal_fillMpeghConfig(mpeghConfig, config);

          (*track)->trackWriter =
              isobmff->isobmffWriter->trackWriter<CMpeghTrackWriter>(mpeghConfig);
        }
        break;
      }
      case Codec::mpegh_mha: {
        ILO_ASSERT_WITH(config->enableMpeghMultiStream == false, std::invalid_argument,
                        "MPEG-H multistream of type mha2 is currently not supported.");
        SMpeghMha1TrackConfig mpeghConfig;
        internal_fillMpeghConfig(mpeghConfig, config);

        (*track)->trackWriter = isobmff->isobmffWriter->trackWriter<CMpeghTrackWriter>(mpeghConfig);
        break;
      }
      case Codec::mp4a: {
        SMp4aTrackConfig mp4aConfig;
        internal_fillBasicConfig(mp4aConfig, config);
        internal_fillAudioConfig(mp4aConfig, config);
        mp4aConfig.channelCount = config->channelCount;

        if (config->channelCount == 0) {
          ILO_LOG_WARNING("Channel count is zero. Is this intentional?");
        }

        if (config->dsc && config->dsc->size() != 0) {
          ilo::ByteBuffer::const_iterator dataBegin = config->dsc->cbegin();
          mp4aConfig.configRecord =
              ilo::make_unique<config::CMp4aDecoderConfigRecord>(dataBegin, config->dsc->end());
        }

        (*track)->trackWriter = isobmff->isobmffWriter->trackWriter<CMp4aTrackWriter>(mp4aConfig);
        break;
      }
      default:
        throw std::invalid_argument("This codec type not supported for writing yet.");
        break;
    }

    // Copy the movie timescale over to the tracks.
    // Needed for e.g. edit list duration recalculation in a copy track scenario, when the movie
    // timescale changed.
    (*track)->usedMovieTimescale = isobmff->usedMovieTimescale;

    // Copy the type of movie writer that was used.
    // Needed for the sample copy function to check what writing mode is active.
    (*track)->isFragmentedWriter = isobmff->isFragmentedWriter;
  } catch (const std::invalid_argument& e) {
    if (*track != nullptr) {
      delete *track;
      *track = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_PARAM_ERR;
  } catch (const std::exception& e) {
    if (*track != nullptr) {
      delete *track;
      *track = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroyTrackWriter(TrackWriter* track) {
  if (track == nullptr) {
    ILO_LOG_ERROR("Track is a zero pointer");
    return ISOBMFF_NOT_INIT_ERR;
  }

  delete track;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addSample(TrackWriter* track, Sample* sample) {
  if (track == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addSample has been called without calling isobmff_createTrackWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (sample == nullptr) {
    ILO_LOG_ERROR("sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    track->trackWriter->addSample(*sample);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addEditListEntry(TrackWriter* track, const EditListEntry* editListEntry) {
  if (track == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addEditListEntry has been called without calling isobmff_createTrackWriter first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (editListEntry == nullptr) {
    ILO_LOG_ERROR("editListEntry is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    SEdit edit;
    edit.mediaRate = editListEntry->mediaRate;
    edit.mediaTime = editListEntry->mediaTime;
    edit.segmentDuration = editListEntry->segmentDuration;

    track->trackWriter->addEditListEntry(edit);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_addTrackUserDataEntry(TrackWriter* trackWriter, const uint8_t* data,
                                          const uint32_t size) {
  if (trackWriter == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_addTrackUserDataEntry has been called without calling isobmff_createTrackWriter "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("user data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size < 8) {
    ILO_LOG_ERROR(
        "user given buffer needs to have at least a size of 8 bytes (size and type fields)");
    return ISOBMFF_PARAM_ERR;
  }

  uint32_t tmpSize = static_cast<uint32_t>(data[3] | data[2] << 8 | data[1] << 16 | data[0] << 24);

  if (tmpSize != size) {
    ILO_LOG_ERROR(
        "sizes mismatch between what is given to the user and what has been found in the buffer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    ilo::ByteBuffer buffer(data, data + size);
    trackWriter->trackWriter->addUserData(buffer);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

/* ------------Public sample methods------------------- */

ISOBMFF_ERR isobmff_createSample(Sample** sample, const uint64_t preAllocSampleSize) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Invalid isobmff sample handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the sample already points to data */
  if (*sample != nullptr) {
    ILO_LOG_ERROR("isobmff_createSample can't be called twice on the sample");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  *sample = new (std::nothrow) Sample();

  if (*sample == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new Sample");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    if (preAllocSampleSize > 0) {
      (*sample)->rawData.reserve(static_cast<size_t>(preAllocSampleSize));
    }
  } catch (const std::exception& e) {
    if (*sample != nullptr) {
      delete *sample;
      *sample = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_destroySample(Sample* sample) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  delete sample;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_resetSample(Sample* sample) {
  try {
    if (sample == nullptr) {
      ILO_LOG_ERROR("Sample is a zero pointer");
      return ISOBMFF_PARAM_ERR;
    }

    sample->clear();
  } catch (const std::exception& e) {
    ILO_LOG_ERROR("Failed to clear sample: ", e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleData(Sample* sample, uint8_t** data, uint64_t* size) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (size == nullptr) {
    ILO_LOG_ERROR("Sample size is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *size = sample->rawData.size();

  if (sample->rawData.size() != 0) {
    *data = sample->rawData.data();
  } else {
    *data = nullptr;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleDuration(Sample* sample, uint64_t* duration) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (duration == nullptr) {
    ILO_LOG_ERROR("Sample duration is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *duration = sample->duration;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleCtsOffset(Sample* sample, int64_t* ctsOffset) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (ctsOffset == nullptr) {
    ILO_LOG_ERROR("Sample ctsOffset is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *ctsOffset = sample->ctsOffset;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleFragmentNum(Sample* sample, uint32_t* fragmentNumber) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (fragmentNumber == nullptr) {
    ILO_LOG_ERROR("Sample fragmentNumber is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *fragmentNumber = sample->fragmentNumber;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleSyncFlag(Sample* sample, uint8_t* isSyncSample) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (isSyncSample == nullptr) {
    ILO_LOG_ERROR("Sample isSyncSample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *isSyncSample = (sample->isSyncSample == true) ? 1 : 0;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_getSampleGroup(Sample* sample, SampleGroup_C* sampleGroupType, int16_t* value) {
  if (sample == nullptr) {
    ILO_LOG_ERROR("Sample is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (sampleGroupType == nullptr) {
    ILO_LOG_ERROR("Sample sampleGroupType is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  if (value == nullptr) {
    ILO_LOG_ERROR("Sample sample group value is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  *sampleGroupType = convertSampleGroupEnum(sample->sampleGroupInfo.type);
  switch (*sampleGroupType) {
    case SampleGroup_Roll:
    case SampleGroup_Prol:
      *value = sample->sampleGroupInfo.rollDistance;
      break;
    case SampleGroup_Sap:
      *value = sample->sampleGroupInfo.sapType;
      break;
    default:
      *value = 0;
      break;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleData(Sample* sample, const uint8_t* data, const uint64_t size) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleData has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  if (data == nullptr) {
    ILO_LOG_ERROR("Sample Data is a zero pointer");
    return ISOBMFF_PARAM_ERR;
  }

  try {
    sample->rawData = ilo::ByteBuffer(data, data + size);
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleDuration(Sample* sample, const uint64_t duration) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleDuration has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  sample->duration = duration;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleCtsOffset(Sample* sample, const int64_t ctsOffset) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleCtsOffset has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  sample->ctsOffset = ctsOffset;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleFragmentNum(Sample* sample, const uint32_t fragmentNumber) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleFragmentNum has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  sample->fragmentNumber = fragmentNumber;

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleSyncFlag(Sample* sample, const uint8_t isSyncSample) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleSyncFlag has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  if (isSyncSample > 1) {
    ILO_LOG_ERROR("Flag must have a value of 0 or 1. Assuming a value of 1.");
  }

  sample->isSyncSample = (isSyncSample >= 1);

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_setSampleGroup(Sample* sample, const SampleGroup_C sampleGroupType,
                                   const int16_t value) {
  if (sample == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_setSampleGroup has been called without calling isobmff_createSample first");
    return ISOBMFF_PARAM_ERR;
  }

  sample->sampleGroupInfo.type = convertSampleGroupCEnum(sampleGroupType);
  if (sample->sampleGroupInfo.type == SampleGroupType::sap) {
    if (value < 1 || value > 6) {
      ILO_LOG_ERROR(
          "value for sample group of type sap is not compatible according to specification. "
          "Allowed range is [1 - 6] (inclusive)");
      return ISOBMFF_PARAM_ERR;
    }
    sample->sampleGroupInfo.sapType = static_cast<uint8_t>(value);
  } else {
    sample->sampleGroupInfo.rollDistance = value;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createMovieConfigFromReader(ISOBMFF_Reader* isobmff,
                                                const uint8_t copyMovieUdta, MovieConfig** config) {
  if (isobmff == nullptr || isobmff->isobmffReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_createMovieConfigFromReader has been called without calling isobmff_create first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid movie config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the movie config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createMovieConfigFromReader can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  try {
    std::unique_ptr<IodsConfig> iodsConfig = nullptr;

    if (isobmff->iodsInfo && isobmff->iodsInfo->iodsInfoAvailable()) {
      iodsConfig = ilo::make_unique<IodsConfig>();
      iodsConfig->audioProfileLevelIndication = isobmff->iodsInfo->audioProfileLevelIndication();
    }

    *config = new (std::nothrow) MovieConfig();

    if (*config == nullptr) {
      ILO_LOG_ERROR("Could not allocate memory for a new MovieConfig");
      return ISOBMFF_MEMORY_ERR;
    }

    (*config)->majorBrand = isobmff->movieInfo.majorBrand;
    (*config)->compatibleBrands = isobmff->movieInfo.compatibleBrands;
    (*config)->movieTimeScale = isobmff->movieInfo.timeScale;
    (*config)->iodsConfig = std::move(iodsConfig);
    if (copyMovieUdta > 0) {
      (*config)->userData = isobmff->movieInfo.userData;
    }

  } catch (const std::exception& e) {
    if (*config != nullptr) {
      delete *config;
      *config = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_createTrackConfigFromReader(TrackReader* track, TrackConfig** config) {
  if (track == nullptr || track->trackReader.genericTrackReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_createTrackConfigFromReader has been called without calling isobmff_getTrack "
        "first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (config == nullptr) {
    ILO_LOG_ERROR("Invalid track config handle has been provided");
    return ISOBMFF_NOT_INIT_ERR;
  }

  /* Check if the track config already points to data */
  if (*config != nullptr) {
    ILO_LOG_ERROR("isobmff_createTrackConfigFromReader can't be called twice on the config");
    return ISOBMFF_ALREADY_INIT_ERR;
  }

  if (track->trackInfo.type != TrackType::audio) {
    ILO_LOG_ERROR("isobmff_createTrackConfigFromReader currently only supports audia track");
    return ISOBMFF_NOT_IMPL_ERR;
  }

  *config = new (std::nothrow) TrackConfig();

  if (*config == nullptr) {
    ILO_LOG_ERROR("Could not allocate memory for a new TrackConfig");
    return ISOBMFF_MEMORY_ERR;
  }

  try {
    auto audiotrackReader =
        dynamic_cast<CGenericAudioTrackReader*>(track->trackReader.genericTrackReader.get());
    if (audiotrackReader != nullptr) {
      (*config)->sampleRate = audiotrackReader->sampleRate();
      (*config)->channelCount = audiotrackReader->channelCount();
    }

    (*config)->codec = track->trackInfo.codec;
    (*config)->trackTimeScale = track->trackInfo.timescale;
    (*config)->trackId = track->trackInfo.trackId;
    (*config)->language = track->trackInfo.language;
    (*config)->dsc = ilo::make_unique<ilo::ByteBuffer>(
        track->trackReader.genericTrackReader->decoderConfigRecord());
    if (track->trackReader.genericTrackReader->codingName() == ilo::toFcc("mhm2")) {
      (*config)->enableMpeghMultiStream = true;
    }
    (*config)->mpegHPLcompatibleSets = track->mpeghProfileAndLevelCompatibleSets;
  } catch (const std::exception& e) {
    if (*config != nullptr) {
      delete *config;
      *config = nullptr;
    }
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_copyUdataFromTrack(TrackReader* trackReader, TrackWriter* trackWriter) {
  if (trackReader == nullptr || trackReader->trackReader.genericTrackReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copyUdataForTrack has been called without creating a track reader first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (trackWriter == nullptr || trackWriter->trackWriter == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copyUdataForTrack has been called without creating a track writer first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  try {
    for (const auto& userDataEntry : trackReader->userData) {
      trackWriter->trackWriter->addUserData(userDataEntry);
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_copyEditListsFromTrack(TrackReader* trackReader, TrackWriter* trackWriter) {
  if (trackReader == nullptr || trackReader->trackReader.genericTrackReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copyEditListsForTrack has been called without creating a track reader first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (trackWriter == nullptr || trackWriter->trackWriter == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copyEditListsForTrack has been called without creating a track writer first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  try {
    // Note: editListEntry must be a copy, since we do not want to edit the original!
    for (auto editListEntry : trackReader->trackInfo.editList) {
      if (trackWriter->usedMovieTimescale != 0 &&
          trackReader->isobmff->movieInfo.timeScale != trackWriter->usedMovieTimescale) {
        editListEntry.segmentDuration = static_cast<uint64_t>(
            std::floor(editListEntry.segmentDuration * trackWriter->usedMovieTimescale /
                       trackReader->isobmff->movieInfo.timeScale));
      }
      trackWriter->trackWriter->addEditListEntry(editListEntry);
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }

  return ISOBMFF_OK;
}

ISOBMFF_ERR isobmff_copySamplesFromTrack(TrackReader* trackReader, TrackWriter* trackWriter) {
  if (trackReader == nullptr || trackReader->trackReader.genericTrackReader == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copySamplesForTrack has been called without creating a track reader first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  if (trackWriter == nullptr || trackWriter->trackWriter == nullptr) {
    ILO_LOG_ERROR(
        "isobmff_copySamplesForTrack has been called without creating a track writer first");
    return ISOBMFF_NOT_INIT_ERR;
  }

  // Only supported for plain (flat) mp4 file writers at the moment.
  if (trackWriter->isFragmentedWriter) {
    ILO_LOG_ERROR("isobmff_copySamplesForTrack is currently not supported for fragment writers");
    return ISOBMFF_NOT_IMPL_ERR;
  }

  try {
    CSample sample(trackReader->trackInfo.maxSampleSize);
    trackReader->trackReader.genericTrackReader->nextSample(sample);
    while (!sample.empty()) {
      // Clear fragment number, since we do not support fragmented writing with this copy function.
      // If a fragmented mp4 input is read, it will be defragmented on-the-fly.
      sample.fragmentNumber = 0;
      trackWriter->trackWriter->addSample(sample);
      trackReader->trackReader.genericTrackReader->nextSample(sample);
    }
  } catch (const std::exception& e) {
    ILO_LOG_ERROR(e.what());
    return ISOBMFF_LIB_ERR;
  }
  return ISOBMFF_OK;
}
